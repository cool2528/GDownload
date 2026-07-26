#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <algorithm>
#include <deque>
#include <mutex>
#include "cache/cache.h"
namespace gdl {
	namespace ui {
		namespace browser {

            enum class TaskState : int { kComplete = 0, kActive, kPause, kWaiting, kError, kRemoved };

			// 停滞的两种形状。它们的界面文案必须不同 —— 说错了比不说更糟:
			//   kNothingLands         一个字节都没落盘。判据是"自 bytes_done 最后一次发生任何
			//                         增长以来,线上又送来了整整一个字节预算的数据"。引擎
			//                         accumulate_blocks 的空闲超时会把未凑齐的区间整段丢弃,
			//                         或磁盘写入本身在失败;此时 bytes_done 纹丝不动,
			//                         "none of it is being written" 是实话。
			//   kWrittenThenDiscarded bytes_done 仍在增长,只是长时间涨不过历史最高水位 ——
			//                         整个 part 的 MD4 没过被 alloc.reset_part() 作废、正在重下
			//                         同一个 part(或多 part 文件里某个 part 被重置、其余 part
			//                         照常推进)。数据确实落了盘,说"存不下来"是假话。
			enum class StallKind : int { kNone = 0, kNothingLands = 1, kWrittenThenDiscarded = 2 };

			// 进度速率估计器:对 task_current_size(即引擎的 bytes_done)自身的推进做差分,
			// 得到"真正落盘的速率"。它与引擎上报的 task_download_speed(线上到达的字节)
			// 是两个不同的量,绝不能互相顶替:
			//   * 线上到达的字节可能永远落不了盘 —— accumulate_blocks 空闲超时会把未凑齐的
			//     区间整段丢弃、part 的 MD4 没过会 reset_part 整段重下、consume_block 对
			//     间隙帧与完全重叠帧直接弃帧(而线上计量发生在 consume_block 之前);
			//   * 所以拿线上速率当分母去算"进度剩余 / 速率",分子分母不同源,压缩传输下连
			//     量纲都不匹配(分母是压缩后字节、分子是解压后字节),算出来的 ETA 每秒都
			//     差不多、永不收敛,把一个真卡住的任务粉饰成"马上就好"。
			class ProgressRateTracker {
			   public:
				// 速率窗口长度。窗口里保留的不是"每一次采样",而是"每一次进度推进事件"
				// (见 rate_bps()):bytes_done 是阶跃量,拿"窗口内任意两点之差"去算速率,
				// 窗口装不下一个完整台阶时结果恒为 0,ETA 就会在"某个值"和"未知"之间抖。
				// 改成两次推进之间求差,量纲天然对齐,45 秒只决定"平滑多少次推进"。
				static constexpr std::int64_t kWindowMs = 45000;
				// 起步保护:两次推进相隔太近(例如同一秒落了两批)算出来的瞬时速率荒谬地高,
				// ETA 会先给一个乐观得离谱的值。跨度不足 3 秒时一律报"未知"。
				static constexpr std::int64_t kMinSpanMs = 3000;

				// 单次采样最多按 5 秒计入线上字节积分。进程被挂起、或界面线程被饿死时,
				// 相邻两次快照可能相隔几十秒;按真实间隔积分会凭空积出几十 MB 的"线上
				// 到达量",把一次卡顿伪造成停滞的证据。
				static constexpr std::int64_t kMaxIntegrationStepMs = 5000;

				// 停滞的字节预算 = 观测到的落盘粒度 × 该系数。取 3:健康的下载每收满一个
				// 粒度必定推进一次,留 3 倍余量足以吸收线上重传/弃帧带来的浪费。
				static constexpr std::int64_t kStallBudgetFactor = 3;
				// 预算下限。粒度还没观测到时(一次推进都没见过)用它兜底。当前引擎的粒度
				// 是一批 3 × 184320 = 552960 字节,2 MiB ≈ 3.8 批 —— 即便流水线深度翻到 11
				// 倍,首次推进之前也不会误报;首次推进之后粒度就被测出来了,与深度无关。
				static constexpr std::int64_t kMinStallBudgetBytes = 2 * 1024 * 1024;
				// 预算上限。防止一次异常巨大的推进(例如恢复任务时 bytes_done 一次跳几百 MB)
				// 把预算抬到永远够不到、从此再也报不出停滞。
				static constexpr std::int64_t kMaxStallBudgetBytes = 64 * 1024 * 1024;

