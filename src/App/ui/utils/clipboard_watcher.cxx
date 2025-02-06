#include "clipboard_watcher.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QRegularExpression>
#include "Settings/settings_manager.h"
namespace gdl {
    namespace ui {
        namespace utils {

            ClipboardWatcher::~ClipboardWatcher() {
                disconnect(qApp->clipboard(), &QClipboard::dataChanged, this, &ClipboardWatcher::clipboardChangedSlot);
            }

            QString ClipboardWatcher::GetClipboardText() const {
                return clipboard_text_;
            }

            ClipboardWatcher::ClipboardWatcher(QObject* parent) {
                connect(qApp->clipboard(), &QClipboard::dataChanged, this, &ClipboardWatcher::clipboardChangedSlot);
            }

            void ClipboardWatcher::clipboardChangedSlot() {
                const auto is_listen_clipborad = gdl::ui::settings::Settings::Instance().GetListenClipboard();
                const QMimeData* mimeData	   = qApp->clipboard()->mimeData();
                if (!mimeData || !is_listen_clipborad) return;

                QStringList urls;

                // 解析HTML内容中的链接
                if (mimeData->hasHtml()) {
                    QString html = mimeData->html();
                    QRegularExpression hrefRegex(R"(href\s*=\s*["']?([^"'\s>]+))",
                                                 QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatchIterator it = hrefRegex.globalMatch(html);
                    while (it.hasNext()) {
                        QRegularExpressionMatch match = it.next();
                        QString url					  = match.captured(1);
                        urls.append(url);
                    }
                }

                // 解析纯文本中的链接
                if (mimeData->hasText()) {
                    QString text = mimeData->text();
                    QRegularExpression urlRegex(
                        R"((https?|ftp|magnet|torrent):\/\/[^\s/$.?#]+\.[^\s]*|magnet:\?xt=urn:[^\s]+|torrent:\/\/[^\s]+))",
                        QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatchIterator it = urlRegex.globalMatch(text);
                    while (it.hasNext()) {
                        QRegularExpressionMatch match = it.next();
                        QString url					  = match.captured(0);
                        urls.append(url);
                    }
                }

                // 去重
                urls = QSet<QString>(urls.begin(), urls.end()).values();
                clipboard_text_.clear();
                if (!urls.isEmpty()) {
                    clipboard_text_ = urls.join("\n");
                }
                if (is_listen_clipborad) Q_EMIT clipboardChanged(clipboard_text_);
            }

        }  // namespace utils
    }  // namespace ui
}  // namespace gdl
