#include <libwebsockets.h>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "logger.h"
namespace gdl {
	namespace engine {
		class WebSocketClient {
		   public:
			// 回调函数类型定义
			using MessageCallback	 = std::function<void(const std::string&)>;
			using ConnectCallback	 = std::function<void()>;
			using DisconnectCallback = std::function<void()>;
			using ErrorCallback		 = std::function<void(const std::string&)>;

			WebSocketClient() : context_(nullptr), websocket_(nullptr), isConnected_(false) {
				// 设置日志级别
				lws_set_log_level(LLL_ERR | LLL_WARN, nullptr);
			}

			~WebSocketClient() { disconnect(); }

			// 连接到WebSocket服务器
			bool connect(const std::string& url, int port, const std::string& path = "/") {
				disconnect();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				LOG_INFO("Attempting to connect to Aria2 RPC at {}:{}{}", url, port, path);

				// 保存连接参数供重连使用
				lastUrl_  = url;
				lastPort_ = port;
				lastPath_ = path;

				struct lws_context_creation_info info;
				memset(&info, 0, sizeof(info));

				protocols_ = {{
								  "ws",								 // name
								  &WebSocketClient::globalCallback,	 // callback
								  0,								 // per_session_data_size
								  32768,							 // rx_buffer_size
								  0,								 // id
								  this,								 // user
								  32768								 // tx_packet_size
							  },
							  {nullptr, nullptr, 0, 0, 0, nullptr, 0}};

				info.port = CONTEXT_PORT_NO_LISTEN;

				info.protocols			   = protocols_.data();
				info.gid				   = -1;
				info.uid				   = -1;
				info.options			   = 0;
				info.ka_time			   = 10;  // keepalive 时间
				info.ka_probes			   = 3;	  // keepalive 探测次数
				info.ka_interval		   = 10;  // keepalive 间隔
				info.retry_and_idle_policy = NULL;

				LOG_DBG("Creating context...");
				context_ = lws_create_context(&info);
				if (!context_) {
					LOG_ERR("Failed to create context");
					return false;
				}

				struct lws_client_connect_info conn_info;
				memset(&conn_info, 0, sizeof(conn_info));

				conn_info.context					= context_;
				conn_info.address					= url.c_str();
				conn_info.port						= port;
				conn_info.path						= path.c_str();
				conn_info.host						= url.c_str();
				conn_info.origin					= url.c_str();
				conn_info.protocol					= protocols_[0].name;
				conn_info.pwsi						= &websocket_;
				conn_info.ssl_connection			= 0;
				conn_info.ietf_version_or_minus_one = -1;
				conn_info.userdata					= this;
				conn_info.retry_and_idle_policy		= NULL;

				LOG_DBG("Connecting to {}:{}{}", url, port, path);
				websocket_ = lws_client_connect_via_info(&conn_info);
				if (!websocket_) {
					LOG_ERR("Failed to create websocket connection");
					return false;
				}

				// 启动服务线程
				shouldStop_	   = false;
				serviceThread_ = std::thread([this]() {
					int retry_count		  = 0;
					const int MAX_RETRIES = 5;

					while (!shouldStop_) {
						if (context_) {
							int n = lws_service(context_, 50);
							if (n < 0) {
								LOG_ERR("lws_service error: {}", n);
								retry_count++;
								if (retry_count >= MAX_RETRIES) {
									LOG_ERR("Max retries reached, stopping service thread");
									break;
								}
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								continue;
							}
							retry_count = 0;  // 重置重试计数
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
					LOG_INFO("Service thread stopped");
				});

				// 等待连接建立或超时
				// const int TIMEOUT_MS		= 5000;
				// const int SLEEP_INTERVAL_MS = 100;
				// int waited_ms				= 0;

				// while (!isConnected_ && waited_ms < TIMEOUT_MS) {
				// 	std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL_MS));
				// 	waited_ms += SLEEP_INTERVAL_MS;

				// 	// 检查是否发生了错误
				// 	if (lastError_.length() > 0) {
				// 		LOG_ERR("Connection failed: {}", lastError_);
				// 		disconnect();
				// 		return false;
				// 	}
				// }

				// if (!isConnected_) {
				// 	LOG_ERR("Connection timeout after {} ms", TIMEOUT_MS);
				// 	disconnect();
				// 	return false;
				// }

				LOG_INFO("Successfully connected to Aria2 RPC");
				return true;
			}

			// 断开连接
			void disconnect() {
				LOG_DBG("Disconnecting websocket...");
				shouldStop_ = true;

				// 清除最后的错误信息
				{
					std::lock_guard<std::mutex> lock(errorMutex_);
					lastError_.clear();
				}

				// 等待服务线程结束
				if (serviceThread_.joinable()) {
					serviceThread_.join();
				}

				// 关闭 websocket 连接
				if (websocket_) {
					lws_callback_on_writable(websocket_);
					websocket_ = nullptr;
				}

				// 清理 context
				if (context_) {
					lws_context_destroy(context_);
					context_ = nullptr;
				}

				isConnected_ = false;

				// 清空消息队列
				std::lock_guard<std::mutex> lock(messageMutex_);
				std::queue<std::string> empty;
				std::swap(messageQueue_, empty);

				LOG_DBG("Websocket disconnected");
			}

			// 发送消息
			bool send(const std::string& message) {
				if (!isConnected_ || !websocket_) {
					LOG_ERR("Not connected");
					return false;
				}

				std::lock_guard<std::mutex> lock(messageMutex_);
				messageQueue_.push(message);
				lws_callback_on_writable(websocket_);
				return true;
			}

			// 设置回调
			void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
			void setConnectCallback(ConnectCallback cb) { connectCallback_ = std::move(cb); }
			void setDisconnectCallback(DisconnectCallback cb) { disconnectCallback_ = std::move(cb); }
			void setErrorCallback(ErrorCallback cb) { errorCallback_ = std::move(cb); }
			bool isConnect() const { return isConnected_; }

		   private:
			static int globalCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in,
									  size_t len) {
				auto protocol_ptr = lws_get_protocol(wsi);
				if (!protocol_ptr) return -1;
				auto client = static_cast<WebSocketClient*>(protocol_ptr->user);
				if (!client) {
					LOG_ERR("Client pointer is null");
					return -1;
				}

				switch (reason) {
					case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
						std::string error = "Connection error";
						if (in) {
							error += ": " + std::string(static_cast<char*>(in), len);
						}
						LOG_ERR(error);
						client->setLastError(error);
						client->isConnected_ = false;
						if (client->errorCallback_) {
							client->errorCallback_(error);
						}
						// 尝试自动重连
						if (!client->shouldStop_) {	 // 只在非主动断开的情况下重连
							std::thread([client]() {
								std::this_thread::sleep_for(std::chrono::seconds(2));  // 等待2秒后重连
								if (!client->shouldStop_ && !client->isConnected_) {
									client->reconnect();
								}
							}).detach();
						}
						break;
					}

					case LWS_CALLBACK_WSI_DESTROY:
						LOG_DBG("WSI destroyed");
						client->websocket_ = nullptr;  // 清除 websocket 指针
						break;

					case LWS_CALLBACK_CLIENT_CLOSED:
						LOG_INFO("Connection closed");
						client->isConnected_ = false;
						client->websocket_	 = nullptr;	 // 清除 websocket 指针
						if (client->disconnectCallback_) {
							client->disconnectCallback_();
						}
						// 尝试自动重连
						if (!client->shouldStop_) {	 // 只在非主动断开的情况下重连
							std::thread([client]() {
								std::this_thread::sleep_for(std::chrono::seconds(2));  // 等待2秒后重连
								if (!client->shouldStop_ && !client->isConnected_) {
									client->reconnect();
								}
							}).detach();
						}
						break;

					case LWS_CALLBACK_CLIENT_ESTABLISHED:
						LOG_INFO("Connection established");
						client->isConnected_ = true;
						if (client->connectCallback_) {
							client->connectCallback_();
						}
						break;

					case LWS_CALLBACK_CLIENT_RECEIVE:
						if (client->messageCallback_ && in && len > 0) {
							std::string message(static_cast<char*>(in), len);
							LOG_DBG("Received message: {}", message);
							client->messageCallback_(message);
						}
						break;

					case LWS_CALLBACK_CLIENT_WRITEABLE:
						client->sendPendingMessages();
						break;

					default:
						// 其他回调不需要特别处理
						break;
				}
				return 0;
			}