				// wire_speed_bps 是引擎上报的线上到达速率(每秒真值,来自独立的 wire meter)。
				// 它不参与速率/水位计算,只做两件事:
				//   1) 记住"最近一次线上还有数据到达是什么时候" —— 判断"在收但收不出进展"
				//      必须知道确实还在收,而这件事不能逐帧去问:慢速点滴源(例如每 2 秒一个
				//      子帧)有一半的 1 秒采样速度恰好为 0,逐帧判据会让告警以 1 秒为周期明灭;
				//   2) 对它积分,得到"自上次进度推进以来线上一共送来了多少字节" —— 这是停滞
				//      判据的分母,取代了原来的纯墙钟计时。
				void Sample(std::int64_t current_size, std::int64_t wire_speed_bps, std::int64_t now_ms) {
					// 时间倒流只可能来自换了时钟源或测试;此时窗口里的跨度全不可信,整段作废。
					// 【进度回退不走这条路】part 的 MD4 没过会 reset_part() 把 bytes_done 打回
					// 去,那不是"没有信息",恰恰是最该被记下来的一次倒退 —— 见下面的水位判据。
					if (!started_ || now_ms < latest_ms_) {
						Restart(current_size, now_ms);
					}
					else {
						// 线上到达量的积分。判"停滞"绝不能只看墙钟:引擎是按批落盘的
						// (download.cpp 的 kPipelineDepth 一次要 3 个 AICH 块,c2c_connection
						// 的 accumulate_blocks 要求整批凑齐才返回,sync_progress 只在写盘成功
						// 后调用),一批 552960 字节在 5 KiB/s 下要 108 秒才凑齐,期间 bytes_done
						// 纹丝不动却完全健康。改看"自上次推进以来线上一共送来了多少字节":
						// 健康的下载每收满一个粒度必定推进一次,真卡住时线上字节会一直堆高
						// 而进度不动。
						const std::int64_t step = std::min(now_ms - latest_ms_, kMaxIntegrationStepMs);
						if (wire_speed_bps > 0 && step > 0) {
							const std::int64_t arrived = wire_speed_bps * step / 1000;
							wire_bytes_since_growth_ += arrived;
							wire_bytes_since_peak_ += arrived;
						}
						if (current_size > last_size_) {
							// 任何增长都证明"确实有数据写进了文件"。kNothingLands 那句文案
							// 到此刻为止成立不了,计数从头起算。
							last_growth_ms_			 = now_ms;
							wire_bytes_since_growth_ = 0;
							// 顺手把引擎真实的落盘粒度测出来。桌面端不再假设它等于多少个
							// AICH 块 —— 流水线深度改了、或者多源并发把粒度抬高了,这里
							// 自动跟着变。
							progress_step_bytes_ = std::max(progress_step_bytes_, current_size - last_size_);
						}
						else if (current_size < last_size_) {
							// 进度回退只可能来自 alloc.reset_part():之前那段爬升已被整段作废,
							// 拿它算速率会给出一个永不兑现的 ETA。推进历史整段清空,速率暂时
							// 报"未知"。
							advances_.clear();
						}
						// 只有真正刷新历史最高水位才算"取得了新进展"。若按"比上一次采样大"
						// 来算,一个反复 reset_part 的任务会在每次重下的爬升段不断刷新这个
						// 时刻,于是整夜原地踏步也永远报不出停滞 —— 而那正是最该报的一种。
						if (current_size > peak_size_) {
							peak_size_			   = current_size;
							last_advance_ms_	   = now_ms;
							wire_bytes_since_peak_ = 0;
							advances_.push_back(Point{now_ms, current_size});
							// 至少留两次推进:慢速源两次推进可能相隔几百秒,窗口按时间裁剪
							// 会把它裁成空的,速率随即变"未知"、ETA 开始明灭。
							while (advances_.size() > 2 && now_ms - advances_.front().ms > kWindowMs) {
								advances_.pop_front();
							}
						}
						last_size_ = current_size;
						latest_ms_ = now_ms;
					}
					if (wire_speed_bps > 0) {
						has_wire_activity_	   = true;
						last_wire_activity_ms_ = now_ms;
					}
				}

