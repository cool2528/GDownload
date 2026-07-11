#pragma once
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace gdl {
	namespace engine {

		namespace beast		= boost::beast;
		namespace http		= beast::http;
		namespace websocket = beast::websocket;
		namespace net		= boost::asio;
		using tcp			= boost::asio::ip::tcp;

		class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
		   public:
			using MessageCallback	 = std::function<void(const std::string&)>;
			using ConnectCallback	 = std::function<void()>;
			using DisconnectCallback = std::function<void()>;
			using ErrorCallback		 = std::function<void(const std::string&)>;

			// resolver_ 与 ws_ 共用同一 strand，避免 onResolve 跨 strand 触碰 ws_（B7）
			WebSocketClient(net::io_context& ioc) : strand_(net::make_strand(ioc)), resolver_(strand_), ws_(strand_) {}

			// 析构时不能调用 shared_from_this()：此时控制块已失效会抛 bad_weak_ptr，
			// 改为同步关闭（错误码吞掉），由上层保证 io_context 已停止后再析构。
			~WebSocketClient() {
				boost::system::error_code ec;
				if (ws_.is_open()) {
					ws_.close(websocket::close_code::normal, ec);
				}
			}

			bool connect(const std::string& host, const std::string& port, const std::string& path) {
				host_ = host;
				port_ = port;
				path_ = path;
				resolver_.async_resolve(host_, port_,
										beast::bind_front_handler(&WebSocketClient::onResolve, shared_from_this()));
				return true;
			}

			// 必须投递到 ws_ 所在的 strand 上执行，避免与 async_read/write 并发，
			// 违反 Beast stream 串行化要求导致数据竞争。
			void disconnect() {
				connected_.store(false);
				net::post(ws_.get_executor(), [self = shared_from_this()]() {
					if (self->ws_.is_open()) {
						self->ws_.async_close(websocket::close_code::normal,
											  beast::bind_front_handler(&WebSocketClient::onClose, self));
					}
				});
			}

			bool send(const std::string& message) {
				if (!connected_.load()) {
					return false;
				}
				net::post(ws_.get_executor(), [self = shared_from_this(), message]() { self->onSend(message); });
				return true;
			}

			void setMessageCallback(MessageCallback cb) { message_callback_ = std::move(cb); }
			void setConnectCallback(ConnectCallback cb) { connect_callback_ = std::move(cb); }
			void setDisconnectCallback(DisconnectCallback cb) { disconnect_callback_ = std::move(cb); }
			void setErrorCallback(ErrorCallback cb) { error_callback_ = std::move(cb); }
			bool isConnected() const { return connected_.load(); }

			void setAutoReconnect(bool enabled, int maxRetries = -1, int retryInterval = 5) {
				auto_reconnect_.store(enabled);
				max_retries_ = maxRetries;
				retry_interval_ = retryInterval;
			}

			void disableAutoReconnect() {
				auto_reconnect_.store(false);
				net::post(ws_.get_executor(), [self = shared_from_this()] {
					if (self->reconnect_timer_) {
						self->reconnect_timer_->cancel();
						self->reconnect_timer_.reset();
					}
				});
			}
			bool isAutoReconnectEnabled() const { return auto_reconnect_.load(); }

		   private:
			void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
				if (ec) {
					handleError("Resolve failed: " + ec.message());
					return;
				}
				beast::get_lowest_layer(ws_).async_connect(
					results, beast::bind_front_handler(&WebSocketClient::onConnect, shared_from_this()));
			}

			void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
				if (ec) {
					handleError("Connect failed: " + ec.message());
					return;
				}

				ws_.async_handshake(host_, path_,
									beast::bind_front_handler(&WebSocketClient::onHandshake, shared_from_this()));
			}

			void onHandshake(beast::error_code ec) {
				if (ec) {
					handleError("WebSocket handshake failed: " + ec.message());
					return;
				}

				connected_.store(true);
				retry_count_ = 0;

				// 重连成功后清空断连前残留的陈旧 RPC（id 已过期，重发无意义）。
				// 断连期间 send() 因 connected_==false 直接返回，队列不会新增；
				// 正常流程下此时 write_in_flight_ 必为 false，守卫仅为防御在途写缓冲被析构。
				if (!write_in_flight_) {
					std::queue<std::string> empty;
					message_queue_.swap(empty);
				}

				if (connect_callback_) {
					connect_callback_();
				}
				asyncRead();
			}

			void asyncRead() {
				ws_.async_read(read_buffer_, beast::bind_front_handler(&WebSocketClient::onRead, shared_from_this()));
			}

			void onRead(beast::error_code ec, std::size_t bytes_transferred) {
				if (ec) {
					handleError("Read failed: " + ec.message());
					return;
				}

				if (message_callback_) {
					std::string message(beast::buffers_to_string(read_buffer_.data()));
					message_callback_(message);
				}

				read_buffer_.consume(bytes_transferred);

				asyncRead();
			}

			// onSend/onWrite/asyncWrite 均在 ws strand 上执行（send 通过 post 投递），
			// message_queue_/write_in_flight_ 为 strand 独占，无需加锁。
			void onSend(const std::string& message) {
				message_queue_.push(message);

				// 用显式在途标记替代"队列大小即在途"约定，避免写错误后永久死锁（E1）
				if (write_in_flight_) {
					return;
				}

				write_in_flight_ = true;
				asyncWrite();
			}

			void asyncWrite() {
				ws_.async_write(net::buffer(message_queue_.front()),
								beast::bind_front_handler(&WebSocketClient::onWrite, shared_from_this()));
			}

			void onWrite(beast::error_code ec, std::size_t) {
				if (ec) {
					// 写失败先复位在途标记再报错，否则重连成功后写链永久死锁（E1）
					write_in_flight_ = false;
					handleError("Write failed: " + ec.message());
					return;
				}

				message_queue_.pop();

				if (!message_queue_.empty()) {
					asyncWrite();
				} else {
					write_in_flight_ = false;
				}
			}

			void onClose(beast::error_code ec) {
				connected_.store(false);
				if (ec) {
					handleError("Close failed: " + ec.message());
				}

				if (disconnect_callback_) {
					disconnect_callback_();
				}
			}

			void handleError(const std::string& error) {
				connected_.store(false);
				if (error_callback_) {
					error_callback_(error);
				}

				if (auto_reconnect_.load()) {
					if (max_retries_ == -1 || retry_count_ < max_retries_) {
						retry_count_++;
						reconnect_timer_ = std::make_shared<net::steady_timer>(
							ws_.get_executor(), std::chrono::seconds(retry_interval_));
						reconnect_timer_->async_wait([self = shared_from_this()](beast::error_code ec) {
							if (!ec) {
								self->reconnect();
							}
						});
						return;
					}
				}

				disconnect();
			}

			void reconnect() {
				if (!ws_.is_open()) {
					resolver_.async_resolve(host_, port_,
						beast::bind_front_handler(&WebSocketClient::onResolve, shared_from_this()));
				}
			}

		   private:
			net::strand<net::io_context::executor_type> strand_;
			tcp::resolver resolver_;
			websocket::stream<beast::tcp_stream> ws_;
			beast::flat_buffer read_buffer_;
			std::queue<std::string> message_queue_;
			bool write_in_flight_ = false;  // 是否有 async_write 在途（仅 ws strand 访问）

			std::string host_;
			std::string port_;
			std::string path_;

			MessageCallback message_callback_;
			ConnectCallback connect_callback_;
			DisconnectCallback disconnect_callback_;
			ErrorCallback error_callback_;
			std::shared_ptr<net::steady_timer> reconnect_timer_;

			std::atomic<bool> auto_reconnect_{false};
			std::atomic<bool> connected_{false};
			int max_retries_ = -1;        // -1 表示无限重试
			int retry_count_ = 0;
			int retry_interval_ = 5;      // 重试间隔（秒）
		};

	}  // namespace engine
}  // namespace gdl
