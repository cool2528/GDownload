#include "clipboard_watcher.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QSet>
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

            ClipboardWatcher::ClipboardWatcher(QObject* parent)
                : QObject(parent) {
                connect(qApp->clipboard(), &QClipboard::dataChanged, this, &ClipboardWatcher::clipboardChangedSlot);
                clipboardChangedSlot();
            }

            void ClipboardWatcher::clipboardChangedSlot() {
                const auto listen_clipboard_enabled = gdl::ui::settings::Settings::Instance().GetListenClipboard();
                const QMimeData* mime_data		  = qApp->clipboard()->mimeData();
                if (!mime_data || !listen_clipboard_enabled) return;

                QStringList collected_urls;
                static const QRegularExpression url_regex(
                    R"((?:(?:https?|ftps?|sftp|ftp|torrent)://[^\s"'<>]+)|(?:magnet:\?xt=urn:[^\s"'<>]+))",
                    QRegularExpression::CaseInsensitiveOption);

                // 解析纯文本中的链接，优先处理完整单行 URL
                if (mime_data->hasText()) {
                    const QString text	  = mime_data->text();
                    const QString trimmed = text.trimmed();

                    bool matched_whole_text = false;
                    if (!trimmed.isEmpty()) {
                        const QRegularExpressionMatch whole_match = url_regex.match(trimmed);
                        if (whole_match.hasMatch() && whole_match.capturedStart() == 0 &&
                            whole_match.capturedLength() == trimmed.length()) {
                            collected_urls.append(trimmed);
                            matched_whole_text = true;
                        }
                    }

                    if (!matched_whole_text) {
                        QRegularExpressionMatchIterator it = url_regex.globalMatch(text);
                        while (it.hasNext()) {
                            const QString url = it.next().captured(0);
                            if (!url.isEmpty()) {
                                collected_urls.append(url);
                            }
                        }
                    }
                }

                // 若纯文本未提取到有效链接，再尝试解析 HTML 内容中的 href
                if (collected_urls.isEmpty() && mime_data->hasHtml()) {
                    const QString html = mime_data->html();
                    QRegularExpression href_regex(R"(href\s*=\s*["']?([^"'#\s>]+))",
                                                  QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatchIterator it = href_regex.globalMatch(html);
                    while (it.hasNext()) {
                        const QString url = it.next().captured(1);
                        if (url.isEmpty()) continue;
                        if (url.startsWith(QLatin1String("javascript:"), Qt::CaseInsensitive) ||
                            url.startsWith(QLatin1String("mailto:"), Qt::CaseInsensitive) ||
                            url.startsWith(QLatin1Char('#'))) {
                            continue;
                        }
                        if (url_regex.match(url).hasMatch()) {
                            collected_urls.append(url);
                        }
                    }
                }

                QStringList unique_urls;
                QSet<QString> seen;
                for (const QString& raw_url : collected_urls) {
                    const QString normalized = raw_url.trimmed();
                    if (normalized.isEmpty() || seen.contains(normalized)) continue;
                    seen.insert(normalized);
                    unique_urls.append(normalized);
                }

                clipboard_text_.clear();
                if (!unique_urls.isEmpty()) {
                    clipboard_text_ = unique_urls.join("\n");
                }
                Q_EMIT clipboardChanged(clipboard_text_);
            }

        }  // namespace utils
    }  // namespace ui
}  // namespace gdl