				// 任务离开活动态(暂停/等待/终态)时调用。暂停期间引擎照样每秒推快照,
				// 那些平样本既不代表"下载没进展",也不该被算进速率窗口:算进去的话,
				// 一次完全健康的恢复会在瞬间满足停滞条件而弹出告警,恢复后的速率还会被
				// 暂停期的平样本稀释成零头,ETA 随之虚高数倍。
				// 直接把窗口整段作废,恢复时从恢复那一刻重新起算 —— 速率短暂地报"未知"
				// 是诚实的,拿暂停期的样本算出来的速率不是。
				void Suspend() {
					advances_.clear();
					started_		   = false;
					has_wire_activity_ = false;
				}

				// 返回 0 表示"暂时给不出可信估计":推进次数不足两次、两次之间跨度太短,
				// 或刚发生过一次 part 回退(推进历史被整段清空)。此时 ETA 必须报"未知",
				// 而不是拿回退前后的差值算出一个负数或荒谬的值。
				//
				// 分子分母都取自"进度推进事件",两次推进之间的间隔无论多长都被如实计入 ——
				// 5 KiB/s 单源每 108 秒推进 552960 字节,算出来正好是 5120 B/s。
				std::int64_t rate_bps() const {
					if (advances_.size() < 2) return 0;
					const std::int64_t span = advances_.back().ms - advances_.front().ms;
					if (span < kMinSpanMs) return 0;
					const std::int64_t delta = advances_.back().size - advances_.front().size;
					if (delta <= 0) return 0;
					return delta * 1000 / span;
				}

				bool has_samples() const { return started_; }

				// 判定停滞所需的线上字节量。粒度是测出来的,不是硬编码的引擎常数 ——
				// 这就是这套判据对流水线深度免疫的地方。
				std::int64_t stall_budget_bytes() const {
					return std::clamp(progress_step_bytes_ * kStallBudgetFactor, kMinStallBudgetBytes,
									  kMaxStallBudgetBytes);
				}

				// 自最后一次刷新历史最高水位以来:过了多久 / 线上又送来了多少字节
				std::int64_t ms_since_peak() const { return started_ ? latest_ms_ - last_advance_ms_ : 0; }
				std::int64_t wire_bytes_since_peak() const { return wire_bytes_since_peak_; }

				// 自 bytes_done 最后一次发生任何增长以来:过了多久 / 线上又送来了多少字节
				std::int64_t ms_since_growth() const { return started_ ? latest_ms_ - last_growth_ms_ : 0; }
				std::int64_t wire_bytes_since_growth() const { return wire_bytes_since_growth_; }

				// 最近 grace_ms 毫秒内线上是否还有数据到达。用窗口而非逐帧判断,避免点滴
				// 源把告警抖成闪烁;从头到尾一个字节都没来过时恒为 false ——"没有源"是
				// 另一种故障,不能套用"在收但收不出进展"的文案。
				bool wire_active_within(std::int64_t grace_ms) const {
					if (!has_wire_activity_ || !started_) return false;
					return latest_ms_ - last_wire_activity_ms_ <= grace_ms;
				}

			   private:
				struct Point {
					std::int64_t ms{0};
					std::int64_t size{0};
				};

				void Restart(std::int64_t current_size, std::int64_t now_ms) {
					advances_.clear();
					started_				 = true;
					peak_size_				 = current_size;
					last_size_				 = current_size;
					last_advance_ms_		 = now_ms;
					last_growth_ms_			 = now_ms;
					latest_ms_				 = now_ms;
					wire_bytes_since_peak_	 = 0;
					wire_bytes_since_growth_ = 0;
					progress_step_bytes_	 = 0;
					has_wire_activity_		 = false;
					last_wire_activity_ms_	 = now_ms;
				}

