#pragma once
#include <QMutex>
#include <QObject>
#include <QQmlEngine>
#include <QThread>
#include <QWaitCondition>
#include <queue>
#include "GDLCore/singleton.hpp"
#include "NetWork_disk_model.h"
namespace gdl {
    namespace ui {

        namespace netdisk {
            enum class NetDiskTaskType {
                ParseShareUrl,
                EnterDirectory,
                GetDownloadInfo,
            };
            class NetDiskTask {
               public:
                NetDiskTaskType type;
                explicit NetDiskTask(NetDiskTaskType t) : type(t) {}
                virtual ~NetDiskTask() = default;
            };
            class ParseShareUrlTask : public NetDiskTask {
               public:
                QString url;
                QString userToken;
                ParseShareUrlTask(const QString& u, const QString& token)
                    : NetDiskTask(NetDiskTaskType::ParseShareUrl), url(u), userToken(token) {}
            };

            class EnterDirectoryTask : public NetDiskTask {
               public:
                QString fileId;
                QString path;

                EnterDirectoryTask(const QString& id, const QString& p)
                    : NetDiskTask(NetDiskTaskType::EnterDirectory), fileId(id), path(p) {}
            };
            class GetDownloadInfoTask : public NetDiskTask {
               public:
                std::vector<INetDiskDownloadPlugin::FileInfo> files;
                explicit GetDownloadInfoTask(const std::vector<INetDiskDownloadPlugin::FileInfo>& ids)
                    : NetDiskTask(NetDiskTaskType::GetDownloadInfo), files(ids) {}
            };

            class TaskResult {
               public:
                bool success;
                QString message;
                NetDiskTaskType taskType;

                TaskResult(bool s, const QString& msg, NetDiskTaskType type)
                    : success(s), message(msg), taskType(type) {}
                virtual ~TaskResult() = default;
            };

            class ParseShareUrlResult : public TaskResult {
               public:
                std::vector<INetDiskDownloadPlugin::FileInfo> files;

                ParseShareUrlResult(bool s, const QString& msg, const std::vector<INetDiskDownloadPlugin::FileInfo>& f)
                    : TaskResult(s, msg, NetDiskTaskType::ParseShareUrl), files(f) {}
            };

            class EnterDirectoryResult : public TaskResult {
               public:
                std::vector<INetDiskDownloadPlugin::FileInfo> files;
                QString path;

                EnterDirectoryResult(bool s, const QString& msg, const std::vector<INetDiskDownloadPlugin::FileInfo>& f,
                                     const QString& p)
                    : TaskResult(s, msg, NetDiskTaskType::EnterDirectory), files(f), path(p) {}
            };

            class GetDownloadInfoResult : public TaskResult {
               public:
                std::vector<INetDiskDownloadPlugin::ParseResult> downloadInfo;

                GetDownloadInfoResult(bool s, const QString& msg,
                                      const std::vector<INetDiskDownloadPlugin::ParseResult>& info)
                    : TaskResult(s, msg, NetDiskTaskType::GetDownloadInfo), downloadInfo(info) {}
            };

            class AsyncTaskWorker : public QThread {
                Q_OBJECT
               public:
                explicit AsyncTaskWorker(QObject* parent = nullptr);
                ~AsyncTaskWorker() override;
                void AddTask(std::shared_ptr<NetDiskTask> task);
                void Stop();

               public:
               Q_SIGNALS:
                void taskCompleted(std::shared_ptr<TaskResult> result);

               protected:
                void run() override;

               private:
                std::shared_ptr<TaskResult> ExecuteTask(std::shared_ptr<NetDiskTask> task);
                QMutex mutex_;
                QWaitCondition condition_;
                bool stopped_{false};
                std::queue<std::shared_ptr<NetDiskTask>> task_queue_;
            };

            class NetWorkDiskManager : public QObject, public Singleton<NetWorkDiskManager> {
                Q_OBJECT
                SINGLETON_DECLARE(NetWorkDiskManager)
               public:
                ~NetWorkDiskManager() override;
                Q_INVOKABLE NetWorkDiskModel* GetNetWorkDiskModel();
                // 解析分享链接
                Q_INVOKABLE void ParseShareUrl(const QString& url);

                // 切换目录
                Q_INVOKABLE void ChangeDir(const QString& path, const QString& id);

                // 选择所有
                Q_INVOKABLE void SelectAll();
                // 取消选择所有
                Q_INVOKABLE void UnselectAll();
                // 选择文件
                Q_INVOKABLE void ToggleSelection(int index, bool is_selected);

                // 下载选中的文件
                Q_INVOKABLE void DownloadSelectedFiles();

               public:
               Q_SIGNALS:
                void taskFinished(const QString& msg, bool success, int type);

               private:
                explicit NetWorkDiskManager(QObject* parent = nullptr);
                void HandleTaskResult(std::shared_ptr<TaskResult> result);

               private:
                std::unique_ptr<NetWorkDiskModel> model_{nullptr};
                AsyncTaskWorker worker_;
            };
            void RegisterTypes(QQmlEngine* engine);
        }  // namespace netdisk
    }  // namespace ui
}  // namespace gdl
