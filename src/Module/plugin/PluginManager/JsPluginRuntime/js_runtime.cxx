#include "js_runtime.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <fstream>
#include <sstream>

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// 读取文件全部内容；失败返回 nullopt
				std::optional<std::string> ReadFileContent(const std::filesystem::path& path) {
					std::ifstream file(path, std::ios::binary);
					if (!file.is_open()) {
						return std::nullopt;
					}
					std::ostringstream oss;
					oss << file.rdbuf();
					return oss.str();
				}

				// 判断 child 是否位于 root 目录内（防止 import 路径逃逸）
				bool IsPathInside(const std::filesystem::path& root, const std::filesystem::path& child) {
					std::error_code ec;
					auto canonical_root  = std::filesystem::weakly_canonical(root, ec);
					if (ec) {
						return false;
					}
					auto canonical_child = std::filesystem::weakly_canonical(child, ec);
					if (ec) {
						return false;
					}
					auto root_str  = canonical_root.generic_string();
					auto child_str = canonical_child.generic_string();
					if (child_str.size() < root_str.size()) {
						return false;
					}
					if (child_str.compare(0, root_str.size(), root_str) != 0) {
						return false;
					}
					// 防止前缀误匹配兄弟目录（如 root=/a/foo 误匹配 /a/foobar/x）：
					// 必须完全相等，或前缀后紧跟路径分隔符
					return child_str.size() == root_str.size() || child_str[root_str.size()] == '/'
						   || (!root_str.empty() && root_str.back() == '/');
				}
			}  // namespace

			JsRuntime::JsRuntime(Options options) : options_(std::move(options)) {
				rt_ = JS_NewRuntime();
				if (!rt_) {
					spdlog::error("[js-runtime] JS_NewRuntime failed");
					return;
				}
				JS_SetMemoryLimit(rt_, options_.memory_limit);
				JS_SetMaxStackSize(rt_, options_.stack_size);
				JS_SetInterruptHandler(rt_, &JsRuntime::InterruptHandler, this);
				JS_SetModuleLoaderFunc(rt_, &JsRuntime::ModuleNormalize, &JsRuntime::ModuleLoader, this);

				ctx_ = JS_NewContext(rt_);
				if (!ctx_) {
					spdlog::error("[js-runtime] JS_NewContext failed");
					JS_FreeRuntime(rt_);
					rt_ = nullptr;
				}
			}

			JsRuntime::~JsRuntime() {
				if (ctx_) {
					JS_FreeContext(ctx_);
				}
				if (rt_) {
					JS_FreeRuntime(rt_);
				}
			}

			int JsRuntime::InterruptHandler(JSRuntime* /*rt*/, void* opaque) {
				auto* self		 = static_cast<JsRuntime*>(opaque);
				auto deadline_ns = self->deadline_ns_.load(std::memory_order_relaxed);
				if (deadline_ns == 0) {
					return 0;
				}
				auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
								  std::chrono::steady_clock::now().time_since_epoch())
								  .count();
				// 返回非 0 表示中断 JS 执行
				return now_ns > deadline_ns ? 1 : 0;
			}

			void JsRuntime::BeginCall(std::chrono::milliseconds timeout) {
				if (timeout.count() <= 0) {
					timeout = options_.default_timeout;
				}
				auto deadline = std::chrono::steady_clock::now() + timeout;
				deadline_ns_.store(
					std::chrono::duration_cast<std::chrono::nanoseconds>(deadline.time_since_epoch()).count(),
					std::memory_order_relaxed);
			}

			void JsRuntime::ExtendDeadline(std::chrono::milliseconds extra) {
				auto current = deadline_ns_.load(std::memory_order_relaxed);
				if (current == 0) {
					return;
				}
				deadline_ns_.store(current + std::chrono::duration_cast<std::chrono::nanoseconds>(extra).count(),
								   std::memory_order_relaxed);
			}

			void JsRuntime::CaptureException() {
				last_error_ = {};
				JsValueGuard exception(ctx_, JS_GetException(ctx_));
				// 中断（超时）产生的异常是 InternalError: interrupted
				const char* msg = JS_ToCString(ctx_, exception.get());
				if (msg) {
					last_error_.message = msg;
					JS_FreeCString(ctx_, msg);
				} else {
					last_error_.message = "unknown error";
				}
				if (last_error_.message.find("interrupted") != std::string::npos) {
					last_error_.timeout = true;
				}
				JsValueGuard stack(ctx_, JS_GetPropertyStr(ctx_, exception.get(), "stack"));
				if (!JS_IsUndefined(stack.get()) && !JS_IsException(stack.get())) {
					const char* stack_str = JS_ToCString(ctx_, stack.get());
					if (stack_str) {
						last_error_.stack = stack_str;
						JS_FreeCString(ctx_, stack_str);
					}
				}
			}

			char* JsRuntime::ModuleNormalize(JSContext* ctx, const char* base_name, const char* name, void* opaque) {
				auto* self = static_cast<JsRuntime*>(opaque);
				// 仅允许相对路径 import，禁止绝对路径与裸模块名
				std::string spec(name);
				if (spec.rfind("./", 0) != 0 && spec.rfind("../", 0) != 0) {
					JS_ThrowReferenceError(ctx, "only relative imports are allowed: %s", name);
					return nullptr;
				}
				// 以 base（当前模块路径）所在目录为基准解析
				std::filesystem::path base(base_name);
				auto resolved = (base.parent_path() / spec).lexically_normal();
				// 越界检查：解析结果必须仍在插件目录内
				if (!IsPathInside(self->options_.module_root, resolved)) {
					JS_ThrowReferenceError(ctx, "import escapes plugin directory: %s", name);
					return nullptr;
				}
				auto resolved_str = resolved.generic_string();
				char* result	  = static_cast<char*>(js_malloc(ctx, resolved_str.size() + 1));
				if (!result) {
					return nullptr;
				}
				std::memcpy(result, resolved_str.c_str(), resolved_str.size() + 1);
				return result;
			}

			JSModuleDef* JsRuntime::ModuleLoader(JSContext* ctx, const char* module_name, void* opaque) {
				auto* self = static_cast<JsRuntime*>(opaque);
				std::filesystem::path path(module_name);
				if (!IsPathInside(self->options_.module_root, path)) {
					JS_ThrowReferenceError(ctx, "module outside plugin directory: %s", module_name);
					return nullptr;
				}
				auto content = ReadFileContent(path);
				if (!content) {
					JS_ThrowReferenceError(ctx, "could not load module: %s", module_name);
					return nullptr;
				}
				// 编译为模块（不执行）；执行由 import 机制驱动
				JSValue func_val = JS_Eval(ctx, content->c_str(), content->size(), module_name,
										   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
				if (JS_IsException(func_val)) {
					return nullptr;
				}
				auto* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func_val));
				// 模块定义的所有权归 Context，释放 JSValue 外壳
				JS_FreeValue(ctx, func_val);
				return module;
			}

			std::optional<JSValue> JsRuntime::LoadModuleDefaultExport(const std::filesystem::path& entry_file) {
				if (!ctx_) {
					last_error_ = {"runtime not initialized", "", false};
					return std::nullopt;
				}
				auto content = ReadFileContent(entry_file);
				if (!content) {
					last_error_ = {"cannot read entry file: " + entry_file.generic_string(), "", false};
					return std::nullopt;
				}
				BeginCall(options_.default_timeout);
				auto entry_name = entry_file.lexically_normal().generic_string();
				// 编译入口模块
				JSValue compiled = JS_Eval(ctx_, content->c_str(), content->size(), entry_name.c_str(),
										   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
				if (JS_IsException(compiled)) {
					CaptureException();
					return std::nullopt;
				}
				auto* module_def = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled));
				// 执行模块（quickjs-ng 返回模块求值 Promise，需等待 settle 以支持顶层 await）
				JSValue eval_result = JS_EvalFunction(ctx_, compiled);
				auto settled		= AwaitPromise(eval_result, options_.default_timeout);
				if (!settled) {
					return std::nullopt;
				}
				JS_FreeValue(ctx_, *settled);
				// 从模块命名空间取 default export
				JsValueGuard ns(ctx_, JS_GetModuleNamespace(ctx_, module_def));
				if (JS_IsException(ns.get())) {
					CaptureException();
					return std::nullopt;
				}
				JSValue default_export = JS_GetPropertyStr(ctx_, ns.get(), "default");
				if (JS_IsException(default_export)) {
					CaptureException();
					return std::nullopt;
				}
				if (JS_IsUndefined(default_export)) {
					last_error_ = {"entry module has no default export", "", false};
					return std::nullopt;
				}
				return default_export;
			}

			std::optional<JSValue> JsRuntime::AwaitPromise(JSValue promise, std::chrono::milliseconds timeout) {
				if (!ctx_) {
					JS_FreeValue(ctx_, promise);
					last_error_ = {"runtime not initialized", "", false};
					return std::nullopt;
				}
				if (JS_IsException(promise)) {
					CaptureException();
					JS_FreeValue(ctx_, promise);
					return std::nullopt;
				}
				// 非 Promise 值直接返回
				if (!JS_IsPromise(promise)) {
					return promise;
				}
				JsValueGuard guard(ctx_, promise);
				BeginCall(timeout);
				auto deadline = std::chrono::steady_clock::now()
								+ (timeout.count() > 0 ? timeout : options_.default_timeout);
				// 驱动微任务队列直至 Promise settle 或超时
				for (;;) {
					JSPromiseStateEnum state = JS_PromiseState(ctx_, promise);
					if (state == JS_PROMISE_FULFILLED) {
						return JS_PromiseResult(ctx_, promise);
					}
					if (state == JS_PROMISE_REJECTED) {
						JsValueGuard reason(ctx_, JS_PromiseResult(ctx_, promise));
						// 将 reject 原因写入 last_error_
						JS_Throw(ctx_, JS_DupValue(ctx_, reason.get()));
						CaptureException();
						return std::nullopt;
					}
					if (std::chrono::steady_clock::now() > deadline) {
						last_error_ = {"promise await timeout", "", true};
						return std::nullopt;
					}
					JSContext* job_ctx = nullptr;
					int job_ret		   = JS_ExecutePendingJob(rt_, &job_ctx);
					if (job_ret < 0) {
						// 微任务内抛出异常
						CaptureException();
						return std::nullopt;
					}
					if (job_ret == 0) {
						// 无微任务且 Promise 仍 pending：宿主 API 全部同步 resolve，
						// 正常不会出现该状态；视为插件逻辑错误（如 await 了永不 settle 的 Promise）
						last_error_ = {"promise will never settle (no pending jobs)", "", false};
						return std::nullopt;
					}
				}
			}

			std::optional<JSValue> JsRuntime::CallAndAwait(JSValue func, JSValue this_obj, int argc, JSValue* argv,
														   std::chrono::milliseconds timeout) {
				if (!ctx_) {
					last_error_ = {"runtime not initialized", "", false};
					return std::nullopt;
				}
				BeginCall(timeout);
				JSValue result = JS_Call(ctx_, func, this_obj, argc, argv);
				if (JS_IsException(result)) {
					CaptureException();
					JS_FreeValue(ctx_, result);
					return std::nullopt;
				}
				return AwaitPromise(result, timeout);
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl
