#pragma once

#include <QPoint>
#include <QSize>
#include <QString>

namespace gdl {
	namespace ui {
		namespace settings {

			// Settings 的纯虚接口
			// 用于在测试代码中注入 FakeSettingsManager 替身,实现 UI 逻辑的隔离测试
			// 实现类 SettingsImpl 通过 Q_OBJECT/Q_PROPERTY/Q_INVOKABLE 提供元对象信息,QML 侧不受影响
			// 注意:本接口不继承 QObject,避免与 SettingsImpl 的 QObject 基类形成菱形继承与 moc 歧义
			// setter 签名按 SETTING_PROPERTY 宏展开的 by-value 形式声明,确保 override 精确匹配
			class ISettings {
			   public:
				virtual ~ISettings() = default;

				// 窗口与界面
				virtual QSize	 GetWindowSize() const			  = 0;
				virtual void	 SetWindowSize(QSize value)		  = 0;
				virtual QString GetTheme() const				  = 0;
				virtual void	 SetTheme(QString value)			  = 0;
				virtual QString GetLanguage() const			  = 0;
				virtual void	 SetLanguage(QString value)		  = 0;
				virtual QString GetBtExludeTracker() const		  = 0;
				virtual void	 SetBtExludeTracker(QString value)  = 0;
				virtual QString GetBtTracker() const			  = 0;
				virtual void	 SetBtTracker(QString value)		  = 0;
				virtual QPoint	 GetWindowPosition() const		  = 0;
				virtual void	 SetWindowPosition(QPoint value)	  = 0;

				// 下载路径与 aria2 基础
				virtual QString GetDir() const					  = 0;
				virtual void	 SetDir(QString value)			  = 0;
				virtual QString GetUserAgent() const			  = 0;
				virtual void	 SetUserAgent(QString value)		  = 0;
				virtual QString GetAllProxy() const				  = 0;
				virtual void	 SetAllProxy(QString value)		  = 0;
				virtual QString GetConfPath() const				  = 0;
				virtual void	 SetConfPath(QString value)		  = 0;
				virtual QString GetSaveSession() const			  = 0;
				virtual void	 SetSaveSession(QString value)	  = 0;
				virtual QString GetGlobalProxy() const			  = 0;
				virtual void	 SetGlobalProxy(QString value)	  = 0;
				virtual QString GetBaiduPanCookies() const		  = 0;
				virtual void	 SetBaiduPanCookies(QString value)  = 0;

				// 端口与 RPC
				virtual int GetListenPort() const				  = 0;
				virtual void SetListenPort(int value)			  = 0;
				virtual int GetRpcListenPort() const			  = 0;
				virtual void SetRpcListenPort(int value)		  = 0;
				virtual QString GetRpcSecret() const			  = 0;
				virtual void	 SetRpcSecret(QString value)		  = 0;
				virtual int GetDhtListenPort() const			  = 0;
				virtual void SetDhtListenPort(int value)		  = 0;

				// 分片与并发
				virtual int GetSplit() const					  = 0;
				virtual void SetSplit(int value)				  = 0;
				virtual int GetMaxConcurrentDownloads() const	  = 0;
				virtual void SetMaxConcurrentDownloads(int value) = 0;
				virtual int GetMaxConnectionPerServer() const	  = 0;
				virtual void SetMaxConnectionPerServer(int value) = 0;
				virtual int GetMinSplitSize() const			  = 0;
				virtual void SetMinSplitSize(int value)		  = 0;

				// 速度限制
				virtual int GetMaxDownloadLimit() const		  = 0;
				virtual void SetMaxDownloadLimit(int value)	  = 0;
				virtual int GetMaxOverallDownloadLimit() const	  = 0;
				virtual void SetMaxOverallDownloadLimit(int value) = 0;
				virtual int GetMaxUploadLimit() const			  = 0;
				virtual void SetMaxUploadLimit(int value)		  = 0;
				virtual int GetMaxOverallUploadLimit() const	  = 0;
				virtual void SetMaxOverallUploadLimit(int value) = 0;
				virtual int GetLowestSpeedLimit() const		  = 0;
				virtual void SetLowestSpeedLimit(int value)	  = 0;

				// 超时与重试
				virtual int GetTimeout() const					  = 0;
				virtual void SetTimeout(int value)				  = 0;
				virtual int GetConnectTimeout() const			  = 0;
				virtual void SetConnectTimeout(int value)		  = 0;
				virtual int GetMaxTries() const				  = 0;
				virtual void SetMaxTries(int value)			  = 0;
				virtual int GetRetryWait() const				  = 0;
				virtual void SetRetryWait(int value)			  = 0;

