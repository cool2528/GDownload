#include "aria2c_lifecycle_adapters.h"

#include "process/process.h"
#include <stdexcept>

namespace gdl::engine {
	namespace {
		class Aria2RpcClientBackend final : public detail::IAria2RpcClientBackend {
		   public:
			explicit Aria2RpcClientBackend(std::shared_ptr<Aria2cWebSocketClient> client)
				: client_(std::move(client)) {}
			void SetStateCallback(StateCallback callback) override {
				client_->SetStateChanageCallback(std::move(callback));
			}
			void ClearStateCallback() override { client_->ClearStateChanageCallback(); }
			void Open() override { client_->Open(); }
			bool IsConnected() const override { return client_->IsConnected(); }
			Result<bool> Shutdown() override { return client_->Shutdown(); }
			void DisableAutoReconnect() override { client_->DisableAutoReconnect(); }
			void Disconnect() override { client_->Disconnect(); }

		   private:
			std::shared_ptr<Aria2cWebSocketClient> client_;
		};
	}  // namespace

	int64_t Aria2ProcessLifecycleAdapter::Execute(const String_View& command,
		const std::vector<String>& arguments) {
		return process::Execute(command, arguments);
	}

	bool Aria2ProcessLifecycleAdapter::IsAlive(int64_t pid) {
		return process::IsProcessExistByPid(pid);
	}

	void Aria2ProcessLifecycleAdapter::Shutdown(int64_t pid, int grace_ms) {
		process::ShutdownProcess(pid, grace_ms);
	}

	Aria2RpcLifecycleAdapter::Aria2RpcLifecycleAdapter(std::shared_ptr<Aria2cWebSocketClient> client)
		: Aria2RpcLifecycleAdapter(client ? std::make_shared<Aria2RpcClientBackend>(std::move(client)) : nullptr) {}

	Aria2RpcLifecycleAdapter::Aria2RpcLifecycleAdapter(
		std::shared_ptr<detail::IAria2RpcClientBackend> client)
		: client_(std::move(client)) {
		if (!client_) throw std::invalid_argument("aria2 RPC lifecycle client must not be null");
	}

	Aria2RpcLifecycleAdapter::~Aria2RpcLifecycleAdapter() { ClearStateCallback(); }

	void Aria2RpcLifecycleAdapter::SetStateCallback(StateCallback callback) {
		client_->SetStateCallback([callback = std::move(callback)](const State& state, std::string) {
			if (callback) callback(detail::IsReadyState(state));
		});
	}

	void Aria2RpcLifecycleAdapter::ClearStateCallback() { client_->ClearStateCallback(); }
	void Aria2RpcLifecycleAdapter::Open() { client_->Open(); }
	bool Aria2RpcLifecycleAdapter::IsReady() const { return client_->IsConnected(); }
	bool Aria2RpcLifecycleAdapter::RequestShutdown() { return detail::ShutdownSucceeded(client_->Shutdown()); }
	void Aria2RpcLifecycleAdapter::DisableReconnect() { client_->DisableAutoReconnect(); }
	void Aria2RpcLifecycleAdapter::Disconnect() { client_->Disconnect(); }

}  // namespace gdl::engine
