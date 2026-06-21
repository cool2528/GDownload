#pragma once

namespace gdl {
	namespace ui {
		// 测试模式查询:当环境变量 GDOWNLOAD_TEST=1 时返回 true
		// 用于在启动期跳过 aria2c 子进程、自动更新、远程 cookie 等副作用,便于 UI 隔离测试
		bool isTestMode();
	}  // namespace ui
}  // namespace gdl
