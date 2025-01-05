#pragma once
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
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

			WebSocketClient(net::io_context& ioc) : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)) {}

			~WebSocketClient() { disconnect(); }

			bool connect(const std::string& host, const std::string& port, const std::string& path) {
				host_ = host;
				port_ = port;
				path_ = path;
				resolver_.async_resolve(host_, port_,
										beast::bind_front_handler(&WebSocketClient::onResolve, shared_from_this()));
				return true;
			}

			void disconnect() {
				if (ws_.is_open()) {
					ws_.async_close(websocket::close_code::normal,
									beast::bind_front_handler(&WebSocketClient::onClose, shared_from_this()));
				}
			}

			bool send(const std::string& message) {
				net::post(ws_.get_executor(), [self = shared_from_this(), message]() { self->onSend(message); });
				return true;
			}

			void setMessageCallback(MessageCallback cb) { message_callback_ = std::move(cb); }
			void setConnectCallback(ConnectCallback cb) { connect_callback_ = std::move(cb); }
			void setDisconnectCallback(DisconnectCallback cb) { disconnect_callback_ = std::move(cb); }
			void setErrorCallback(ErrorCallback cb) { error_callback_ = std::move(cb); }
			bool isConnected() const { return ws_.is_open(); }

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

			void onSend(const std::string& message) {
				std::lock_guard<std::mutex> lock(queue_mutex_);
				message_queue_.push(message);

				if (message_queue_.size() > 1) {
					return;	 
				}

				asyncWrite();
			}

			void asyncWrite() {
				ws_.async_write(net::buffer(message_queue_.front()),
								beast::bind_front_handler(&WebSocketClient::onWrite, shared_from_this()));
			}

			void onWrite(beast::error_code ec, std::size_t) {
				if (ec) {
					handleError("Write failed: " + ec.message());
					return;
				}

				{
					std::lock_guard<std::mutex> lock(queue_mutex_);
					message_queue_.pop();
				}

				if (!message_queue_.empty()) {
					asyncWrite();
				}
			}

			void onClose(beast::error_code ec) {
				if (ec) {
					handleError("Close failed: " + ec.message());
				}

				if (disconnect_callback_) {
					disconnect_callback_();
				}
			}

			void handleError(const std::string& error) {
				if (error_callback_) {
					error_callback_(error);
				}
				disconnect();
			}

		   private:
			tcp::resolver resolver_;
			websocket::stream<beast::tcp_stream> ws_;
			beast::flat_buffer read_buffer_;
			std::queue<std::string> message_queue_;
			std::mutex queue_mutex_;

			std::string host_;
			std::string port_;
			std::string path_;

			MessageCallback message_callback_;
			ConnectCallback connect_callback_;
			DisconnectCallback disconnect_callback_;
			ErrorCallback error_callback_;
		};

	}  // namespace engine
}  // namespace gdl