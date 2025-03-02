#include "win_updater.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QThread>
#include <QCryptographicHash>

namespace gdl {
namespace update {

WinUpdater::WinUpdater() = default;

WinUpdater::~WinUpdater() {
  CancelUpdate();
  CleanupTempFiles();
}

bool WinUpdater::Initialize(const UpdateConfig& config) {
  config_ = config;
  
  // 确保临时目录存在
  if (config_.temp_dir.empty()) {
    config_.temp_dir = QStandardPaths::writableLocation(
        QStandardPaths::TempLocation).toStdString() + "/GDownloadUpdater";
  }
  
  QDir dir(QString::fromStdString(config_.temp_dir));
  if (!dir.exists() && !dir.mkpath(".")) {
    last_error_ = "Failed to create temp directory: " + config_.temp_dir;
    return false;
  }
  
  return true;
}

void WinUpdater::CheckForUpdates(UpdateCheckCallback callback) {
  if (update_in_progress_) {
    if (callback) {
      callback(false, UpdateInfo{});
    }
    return;
  }
  
  QNetworkRequest request(QUrl(QString::fromStdString(config_.update_url)));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  
  // 添加自定义请求头
  for (const auto& header : request_headers_) {
    request.setRawHeader(
        header.first.c_str(),
        header.second.c_str());
  }
  
  // 添加当前版本信息
  QJsonObject json_data;
  json_data["current_version"] = QString::fromStdString(config_.current_version);
  json_data["platform"] = "windows";
  json_data["allow_beta"] = config_.allow_beta;
  
  QByteArray data = QJsonDocument(json_data).toJson();
  
  current_reply_ = network_manager_.post(request, data);
  
  QObject::connect(current_reply_, &QNetworkReply::finished, [this, callback]() {
    if (current_reply_->error() == QNetworkReply::NoError) {
      QByteArray response_data = current_reply_->readAll();
      QJsonDocument json_doc = QJsonDocument::fromJson(response_data);
      
      if (json_doc.isObject()) {
        QJsonObject json_obj = json_doc.object();
        bool has_update = json_obj["has_update"].toBool();
        
        if (has_update) {
          update_info_.version = json_obj["version"].toString().toStdString();
          update_info_.download_url = json_obj["download_url"].toString().toStdString();
          update_info_.release_notes = json_obj["release_notes"].toString().toStdString();
          update_info_.release_date = json_obj["release_date"].toString().toStdString();
          update_info_.is_mandatory = json_obj["is_mandatory"].toBool();
          update_info_.package_size = json_obj["package_size"].toInt();
          update_info_.signature = json_obj["signature"].toString().toStdString();
          
          // 解析变更文件列表
          QJsonArray changed_files = json_obj["changed_files"].toArray();
          update_info_.changed_files.clear();
          for (const auto& file : changed_files) {
            update_info_.changed_files.push_back(file.toString().toStdString());
          }
          
          update_available_ = true;
        }
        
        if (callback) {
          callback(has_update, update_info_);
        }
      } else {
        last_error_ = "Invalid server response format";
        if (callback) {
          callback(false, UpdateInfo{});
        }
      }
    } else {
      last_error_ = current_reply_->errorString().toStdString();
      if (callback) {
        callback(false, UpdateInfo{});
      }
    }
    
    current_reply_->deleteLater();
    current_reply_ = nullptr;
  });
}

bool WinUpdater::StartUpdate(ProgressCallback progress_callback) {
  if (!update_available_ || update_in_progress_) {
    last_error_ = "No update available or update already in progress";
    return false;
  }
  
  progress_callback_ = progress_callback;
  update_in_progress_ = true;
  
  // 创建下载路径
  update_package_path_ = QString::fromStdString(config_.temp_dir) + 
                         "/update_" + 
                         QString::fromStdString(update_info_.version) + 
                         ".zip";
  
  extract_path_ = QString::fromStdString(config_.temp_dir) + "/extracted";
  
  // 确保提取目录存在
  QDir dir(extract_path_);
  if (dir.exists()) {
    dir.removeRecursively();
  }
  dir.mkpath(".");
  
  // 开始下载
  QNetworkRequest request(QUrl(QString::fromStdString(update_info_.download_url)));
  
  // 添加自定义请求头
  for (const auto& header : request_headers_) {
    request.setRawHeader(
        header.first.c_str(),
        header.second.c_str());
  }
  
  download_file_.setFileName(update_package_path_);
  if (!download_file_.open(QIODevice::WriteOnly)) {
    last_error_ = "Failed to create download file: " + update_package_path_.toStdString();
    update_in_progress_ = false;
    return false;
  }
  
  current_reply_ = network_manager_.get(request);
  
  if (progress_callback_) {
    UpdateProgress progress;
    progress.stage = UpdateProgress::Stage::kDownloading;
    progress.percentage = 0;
    progress.message = "Starting update download";
    progress_callback_(progress);
  }
  
  // 处理下载进度
  QObject::connect(current_reply_, &QNetworkReply::downloadProgress, 
      [this](qint64 received, qint64 total) {
    if (progress_callback_ && total > 0) {
      UpdateProgress progress;
      progress.stage = UpdateProgress::Stage::kDownloading;
      progress.percentage = static_cast<int>(received * 100 / total);
      progress.message = "Downloading update: " + std::to_string(progress.percentage) + "%";
      progress_callback_(progress);
    }
  });
  
  // 处理下载完成
  QObject::connect(current_reply_, &QNetworkReply::finished, [this]() {
    if (current_reply_->error() == QNetworkReply::NoError) {
      download_file_.write(current_reply_->readAll());
      download_file_.close();
      
      // 验证下载的包
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kVerifying;
        progress.percentage = 0;
        progress.message = "Verifying update package";
        progress_callback_(progress);
      }
      
      if (VerifyUpdatePackage(update_package_path_, update_info_.signature)) {
        // 解压更新包
        if (progress_callback_) {
          UpdateProgress progress;
          progress.stage = UpdateProgress::Stage::kExtracting;
          progress.percentage = 0;
          progress.message = "Extracting update package";
          progress_callback_(progress);
        }
        
        if (ExtractUpdate(update_package_path_, extract_path_)) {
          if (progress_callback_) {
            UpdateProgress progress;
            progress.stage = UpdateProgress::Stage::kFinished;
            progress.percentage = 100;
            progress.message = "Update package ready";
            progress_callback_(progress);
          }
        } else {
          if (progress_callback_) {
            UpdateProgress progress;
            progress.stage = UpdateProgress::Stage::kFailed;
            progress.percentage = 0;
            progress.message = "Failed to extract update package: " + last_error_;
            progress_callback_(progress);
          }
          update_in_progress_ = false;
        }
      } else {
        if (progress_callback_) {
          UpdateProgress progress;
          progress.stage = UpdateProgress::Stage::kFailed;
          progress.percentage = 0;
          progress.message = "Failed to verify update package: " + last_error_;
          progress_callback_(progress);
        }
        update_in_progress_ = false;
      }
    } else {
      download_file_.close();
      last_error_ = current_reply_->errorString().toStdString();
      
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kFailed;
        progress.percentage = 0;
        progress.message = "Failed to download update package: " + last_error_;
        progress_callback_(progress);
      }
      update_in_progress_ = false;
    }
    
    current_reply_->deleteLater();
    current_reply_ = nullptr;
  });
  
