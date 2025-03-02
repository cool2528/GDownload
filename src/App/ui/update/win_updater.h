#ifndef WIN_UPDATER_H_
#define WIN_UPDATER_H_

#include "auto_updater.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>
#include <memory>

namespace gdownload {
namespace update {

class WinUpdater : public AutoUpdater {
 public:
  WinUpdater();
  ~WinUpdater() override;

  bool Initialize(const UpdateConfig& config) override;
  void CheckForUpdates(UpdateCheckCallback callback) override;
  bool StartUpdate(ProgressCallback progress_callback) override;
  void CancelUpdate() override;
  bool ApplyUpdate(bool restart_app = true) override;

 private:
  bool ExtractUpdate(const QString& zip_path, const QString& extract_path);
  bool VerifyUpdatePackage(const QString& package_path, const std::string& signature);
  void CleanupTempFiles();
  
  UpdateConfig config_;
  UpdateInfo update_info_;
  ProgressCallback progress_callback_;
  QNetworkAccessManager network_manager_;
  QNetworkReply* current_reply_ = nullptr;
  QFile download_file_;
  QString update_package_path_;
  QString extract_path_;
  bool update_available_ = false;
  bool update_in_progress_ = false;
};

}  // namespace update
}  // namespace gdownload

#endif  // WIN_UPDATER_H_