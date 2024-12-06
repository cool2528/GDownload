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

			WebSocketClient() { lws_set_log_level(LLL_ERR | LLL_WARN, nullptr); }

			~WebSocketClient() { disconnect(); }

			bool connect(const std::string& url, int port, const std::string& path = "/") {
				server_address_ = url;
				port_			= port;
				path_			= path;
				memset(&info_, 0, sizeof(info_));
				info_.options			  = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
				info_.port				  = CONTEXT_PORT_NO_LISTEN;
				protocols_				  = {{"json", globalCallback, 0, 0, 0, nullptr, 0}, LWS_PROTOCOL_LIST_TERM};
				info_.protocols			  = protocols_.data();
				info_.fd_limit_per_thread = 1 + 1 + 1;

				context_ = lws_create_context(&info_);
				if (!context_) {
					LOG_ERR("init context faild");
					return false;
				}
				is_init_context_ = true;
				lws_sul_schedule(context_, 0, &sul_, &sulCallback, 1);
				loop_thread_ = std::thread([this] {
					int n = 0;
					while (n >= 0 && !interrupted_) {
						n = lws_service(context_, 0);
					}
				});

				return true;
			}

			void disconnect() {
				interrupted_ = true;
				if (loop_thread_.joinable()) {
					loop_thread_.join();
				}
				if (context_) {
					lws_context_destroy(context_);
					context_ = nullptr;
				}
				is_connected_ = false;
			}

			bool send(const std::string& message) {
				if (interrupted_ || !wsi_) {
					LOG_ERR("Not connected");
					return false;
				}
				std::lock_guard lock(message_mutex_);
				message_queue_.push(message);
				lws_callback_on_writable(wsi_);
				return true;
			}

			// 设置回调
			void setMessageCallback(MessageCallback cb) { message_mallback_ = std::move(cb); }
			void setConnectCallback(ConnectCallback cb) { connect_callback_ = std::move(cb); }
			void setDisconnectCallback(DisconnectCallback cb) { disconnect_callback_ = std::move(cb); }
			void setErrorCallback(ErrorCallback cb) { error_callback_ = std::move(cb); }
			bool isConnect() const { return is_connected_; }

		   private:
			static int globalCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in,
									  size_t len) {
				WebSocketClient* self = static_cast<WebSocketClient*>(user);
				switch (reason) {
					case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
						std::string msg(static_cast<char*>(in), len);
						LOG_ERR("CLIENT_CONNECTION_ERROR: {}", msg);
						if (self->error_callback_) {
							self->error_callback_(msg);
						}
						self->is_connected_ = false;
						goto do_retry;
						break;
					}
					case LWS_CALLBACK_CLIENT_RECEIVE: {
						std::string msg(static_cast<char*>(in), len);
						if (self->message_mallback_) {
							self->message_mallback_(msg);
						}
					} break;
					case LWS_CALLBACK_CLIENT_ESTABLISHED: {
						self->is_connected_ = true;
						if (self->connect_callback_) {
							self->connect_callback_();
						}
					} break;
					case LWS_CALLBACK_CLIENT_CLOSED: {
						self->is_connected_ = false;
						std::string msg(static_cast<char*>(in), len);
						LOG_WARN("LWS_CALLBACK_CLIENT_CLOSED:{}", msg);
						goto do_retry;
					} break;
					case LWS_CALLBACK_CLIENT_WRITEABLE:
						self->sendPendingMessages(wsi);
						break;
					default:
						break;
				}
				return lws_callback_http_dummy(wsi, reason, user, in, len);
			do_retry:
				if (lws_retry_sul_schedule_retry_wsi(wsi, &self->sul_, sulCallback, &self->retry_count)) {
					LOG_WARN("connection attempts exhausted");
					self->interrupted_ = true;
				}
				return 0;
			}

			static void sulCallback(lws_sorted_usec_list_t* sul) {
				WebSocketClient* self = lws_container_of(sul, WebSocketClient, sul_);
				struct lws_client_connect_info info;
				memset(&info, 0, sizeof(info));

				info.context = self->context_;
				info.port	 = self->port_;
				info.address = self->server_address_.c_str();
				info.path	 = self->path_.c_str();
				info.host	 = info.address;
				info.origin	 = info.address;
				//info.ssl_connection				   = ~LCCSCF_USE_SSL;
				info.protocol					   = "ws";
				info.local_protocol_name		   = "json";
				info.pwsi						   = &self->wsi_;
				static const uint32_t backoff_ms[] = {1000, 2000, 3000, 4000, 5000};

				static const lws_retry_bo_t retry = {
					.retry_ms_table		  = backoff_ms,
					.retry_ms_table_count = LWS_ARRAY_SIZE(backoff_ms),
					.conceal_count		  = LWS_ARRAY_SIZE(backoff_ms),

					.secs_since_valid_ping	 = 3,
					.secs_since_valid_hangup = 10,

					.jitter_percent = 20,
				};
				info.retry_and_idle_policy = &retry;
				info.userdata			   = self;
				if (!lws_client_connect_via_info(&info)) {
					if (lws_retry_sul_schedule(self->context_, 0, sul, &retry, sulCallback, &self->retry_count)) {
						LOG_WARN("connection attempts exhausted");
						self->interrupted_ = true;
					}
				}
			}
			void sendPendingMessages(struct lws* wsi) {
				if (!wsi) return;
				std::lock_guard lock(message_mutex_);
				while (!message_queue_.empty() && is_connected_) {
					const std::string& message = message_queue_.front();
					std::vector<uint8_t> buf(LWS_PRE + message.size());
					memcpy(buf.data() + LWS_PRE, message.data(), message.size());
					int sent = lws_write(wsi, buf.data() + LWS_PRE, message.size(), LWS_WRITE_TEXT);
					if (sent < 0) {
						LOG_ERR("Error writing to socket");
						if (error_callback_) {
							error_callback_("Write error occurred");
						}
						break;
					}
					else if (static_cast<size_t>(sent) < message.size()) {
						LOG_ERR("Partial write");
						break;
					}

					message_queue_.pop();
				}
			}

		   private:
			std::vector<lws_protocols> protocols_;
			struct lws_context* context_{nullptr};
			struct lws_context_creation_info info_;
			lws_sorted_usec_list_t sul_;
			struct lws* wsi_{nullptr};
			unsigned short retry_count;
			std::thread loop_thread_;
			std::atomic_bool interrupted_{false};
			std::queue<std::string> message_queue_;
			std::mutex message_mutex_;

		   private:
			MessageCallback message_mallback_;
			ConnectCallback connect_callback_;
			DisconnectCallback disconnect_callback_;
			ErrorCallback error_callback_;
			std::atomic_bool is_connected_{false};
			std::string server_address_;
			unsigned int port_;
			std::atomic_bool is_init_context_{false};
			std::string path_;
		};
	}  // namespace engine
}  // namespace gdl