				// 任务完成/错误/开始动作
				virtual int GetOnCompleteAction() const		  = 0;
				virtual void SetOnCompleteAction(int value)	  = 0;
				virtual QString GetCustomCompleteCommand() const  = 0;
				virtual void	 SetCustomCompleteCommand(QString value) = 0;
				virtual int GetOnErrorAction() const			  = 0;
				virtual void SetOnErrorAction(int value)		  = 0;
				virtual QString GetCustomErrorCommand() const	  = 0;
				virtual void	 SetCustomErrorCommand(QString value) = 0;
				virtual int GetOnStartAction() const			  = 0;
				virtual void SetOnStartAction(int value)		  = 0;

				// BitTorrent
				virtual int	 GetBtMaxPeers() const			  = 0;
				virtual void SetBtMaxPeers(int value)			  = 0;
				virtual bool GetEnableDht() const				  = 0;
				virtual void SetEnableDht(bool value)			  = 0;
				virtual bool GetBtRequireCrypto() const		  = 0;
				virtual void SetBtRequireCrypto(bool value)	  = 0;

				// 开关类配置
				virtual bool GetIsSaveSession() const			  = 0;
				virtual void SetIsSaveSession(bool value)		  = 0;
				virtual bool GetEnableGlobalProxy() const		  = 0;
				virtual void SetEnableGlobalProxy(bool value)	  = 0;
				virtual bool GetListenClipboard() const		  = 0;
				virtual void SetListenClipboard(bool value)	  = 0;
				virtual bool GetAutoResumeTask() const			  = 0;
				virtual void SetAutoResumeTask(bool value)		  = 0;
				virtual bool GetAutoStart() const				  = 0;
				virtual void SetAutoStart(bool value)			  = 0;
				virtual bool GetRememberWindowPosition() const	  = 0;
				virtual void SetRememberWindowPosition(bool value) = 0;
				virtual bool GetEnableTrayIcon() const			  = 0;
				virtual void SetEnableTrayIcon(bool value)		  = 0;
				virtual bool GetEnableNotification() const		  = 0;
				virtual void SetEnableNotification(bool value)	  = 0;
				virtual bool GetEnableAutoShutdown() const		  = 0;
				virtual void SetEnableAutoShutdown(bool value)	  = 0;
				virtual bool GetEnableAutoUpdate() const		  = 0;
				virtual void SetEnableAutoUpdate(bool value)	  = 0;
				virtual bool GetEnableGithubAccelerate() const	  = 0;
				virtual void SetEnableGithubAccelerate(bool value) = 0;
				virtual bool GetShowCloseConfirm() const		  = 0;
				virtual void SetShowCloseConfirm(bool value)	  = 0;
				virtual bool GetCloseToTray() const			  = 0;
				virtual void SetCloseToTray(bool value)		  = 0;

				// Tracker 源
				virtual QString GetTrackerSourceUrls() const	  = 0;
				virtual void	 SetTrackerSourceUrls(QString value) = 0;
				virtual QString GetTrackerSourceNames() const	  = 0;
				virtual void	 SetTrackerSourceNames(QString value) = 0;
				virtual bool GetEnableTrackerSourceAutoUpdate() const = 0;
				virtual void SetEnableTrackerSourceAutoUpdate(bool value) = 0;

				// 插件源代理(追加在接口末尾:新增虚函数不移动已有 vtable 索引,避免增量构建下的错位调度)
				virtual QString GetPluginSourceProxy() const	  = 0;
				virtual void	 SetPluginSourceProxy(QString value) = 0;

				// eD2k 引擎设置(追加在接口末尾:新增虚函数不移动已有 vtable 索引,避免增量构建下的错位调度)
				virtual QString GetEd2kNickname() const		  = 0;
				virtual void	 SetEd2kNickname(QString value)	  = 0;
				virtual int  GetEd2kTcpPort() const			  = 0;
				virtual void SetEd2kTcpPort(int value)			  = 0;
				virtual int  GetEd2kUdpPort() const			  = 0;
				virtual void SetEd2kUdpPort(int value)			  = 0;
				virtual bool GetEd2kEnableKad() const			  = 0;
				virtual void SetEd2kEnableKad(bool value)		  = 0;
				virtual bool GetEd2kEnableObfuscation() const	  = 0;
				virtual void SetEd2kEnableObfuscation(bool value) = 0;
				virtual bool GetEd2kAutoConnect() const		  = 0;
				virtual void SetEd2kAutoConnect(bool value)	  = 0;
				virtual int  GetEd2kMaxConcurrentTasks() const	  = 0;
				virtual void SetEd2kMaxConcurrentTasks(int value) = 0;
				virtual QString GetEd2kSharedDirs() const		  = 0;
				virtual void	 SetEd2kSharedDirs(QString value)  = 0;
				virtual QString GetEd2kServerMetUrl() const	  = 0;
				virtual void	 SetEd2kServerMetUrl(QString value) = 0;
			};

		}  // namespace settings
	}	   // namespace ui
}		   // namespace gdl