			void sendPendingMessages() {
				std::lock_guard<std::mutex> lock(messageMutex_);
				while (!messageQueue_.empty() && isConnected_) {
					const std::string& message = messageQueue_.front();

					// LWS_PRE 是必需的预留空间
					std::vector<uint8_t> buf(LWS_PRE + message.size());
					memcpy(buf.data() + LWS_PRE, message.data(), message.size());

					int sent = lws_write(websocket_, buf.data() + LWS_PRE, message.size(), LWS_WRITE_TEXT);

					if (sent < 0) {
						LOG_ERR("Error writing to socket");
						setLastError("Write error occurred");
						isConnected_ = false;  // 标记连接已断开
						if (errorCallback_) {
							errorCallback_("Write error occurred");
						}
						break;
					}
					else if (static_cast<size_t>(sent) < message.size()) {
						LOG_ERR("Partial write");
						// 保留消息以便重试
						break;
					}

					messageQueue_.pop();
				}
			}

		   private:
			struct lws_context* context_;
			struct lws* websocket_;
			std::vector<lws_protocols> protocols_;

			std::thread serviceThread_;
			std::atomic<bool> shouldStop_{false};
			std::atomic<bool> isConnected_{false};

			std::queue<std::string> messageQueue_;
			std::mutex messageMutex_;

			MessageCallback messageCallback_;
			ConnectCallback connectCallback_;
			DisconnectCallback disconnectCallback_;
			ErrorCallback errorCallback_;

			// 添加错误消息存储
			std::string lastError_;
			std::mutex errorMutex_;

			void setLastError(const std::string& error) {
				std::lock_guard<std::mutex> lock(errorMutex_);
				lastError_ = error;
			}

			// 添加重连机制
			bool reconnect() {
				LOG_INFO("Attempting to reconnect...");
				disconnect();  // 确保之前的连接完全关闭

				// 等待一段时间再重连
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));

				return connect(lastUrl_, lastPort_, lastPath_);
			}

			// 存储最后的连接信息，用于重连
			std::string lastUrl_;
			int lastPort_{0};
			std::string lastPath_;
		};
	}  // namespace engine
}  // namespace gdl
