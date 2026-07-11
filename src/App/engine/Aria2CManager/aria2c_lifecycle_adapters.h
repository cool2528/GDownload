#pragma once

#include <memory>

#include "aria2c_lifecycle.h"
#include "aria2c_websocket_rpc_client.h"

namespace gdl::engine {

	namespace detail {
		inline bool IsReadyState(State state) { return state == State::kConnected; }
		inline bool ShutdownSucceeded(const Result<bool>& result) { return static_cast<bool>(result); }
		using SynchronizedStateCallback = Aria2cWebSocketClient::SynchronizedStateCallback;

		class IAria2RpcClientBackend {
		   public:
			using StateCallback = std::function<void(const State&, std::string)>;
			virtual ~IAria2RpcClientBackend() = default;
			virtual void SetStateCallback(StateCallback callback) = 0;
			virtual void ClearStateCallback() = 0;
			virtual void Open() = 0;
			virtual bool IsConnected() const = 0;
			virtual Result<bool> Shutdown() = 0;
			virtual void DisableAutoReconnect() = 0;
			virtual void Disconnect() = 0;
		};
	}  // namespace detail

	class Engine_API Aria2ProcessLifecycleAdapter final : public IAria2ProcessLifecycle {
	   public:
		int64_t Execute(const String_View& command, const std::vector<String>& arguments) override;
		bool IsAlive(int64_t pid) override;
		void Shutdown(int64_t pid, int grace_ms) override;
	};

	class Engine_API Aria2RpcLifecycleAdapter final : public IAria2RpcLifecycle {
	   public:
		// 非 owning：调用方必须保证 client 的生命周期覆盖 adapter。
		explicit Aria2RpcLifecycleAdapter(Aria2cWebSocketClient& client);
		explicit Aria2RpcLifecycleAdapter(std::shared_ptr<Aria2cWebSocketClient> client);
		explicit Aria2RpcLifecycleAdapter(std::shared_ptr<detail::IAria2RpcClientBackend> client);
		~Aria2RpcLifecycleAdapter() override;

		void SetStateCallback(StateCallback callback) override;
		void ClearStateCallback() override;
		void Open() override;
		bool IsReady() const override;
		bool RequestShutdown() override;
		void DisableReconnect() override;
		void Disconnect() override;

	   private:
		std::shared_ptr<detail::IAria2RpcClientBackend> client_;
	};

}  // namespace gdl::engine