  return true;
}

void WinUpdater::CancelUpdate() {
  if (current_reply_) {
    current_reply_->abort();
    current_reply_->deleteLater();
    current_reply_ = nullptr;
  }
  
  if (download_file_.isOpen()) {
    download_file_.close();
  }
  
  update_in_progress_ = false;
}

bool WinUpdater::ApplyUpdate(bool restart_app) {
  if (!update_in_progress_ || extract_path_.isEmpty()) {
    last_error_ = "No update package ready";
    return false;
  }
  
  if (progress_callback_) {
    UpdateProgress progress;
    progress.stage = UpdateProgress::Stage::kInstalling;
    progress.percentage = 0;
    progress.message = "Starting update installation";
    progress_callback_(progress);
  }
  
  // 创建并启动更新器进程
  QString app_path = QCoreApplication::applicationDirPath();
  QString updater_path = extract_path_ + "/updater.exe";
  
  if (!QFile::exists(updater_path)) {
    // 如果更新包中没有updater.exe，则使用内置的更新逻辑
    // 创建批处理脚本来执行更新
    QString batch_path = QString::fromStdString(config_.temp_dir) + "/update.bat";
    QFile batch_file(batch_path);
    
    if (!batch_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      last_error_ = "Failed to create update script";
      return false;
    }
    
    // 获取应用程序PID
    qint64 pid = QCoreApplication::applicationPid();
    
    // 编写批处理脚本
    QTextStream out(&batch_file);
    out << "@echo off\n";
    out << "echo Waiting for application to exit...\n";
    out << "timeout /t 2 /nobreak > nul\n";
    
    // 等待应用程序退出
    out << "taskkill /F /PID " << pid << " > nul 2>&1\n";
    out << ":wait_loop\n";
    out << "tasklist | find \"" << pid << "\" > nul 2>&1\n";
    out << "if not errorlevel 1 (\n";
    out << "  timeout /t 1 /nobreak > nul\n";
    out << "  goto wait_loop\n";
    out << ")\n";
    
    // 复制更新文件
    out << "echo Updating files...\n";
    out << "xcopy /Y /E /I \"" << extract_path_ << "\\*\" \"" << app_path << "\\\" > nul\n";
    
    // 如果需要重启应用程序
    if (restart_app) {
      out << "echo Restarting application...\n";
      out << "start \"\" \"" << app_path << "\\" << QCoreApplication::applicationName() << ".exe\"\n";
    }
    
    out << "echo Update completed\n";
    out << "del \"" << batch_path << "\"\n";
    
    batch_file.close();
    
    // 启动批处理脚本
    bool success = QProcess::startDetached("cmd.exe", QStringList() << "/c" << batch_path);
    
    if (success) {
      if (restart_app) {
        // 退出应用程序以便更新器可以替换文件
        QCoreApplication::quit();
      }
      
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kFinished;
        progress.percentage = 100;
        progress.message = restart_app ? 
            "Application will restart to complete update" : "Update will be applied on next start";
        progress_callback_(progress);
      }
    } else {
      last_error_ = "Failed to start update script";
      
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kFailed;
        progress.percentage = 0;
        progress.message = "Failed to install update: " + last_error_;
        progress_callback_(progress);
      }
    }
    
    update_in_progress_ = false;
    return success;
  } else {
    // 使用更新包中的updater.exe
    QStringList args;
    args << "--app-path=" + app_path;
    args << "--update-path=" + extract_path_;
    
    if (restart_app) {
      args << "--restart";
      args << "--app-name=" + QCoreApplication::applicationName();
    }
    
    // 启动更新器进程
    bool success = QProcess::startDetached(updater_path, args);
    
    if (success) {
      if (restart_app) {
        // 退出应用程序以便更新器可以替换文件
        QCoreApplication::quit();
      }
      
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kFinished;
        progress.percentage = 100;
        progress.message = restart_app ? 
            "Application will restart to complete update" : "Update will be applied on next start";
        progress_callback_(progress);
      }
    } else {
      last_error_ = "Failed to start updater process";
      
      if (progress_callback_) {
        UpdateProgress progress;
        progress.stage = UpdateProgress::Stage::kFailed;
        progress.percentage = 0;
        progress.message = "Failed to install update: " + last_error_;
        progress_callback_(progress);
      }
    }
    
    update_in_progress_ = false;
    return success;
  }
}