				std::deque<Point> advances_;	   // 刷新历史水位的那些采样点(速率只用它们)
				std::int64_t peak_size_{0};		   // 迄今观察到的最高进度(水位)
				std::int64_t last_size_{0};		   // 上一次采样的进度值
				std::int64_t last_advance_ms_{0};  // 最后一次刷新水位的时刻
				std::int64_t last_growth_ms_{0};   // 最后一次 bytes_done 有任何增长的时刻
				std::int64_t latest_ms_{0};
				std::int64_t wire_bytes_since_peak_{0};	   // 自上次刷新水位以来线上累计到达字节
				std::int64_t wire_bytes_since_growth_{0};  // 自上次进度增长以来线上累计到达字节
				std::int64_t progress_step_bytes_{0};	   // 观测到的最大一次进度推进(引擎落盘粒度)
				std::int64_t last_wire_activity_ms_{0};	   // 最后一次线上有数据到达的时刻
				bool started_{false};					   // 是否已有活动态采样
				bool has_wire_activity_{false};			   // 本轮窗口里线上是否到达过数据
			};

			class DownloadTaskInfo {
			   public:
				DownloadTaskInfo() = default;
				// 拷贝/移动一律用编译器生成版本。这里原本是手写的四件套,逐个成员罗列 ——
				// 新增成员时漏写一个不会报错,只会在运行期丢字段,这个类的成员还在持续增加。
				DownloadTaskInfo(const DownloadTaskInfo&)				 = default;
				DownloadTaskInfo(DownloadTaskInfo&&) noexcept			 = default;
				DownloadTaskInfo& operator=(const DownloadTaskInfo&)	 = default;
				DownloadTaskInfo& operator=(DownloadTaskInfo&&) noexcept = default;

				QString task_id() const { return task_id_; }
				TaskState task_state() const { return task_state_; }
				QString task_file_name() const { return task_file_name_; }
				QString task_save_path() const { return task_save_path_; }
                QString task_download_link() const { return task_download_link_; }
				QString task_error_code() const { return task_error_code_; }
				QString task_error_message() const { return task_error_message_; }
				std::int64_t task_total_size() const { return task_total_size_; }
				std::int64_t task_current_size() const { return task_current_size_; }
				std::int64_t task_download_speed() const { return task_download_speed_; }
				std::int64_t task_connections() const { return task_connections_; }
				// eD2k 专有:该任务迄今发现的源总数(含已放弃/冷却中的源,只增不减)。aria2/BT 任务恒为 0,
				// UI 据此隐藏"Sources"标签。与 task_connections()(此刻真正握着连接的对端数)是两个口径。
				std::int64_t task_sources() const { return task_sources_; }
				// eD2k 专有:此刻停在对端上传队列里等放行的源数。aria2/BT 任务恒为 0,UI 据此隐藏
				// "Queued"标签。它与 task_connections() 一起区分两种表象相同、处置完全不同的故障:
				// 排队多而连接少 = 源都在排队(等着就好);二者都少而 task_sources() 很大 = 源多但触达不了。
				std::int64_t task_queued_sources() const { return task_queued_sources_; }

				void set_task_id(const QString& task_id) { task_id_ = task_id; }
				void set_task_state(TaskState state) { task_state_ = state; }
				void set_task_file_name(const QString& file_name) { task_file_name_ = file_name; }
                void set_task_save_path(const QString& save_path) {
                    task_save_path_ = save_path;
                    task_save_path_ = task_save_path_.replace("//", "/");
                }
				void set_task_download_link(const QString& link) {
					if (link.isEmpty()) return;
					task_download_link_ = link;
				}
				void set_task_error_code(const QString& error_code) { task_error_code_ = error_code; }
				void set_task_error_message(const QString& error_message) { task_error_message_ = error_message; }
				void set_task_total_size(std::int64_t total_size) { task_total_size_ = total_size; }
				void set_task_current_size(std::int64_t current_size) { task_current_size_ = current_size; }
				void set_task_download_speed(std::int64_t download_speed) { task_download_speed_ = download_speed; }
				void set_task_connections(std::int64_t task_connections) { task_connections_ = task_connections; }
				void set_task_sources(std::int64_t task_sources) { task_sources_ = task_sources; }
				void set_task_queued_sources(std::int64_t task_queued_sources) {
					task_queued_sources_ = task_queued_sources;
				}

				// 记一次进度采样。桌面端每秒收一次引擎快照,每次都重建一份全新的 TaskInfo,
				// 因此历史窗口必须先从旧条目继承过来(见 DownloadTaskModel::UpdateTask)。
				void SampleProgress(std::int64_t now_ms) {
					// 只有活动态的采样才算数。暂停/等待中的任务引擎照样每秒推快照(ed2k 的
					// 1s 采样只过滤 completed/failed/cancelled,paused 照发;BrowserManager 的
					// kPause 分支把行留在 active_model_ 并走 UpdateTaskById),那些平样本
					// 与"下载没进展"完全是两回事,记进窗口会把一次健康的恢复直接判成停滞。
					if (task_state_ != TaskState::kActive) {
						progress_tracker_.Suspend();
						return;
					}
					progress_tracker_.Sample(task_current_size_, task_download_speed_, now_ms);
				}
				void InheritProgressHistoryFrom(const DownloadTaskInfo& previous) {
					progress_tracker_ = previous.progress_tracker_;
				}

				// 进度速率:current_size 自身的时间差分,与线上速率(task_download_speed)分开
				std::int64_t progress_rate_bps() const { return progress_tracker_.rate_bps(); }

				// 停滞判据的时间下限(不是判据本身)。它只负责挡住"高速下载偶尔卡半秒"
				// 这类抖动:10 MiB/s 的任务几百毫秒就能攒够字节预算,没有时间下限的话一次
				// 写盘打嗝就会弹告警。取 90 秒 —— 与改判据之前同值,于是新判据报出的告警
				// 严格是旧判据的子集,只会更晚、更少,不可能凭空多出一条。
				//
				// 【单靠墙钟是错的,这正是本次修复的缺陷】引擎按批落盘:download.cpp 的
				// kPipelineDepth = 3,一次向对端要 3 个 AICH 块;c2c_connection 的
				// accumulate_blocks 主循环 while (!all_done()) 要求整批凑齐才返回;
				// sync_progress() 只在 write_block_async 成功之后调用。所以 bytes_done 的
				// 最小推进单位是 3 × 184320 = 552960 字节。5 KiB/s 下这要 108 秒才凑齐,
				// 期间进度纹丝不动而线上速率稳定在 5 KiB/s —— 纯墙钟判据会在 t=90..107 挂
				// 告警、t=108 批次到货又撤下,每 108 秒翻转一次(16.7% 的时间挂着橙色),
				// 而且那句"none of it is being written"是彻头彻尾的假话。误报阈值恰好是
				// 552960/90 = 6144 B/s:任何聚合速率低于 6 KiB/s 的下载一律周期性误报。
				static constexpr std::int64_t kStallWarningMs = 90000;

				// 线上"最近还在收数据"的宽限期。慢速点滴源(每 2 秒一个子帧)有一半的 1 秒
				// 采样速度为 0,逐帧要求"此刻速度 > 0"会让告警以 1 秒为周期明灭;取 30 秒,
				// 既盖得住这种抖动,又能在源真的全掉线后半分钟内把这条文案撤下来 ——
				// "一个源都没有"是另一种故障,自有它自己的说明文字。
				static constexpr std::int64_t kWireIdleGraceMs = 30000;

				// "长时间没有取得新进展"。判据由两部分组成,必须同时成立:
				//   (1) 距上次刷新历史最高水位已超过 kStallWarningMs —— 挡高速抖动;
				//   (2) 这期间线上送来的字节已超过一整份"字节预算" —— 真正的判据。
				//
				// 【为什么只认历史最高水位】进度当下在不在涨完全不能说明问题:reset_part()
				// 之后重下同一个 part,进度会连着涨 190-2000 秒(整整一个 PART_SIZE =
				// 9,728,000 字节)。用"进度在涨"当短路条件,等于把这种最该报的停滞彻底
				// 屏蔽掉(引擎 download.hpp 的 ProgressFn 注释明文禁止过)。
				//
				// 【为什么用字节而不是墙钟】健康的慢速下载里,进度是按引擎的落盘粒度整块
				// 跳的,墙钟上的"静止"只说明这一批还没凑齐,不说明有任何问题。换成"线上
				// 收够了 N 倍粒度却仍无推进",健康与故障才真正分得开:收满一个粒度必定
				// 推进一次,而卡住时线上字节会一直堆高。粒度由 ProgressRateTracker 从实际
				// 上报中测出(progress_step_bytes_),桌面端不硬编码引擎的 kPipelineDepth ——
				// 深度从 3 改成别的值,预算自己跟着变。
				//
				// 【两种形状怎么分】走到这里已经确定"长时间没有取得新进展",剩下的只是把
				// 界面上要说的那句话对准事实:
				//   * 自上次 bytes_done 有任何增长以来也收够了一份预算 ⇒ 确实一个字节都
				//     没落盘,kNothingLands 那句话成立;
				//   * 否则 bytes_done 一直在涨、只是涨不过水位 ⇒ 落了盘又被撤销,归到
				//     kWrittenThenDiscarded。多 part 文件里某个 part 被重置、其余 part 照常
				//     推进,净进度相对水位确实 ≤ 0,也落在这一类,文案同样成立。
				StallKind stall_kind() const {
					if (task_state_ != TaskState::kActive) return StallKind::kNone;
					if (task_total_size_ > 0 && task_current_size_ >= task_total_size_) {
						return StallKind::kNone;
					}
					if (!progress_tracker_.has_samples()) return StallKind::kNone;
					// 线上一个字节都不来 = 没有源,是另一种故障,不套用这条文案
					if (!progress_tracker_.wire_active_within(kWireIdleGraceMs)) return StallKind::kNone;

					const std::int64_t budget = progress_tracker_.stall_budget_bytes();
					if (progress_tracker_.ms_since_peak() < kStallWarningMs) return StallKind::kNone;
					if (progress_tracker_.wire_bytes_since_peak() < budget) return StallKind::kNone;

					const bool nothing_landed = progress_tracker_.ms_since_growth() >= kStallWarningMs &&
												progress_tracker_.wire_bytes_since_growth() >= budget;
					return nothing_landed ? StallKind::kNothingLands : StallKind::kWrittenThenDiscarded;
				}

				bool progress_stalled() const { return stall_kind() != StallKind::kNone; }

				double progress() const {
					if (task_total_size_ <= 0) return 0.0;
					return (static_cast<double>(task_current_size_) / task_total_size_) * 100.0;
				}

				// 剩余时间 = 进度剩余字节 / 进度速率。分子分母必须同源:分子是"还差多少字节
				// 落盘",分母就只能是"每秒有多少字节落盘",不能用引擎上报的线上速率顶替。
				// 进度速率给不出可信值(样本不足、或窗口内一个字节都没落盘)时返回 -1,
				// 界面显示"Unknown" —— 这是修复前就有的诚实行为,必须保留。
				int remaining_time() const {
					if (task_total_size_ <= 0) return -1;
					const std::int64_t remaining_size = task_total_size_ - task_current_size_;
					if (remaining_size <= 0) return 0;
					// 已经判定为长时间没有新进展时,窗口里那点涨幅是重下同一个 part 的爬升,
					// 它最终会被整段作废。拿它算出来的 ETA 每轮都一样、永不收敛(实测恒定
					// 在 "9h39m"),与旁边那句"没有新进展"直接打架。此时只能报"未知"。
					if (progress_stalled()) return -1;
					const std::int64_t rate = progress_tracker_.rate_bps();
					if (rate <= 0) return -1;  // 返回-1表示无法计算
					return static_cast<int>(remaining_size / rate);
				}

				QString FormatRemainingTime() const {
					int seconds = remaining_time();
					if (seconds < 0) {
						return QObject::tr("Unknown");
					}

					const int hour = seconds / 3600;
					seconds %= 3600;
					const int minute = seconds / 60;
					seconds %= 60;

					QString result;
					if (hour > 0) {
						result += QString::number(hour) + " " + (hour == 1 ? QObject::tr("h") : QObject::tr("h"));
						if (minute > 0) {
							result += " " + QString::number(minute) + " " +
									  (minute == 1 ? QObject::tr("m") : QObject::tr("m"));
						}
					}
					else if (minute > 0) {
						result += QString::number(minute) + " " + (minute == 1 ? QObject::tr("m") : QObject::tr("m"));
						if (seconds > 0) {
							result += " " + QString::number(seconds) + " " +
									  (seconds == 1 ? QObject::tr("s") : QObject::tr("s"));
						}
					}
					else {
						result = QString::number(seconds) + " " + (seconds == 1 ? QObject::tr("s") : QObject::tr("s"));
					}

					return result;
				}

