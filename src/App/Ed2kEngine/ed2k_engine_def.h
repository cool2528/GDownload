#pragma once

// ed2k 引擎 PubSub 话题定义（对齐 engine_def.h 风格）
// 1s 采样的活动任务进度数组（JSON）
#define kEd2kActiveProgress "ed2k.active.progress"
// 单任务状态变更事件（JSON）
#define kEd2kTaskState "ed2k.task.state"

// 搜索结果批次（JSON 对象，append=true 表示翻页追加）
#define kEd2kSearchResult "ed2k.search.result"
// 服务器列表快照（JSON 对象）
#define kEd2kServerList "ed2k.server.list"
// 服务器连接状态变化（JSON 对象）
#define kEd2kServerState "ed2k.server.state"
// Kad 状态快照（JSON 对象）
#define kEd2kKadStatus "ed2k.kad.status"
