#pragma once
#include <quickjs.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace gdl {
	namespace plugin {
		namespace js {

			// JS 异常信息（message + 调用栈）
			struct JsError {
				std::string message;
				std::string stack;
				bool timeout{false};  // 是否因执行超时被中断
			};

			// JSValue 的 RAII 守卫：离开作用域自动 JS_FreeValue
			class JsValueGuard {
			   public:
				JsValueGuard(JSContext* ctx, JSValue value) : ctx_(ctx), value_(value) {}
				~JsValueGuard() {
					if (ctx_) {
						JS_FreeValue(ctx_, value_);
					}
				}
				JsValueGuard(const JsValueGuard&)			 = delete;
				JsValueGuard& operator=(const JsValueGuard&) = delete;
				JsValueGuard(JsValueGuard&& other) noexcept : ctx_(other.ctx_), value_(other.value_) {
					other.ctx_ = nullptr;
				}
				JsValueGuard& operator=(JsValueGuard&&) = delete;

				JSValue get() const { return value_; }
				// 放弃所有权（调用方负责释放）
				JSValue release() {
					ctx_ = nullptr;
					return value_;
				}

			   private:
				JSContext* ctx_;
				JSValue value_;
			};

			// QuickJS Runtime/Context 的 RAII 封装
			// 职责：资源限制（内存/栈/CPU 超时）、受限 ES Module 加载、Promise 同步等待、异常捕获
			// 线程模型：非线程安全，调用方（JsPluginHost）负责串行化
			class JsRuntime {
			   public:
				struct Options {
					size_t memory_limit				= 64ull * 1024 * 1024;	// 单 Runtime 内存上限
					size_t stack_size				= 1ull * 1024 * 1024;	// JS 栈上限
					std::chrono::milliseconds default_timeout{60000};		// 单次调用默认超时
					std::filesystem::path module_root;						// 模块加载根目录（插件目录），import 不允许越界
				};

				explicit JsRuntime(Options options);
				~JsRuntime();
				JsRuntime(const JsRuntime&)			   = delete;
				JsRuntime& operator=(const JsRuntime&) = delete;

				JSContext* context() const { return ctx_; }
				JSRuntime* runtime() const { return rt_; }

				// 加载入口 ES Module 并返回其 default export 对象
				// 失败返回 std::nullopt，错误详情见 last_error()
				std::optional<JSValue> LoadModuleDefaultExport(const std::filesystem::path& entry_file);

				// 等待 Promise settle 并返回 resolve 值；非 Promise 值原样返回（转移所有权给调用方）
				// reject/超时返回 std::nullopt，错误详情见 last_error()
				// 注意：本函数会消耗（释放）传入的 promise 引用
				std::optional<JSValue> AwaitPromise(JSValue promise, std::chrono::milliseconds timeout);

				// 调用 JS 函数，若返回 Promise 则等待完成；返回最终值（转移所有权给调用方）
				std::optional<JSValue> CallAndAwait(JSValue func, JSValue this_obj, int argc, JSValue* argv,
													std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

				const JsError& last_error() const { return last_error_; }

				// 执行超时截止点，由 InterruptHandler 检查；公开给宿主 API 在长回调（如验证码等待）前延长截止点
				void ExtendDeadline(std::chrono::milliseconds extra);

			   private:
				// 捕获当前 Context 的异常到 last_error_
				void CaptureException();
				// 设置本次调用的超时截止点
				void BeginCall(std::chrono::milliseconds timeout);

				static int InterruptHandler(JSRuntime* rt, void* opaque);
				static JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name, void* opaque);
				static char* ModuleNormalize(JSContext* ctx, const char* base_name, const char* name, void* opaque);

			   private:
				Options options_;
				JSRuntime* rt_{nullptr};
				JSContext* ctx_{nullptr};
				JsError last_error_;
				// 截止时间点（steady_clock 计数），0 表示无限制
				std::atomic<int64_t> deadline_ns_{0};
			};

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl
