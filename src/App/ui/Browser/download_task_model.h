#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <deque>
#include <mutex>
#include "cache/cache.h"
namespace gdl {
	namespace ui {
		namespace browser {

            enum class TaskState : int { kComplete = 0, kActive, kPause, kWaiting, kError, kRemoved };

			// 停滞的两种形状。它们的界面文案必须不同 —— 说错了比不说更糟:
			//   kNothingLands         一个字节都没落盘。引擎 accumulate_blocks 的空闲超时会把
			//                         未凑齐的区间整段丢弃,或磁盘写入本身在失败;此时 bytes_done
			//                         纹丝不动,"none of it can be saved" 是实话。
			//   kWrittenThenDiscarded 数据确实落了盘,只是整个 part 的 MD4 没过被 alloc.reset_part()
			//                         作废、正在重下同一个 part;此时 bytes_done 涨了又退,说
			//                         "存不下来"是假话 —— 存下来了,只是又被撤销了。
			enum class StallKind : int { kNone = 0, kNothingLands = 1, kWrittenThenDiscarded = 2 };

			// 进度速率估计器:对 task_current_size(即引擎的 bytes_done)自身做时间差分,
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
				// 滑动窗口长度。eD2k 的 bytes_done 以 AICH 块(184320 字节)为粒度跳变,
				// 常见单源速率 5-50 KiB/s ⇒ 4-37 秒才落一个块。窗口若短于最慢那一端的
				// 单块间隔,绝大多数窗口里一个块都没落,进度速率会被算成 0,ETA 会在
				// "某个值"和"未知"之间每秒抖动。取 45 秒:比 37 秒的最坏单块间隔多出
				// 约 20% 余量,同时短到真停滞能在一分钟内反映出来。aria2/BT 的进度每秒
				// 都在推进,窗口再长也只是把速率抹平一点,不改变 ETA 的量级。
				static constexpr std::int64_t kWindowMs = 45000;
				// 起步保护:窗口刚建立时跨度太短,eD2k 落下的第一个块会被算成"瞬时
				// 180 KiB/s",ETA 先给一个乐观得离谱的值。跨度不足 3 秒时一律报"未知"。
				static constexpr std::int64_t kMinSpanMs = 3000;

				// wire_speed_bps 是引擎上报的线上到达速率。它不参与速率/水位计算,只用来
				// 记住"最近一次线上还有数据到达是什么时候" —— 判断"在收但收不出进展"必须
				// 知道确实还在收,而这件事不能逐帧去问:慢速点滴源(例如每 2 秒一个子帧)
				// 有一半的 1 秒采样速度恰好为 0,逐帧判据会让告警以 1 秒为周期明灭。
				void Sample(std::int64_t current_size, std::int64_t wire_speed_bps, std::int64_t now_ms) {
					// 时间倒流只可能来自换了时钟源或测试;此时窗口里的跨度全不可信,整段作废。
					// 【进度回退不走这条路】part 的 MD4 没过会 reset_part() 把 bytes_done 打回
					// 去,那不是"没有信息",恰恰是最该被记下来的一次倒退 —— 见下面的水位判据。
					if (samples_.empty() || now_ms < latest_ms_) {
						Restart(current_size, now_ms);
					}
					else {
						// 只有真正刷新历史最高水位才算"前进过"。若按"比上一次采样大"来算,
						// 一个反复 reset_part 的任务会在每次重下的爬升段不断刷新这个时刻,
						// 于是整夜原地踏步也永远报不出停滞 —— 而那正是最该报的一种。
						if (current_size > peak_size_) {
							peak_size_		  = current_size;
							last_advance_ms_  = now_ms;
							moved_below_peak_ = false;
						}
						else if (current_size != last_size_) {
							// 没越过水位,但数值确实动了 —— 只可能是"落了盘又被整段作废"。
							// 这一位是区分两种停滞文案的唯一依据。
							moved_below_peak_ = true;
						}
						last_size_ = current_size;
						latest_ms_ = now_ms;
						samples_.push_back(Point{now_ms, current_size});
						// 保留至少两个点;丢弃早于窗口起点的历史(最多多留一个采样间隔)
						while (samples_.size() > 2 && now_ms - samples_.front().ms > kWindowMs) {
							samples_.pop_front();
						}
					}
					if (wire_speed_bps > 0) {
						has_wire_activity_	   = true;
						last_wire_activity_ms_ = now_ms;
					}
				}

				// 任务离开活动态(暂停/等待/终态)时调用。暂停期间引擎照样每秒推快照,
				// 那些平样本既不代表"下载没进展",也不该被算进速率窗口:算进去的话,
				// 一次完全健康的恢复会在瞬间满足"停滞 90 秒以上"而弹出告警,恢复后的
				// 45 秒里速率还会被暂停期的平样本稀释成零头,ETA 随之虚高数倍。
				// 直接把窗口整段作废,恢复时从恢复那一刻重新起算 —— 速率短暂地报"未知"
				// 是诚实的,拿暂停期的样本算出来的速率不是。
				void Suspend() {
					samples_.clear();
					has_wire_activity_ = false;
					moved_below_peak_  = false;
				}

				// 返回 0 表示"暂时给不出可信估计":样本不足、跨度太短,或窗口内的净增量
				// 不为正 —— 包含一次 part 回退的窗口正属于后者,此时 ETA 必须报"未知",
				// 而不是拿回退前后的差值算出一个负数或荒谬的值。
				std::int64_t rate_bps() const {
					if (samples_.size() < 2) return 0;
					const std::int64_t span = samples_.back().ms - samples_.front().ms;
					if (span < kMinSpanMs) return 0;
					const std::int64_t delta = samples_.back().size - samples_.front().size;
					if (delta <= 0) return 0;
					return delta * 1000 / span;
				}

				// 进度自最后一次刷新最高水位以来停滞了多久(以最近一次采样的时刻为准)
				std::int64_t stalled_ms() const {
					if (samples_.empty()) return 0;
					return latest_ms_ - last_advance_ms_;
				}

				bool has_samples() const { return !samples_.empty(); }

				// 自最后一次刷新水位以来,进度数值动过但始终没越过水位 —— 即"落了盘又被
				// 整段作废重下"。恒定不动的白流不会置位。
				bool moved_below_peak() const { return moved_below_peak_; }

				// 最近 grace_ms 毫秒内线上是否还有数据到达。用窗口而非逐帧判断,避免点滴
				// 源把告警抖成闪烁;从头到尾一个字节都没来过时恒为 false ——"没有源"是
				// 另一种故障,不能套用"在收但收不出进展"的文案。
				bool wire_active_within(std::int64_t grace_ms) const {
					if (!has_wire_activity_ || samples_.empty()) return false;
					return latest_ms_ - last_wire_activity_ms_ <= grace_ms;
				}

			   private:
				struct Point {
					std::int64_t ms{0};
					std::int64_t size{0};
				};

				void Restart(std::int64_t current_size, std::int64_t now_ms) {
					samples_.clear();
					samples_.push_back(Point{now_ms, current_size});
					peak_size_			   = current_size;
					last_size_			   = current_size;
					last_advance_ms_	   = now_ms;
					latest_ms_			   = now_ms;
					moved_below_peak_	   = false;
					has_wire_activity_	   = false;
					last_wire_activity_ms_ = now_ms;
				}

				std::deque<Point> samples_;
				std::int64_t peak_size_{0};		   // 迄今观察到的最高进度(水位)
				std::int64_t last_size_{0};		   // 上一次采样的进度值(用于识别"动过但没过水位")
				std::int64_t last_advance_ms_{0};  // 最后一次刷新水位的时刻
				std::int64_t latest_ms_{0};
				std::int64_t last_wire_activity_ms_{0};	 // 最后一次线上有数据到达的时刻
				bool has_wire_activity_{false};			 // 本轮窗口里线上是否到达过数据
				bool moved_below_peak_{false};
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

				// 停滞告警阈值。eD2k 最慢的常见单源(5 KiB/s)要 37 秒才凑够一个 184320 字节
				// 的块,几十秒不涨完全可能只是"下得慢",阈值必须显著高于它,否则天天误报、
				// 告警立刻被当噪音无视。取 90 秒:约为最坏单块间隔的 2.4 倍;同时略低于引擎
				// accumulate_blocks 的 100 秒空闲超时(超时会把未凑齐的缓冲整段丢弃,半死源
				// 每轮白流约 540 KiB),用户因此能在"白流一整轮"的时间尺度内看到警示。
				static constexpr std::int64_t kStallWarningMs = 90000;

				// 线上"最近还在收数据"的宽限期。慢速点滴源(每 2 秒一个子帧)有一半的 1 秒
				// 采样速度为 0,逐帧要求"此刻速度 > 0"会让告警以 1 秒为周期明灭;取 30 秒,
				// 既盖得住这种抖动,又能在源真的全掉线后半分钟内把这条文案撤下来 ——
				// "一个源都没有"是另一种故障,自有它自己的说明文字。
				static constexpr std::int64_t kWireIdleGraceMs = 30000;

				// "长时间没有取得新进展"。判据只认历史最高水位:进度当下在不在涨完全不能
				// 说明问题 —— reset_part() 之后重下同一个 part,进度会连着涨 190-2000 秒
				// (整整一个 PART_SIZE = 9,728,000 字节),远长于 45 秒的速率窗口,于是窗口
				// 整段落在爬升段内、rate_bps() 恒为正。用"进度在涨"当短路条件,等于把这种
				// 最该报的停滞彻底屏蔽掉(引擎 download.hpp 的 ProgressFn 注释明文禁止过)。
				//
				// 反过来,健康的慢速下载不会被误伤:5 KiB/s 单源要 36 秒才凑够一个 184320
				// 字节的块,但每落一个块水位就抬高一次,停滞时钟随之清零,永远够不到 90 秒。
				StallKind stall_kind() const {
					if (task_state_ != TaskState::kActive) return StallKind::kNone;
					if (task_total_size_ > 0 && task_current_size_ >= task_total_size_) {
						return StallKind::kNone;
					}
					// 线上一个字节都不来 = 没有源,是另一种故障,不套用这条文案
					if (!progress_tracker_.wire_active_within(kWireIdleGraceMs)) return StallKind::kNone;
					if (progress_tracker_.stalled_ms() < kStallWarningMs) return StallKind::kNone;
					return progress_tracker_.moved_below_peak() ? StallKind::kWrittenThenDiscarded
																: StallKind::kNothingLands;
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