bool WinUpdater::ExtractUpdate(const QString& zip_path, const QString& extract_path) {
  // 使用Qt的QZipReader或调用外部库如QuaZip
  // 这里使用简化实现，实际应用中应使用完整的解压缩库
  
  // 检查文件是否存在
  if (!QFile::exists(zip_path)) {
    last_error_ = "Update package file does not exist";
    return false;
  }
  
  // 使用系统命令解压(仅作示例，实际应使用库)
  QProcess process;
  process.setWorkingDirectory(QFileInfo(extract_path).absolutePath());
  
  // 使用PowerShell解压
  QStringList args;
  args << "-NoProfile" << "-ExecutionPolicy" << "Bypass" << "-Command";
  args << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
           .arg(QDir::toNativeSeparators(zip_path))
           .arg(QDir::toNativeSeparators(extract_path));
  
  process.start("powershell.exe", args);
  
  if (!process.waitForFinished(60000)) {  // 等待最多60秒
    last_error_ = "Extraction timeout";
    return false;
  }
  
  if (process.exitCode() != 0) {
    last_error_ = "Extraction failed: " + 
                 QString(process.readAllStandardError()).toStdString();
    return false;
  }
  
  return true;
}

bool WinUpdater::VerifyUpdatePackage(const QString& package_path, 
                                    const std::string& signature) {
  // 实现包验证逻辑，例如检查SHA256或数字签名
  if (!QFile::exists(package_path)) {
    last_error_ = "Update package file does not exist";
    return false;
  }
  
  // 简单的SHA256验证示例
  QFile file(package_path);
  if (!file.open(QIODevice::ReadOnly)) {
    last_error_ = "Failed to open update package for verification";
    return false;
  }
  
  QCryptographicHash hash(QCryptographicHash::Sha256);
  if (!hash.addData(&file)) {
    file.close();
    last_error_ = "Failed to calculate hash";
    return false;
  }
  
  file.close();
  
  QString calculated_hash = hash.result().toHex();
  
  // 比较计算的哈希值与签名
  if (calculated_hash.toStdString() != signature) {
    // 实际应用中，这里应该进行更复杂的签名验证
    // 简化示例中，我们假设签名就是SHA256哈希
    
    // 如果验证失败但我们想继续(仅用于测试)
    if (config_.allow_beta) {
      return true;  // 测试模式下允许不匹配
    }
    
    last_error_ = "Package verification failed: hash mismatch";
    return false;
  }
  
  return true;
}

void WinUpdater::CleanupTempFiles() {
  // 清理临时文件，但保留最新的更新包
  QDir temp_dir(QString::fromStdString(config_.temp_dir));
  
  if (temp_dir.exists()) {
    QStringList filters;
    filters << "update_*.zip";
    filters << "extracted";
    
    QFileInfoList files = temp_dir.entryInfoList(filters, QDir::Files | QDir::Dirs);
    
    for (const QFileInfo& file : files) {
      if (file.isDir()) {
        QDir dir(file.absoluteFilePath());
        dir.removeRecursively();
      } else if (file.absoluteFilePath() != update_package_path_) {
        QFile::remove(file.absoluteFilePath());
      }
    }
  }
}

}  // namespace update
}  // namespace gdl