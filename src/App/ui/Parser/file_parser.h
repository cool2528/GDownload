#pragma once
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

namespace gdl {
    namespace ui {
        namespace parser {

            // 文件信息结构体
            struct FileInfo {
                QString file_name;
                QString file_path;
                qint64 file_size{0};
                QString file_hash;	// SHA1/MD5等
                QString mime_type;
            };

            // BT种子文件信息
            struct TorrentInfo {
                QString name;
                QString comment;
                QString created_by;
                QDateTime creation_date;
                QString announce;
                QVector<QString> announce_list;
                qint64 total_size{0};
                QString info_hash;
                QVector<FileInfo> files;
                bool is_private{false};
                QString encoding;
            };

            // Metalink文件信息
            struct MetalinkInfo {
                QString identity;
                QString version;
                QString description;
                QDateTime published;
                QString publisher_name;
                QString publisher_url;
                QVector<QString> tags;
                QVector<FileInfo> files;
                QVector<QString> mirrors;
            };

            // 文件解析器基类
            class FileParserBase {
               public:
                virtual ~FileParserBase()					 = default;
                virtual bool Parse(const QString& file_path) = 0;
            };

            // Torrent解析器
            class TorrentParser : public FileParserBase {
               public:
                bool Parse(const QString& file_path) override;
                const TorrentInfo& GetTorrentInfo() const { return torrent_info_; }

               private:
                TorrentInfo torrent_info_;
                bool ParseBencodedData(const QByteArray& data);
            };

            // Metalink解析器
            class MetalinkParser : public FileParserBase {
               public:
                bool Parse(const QString& file_path) override;
                const MetalinkInfo& GetMetalinkInfo() const { return metalink_info_; }

               private:
                MetalinkInfo metalink_info_;
                bool ParseXmlData(const QByteArray& data);
                bool IsMeta4Format(const QByteArray& data) const;
            };

        }  // namespace parser
    }  // namespace ui
}  // namespace gdl