				static QString FormatFileSize(std::int64_t size) {
					const double kb = 1024.0;
					const double mb = kb * 1024.0;
					const double gb = mb * 1024.0;
					const double tb = gb * 1024.0;

					if (size >= tb) {
						return QString::number(size / tb, 'f', 2) + " TB";
					}
					else if (size >= gb) {
						return QString::number(size / gb, 'f', 2) + " GB";
					}
					else if (size >= mb) {
						return QString::number(size / mb, 'f', 2) + " MB";
					}
					else if (size >= kb) {
						return QString::number(size / kb, 'f', 2) + " KB";
					}
					else {
						return QString::number(size) + " B";
					}
				}

			   private:
				QString task_id_;
				TaskState task_state_{TaskState::kError};
				QString task_file_name_;
				QString task_save_path_;
                QString task_download_link_;
				QString task_error_code_;
				QString task_error_message_;
				std::int64_t task_total_size_{0};
				std::int64_t task_current_size_{0};
				std::int64_t task_download_speed_{0};
				std::int64_t task_connections_{0};
				std::int64_t task_sources_{0};
				std::int64_t task_queued_sources_{0};
				ProgressRateTracker progress_tracker_;
			};

			class DownloadTaskModel : public QAbstractListModel {
				Q_OBJECT
				Q_PROPERTY(int count READ GetTaskCount NOTIFY countChanged)  // 供 QML 直接绑定计数（P4）
			   public:
				enum Roles {
					kTaskId = Qt::UserRole + 1,
					kTaskState,
					kTaskFileName,
					kTaskSavePath,
					kTaskTotalSize,
					kTaskCurrentSize,
					kTaskDownloadSpeed,
					kTaskProgress,
					kTaskRemainingTime,
                    kTaskConnections,
                    kTaskSources,
                    kTaskQueuedSources,
                    kTaskDownloadLink,
					kTaskErrorCode,
					kTaskErrorMessage,
					// 未经格式化的原始字节数。QML 的可见性判据不能拿 FormatFileSize 的输出
					// 比大小:它只保留两位小数,10 GB 量级上能把 5 MB 的缺口舍成同一个字符串,
					// 于是一个真的没下完的任务会被当成完整的。
					kTaskTotalSizeBytes,
					kTaskCurrentSizeBytes,
					// "长时间没有取得新进展":线上还在收数据,但进度越不过历史最高水位
					kTaskProgressStalled,
					// 停滞的形状(StallKind):一个字节都没落盘 / 落了盘又被整段作废重下。
					// 两者的处置和文案都不同,不能混成一句话
					kTaskProgressStallKind
				};

			   public:
				explicit DownloadTaskModel(QObject* parent = nullptr);
				~DownloadTaskModel() override;
				QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QHash<int, QByteArray> roleNames() const override;
				bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

				// now_ms 为进度采样时刻(毫秒)。生产路径传 -1,表示"取单调钟当前时刻";
				// 单元测试传确定值,才能在不真等 45 秒的前提下覆盖进度速率与停滞判据。
				void AddTask(const DownloadTaskInfo& task, std::int64_t now_ms = -1);
				bool RemoveTask(int index);
				bool RemoveTaskById(const QString& task_id);
				bool UpdateTask(int index, const DownloadTaskInfo& task, std::int64_t now_ms = -1);
				bool UpdateTaskById(const QString& task_id, const DownloadTaskInfo& task,
									std::int64_t now_ms = -1);
				DownloadTaskInfo* GetTask(int index);
				DownloadTaskInfo* GetTaskById(const QString& task_id);
				QStringList GetTaskIds() const;
				void ClearAllTasks();
				int GetTaskCount() const;
				bool ContainsTask(const QString& task_id) const;
				bool IsTombstoned(const QString& task_id) const;
				void ClearTombstone(const QString& task_id);

			   Q_SIGNALS:
				void countChanged();

			   private:
				QVector<DownloadTaskInfo> task_lists_;
				QHash<QString, QString> remove_task_id_;
				std::deque<QString> remove_order_;  // 墓碑插入顺序，配合上限做 FIFO 淘汰（M2）
				static constexpr int kMaxTombstones = 512;
				mutable std::mutex mutex_;
			};
		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
