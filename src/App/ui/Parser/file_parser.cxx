#include "file_parser.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/torrent_info.hpp>
#include <pugixml.hpp>
#include "logger.h"

namespace gdl {
    namespace ui {
        namespace parser {

            bool TorrentParser::Parse(const QString& file_path) {
                QFile file(file_path);
                if (!file.open(QIODevice::ReadOnly)) {
                    LOG_ERR("Failed to open torrent file: {}", file_path.toStdString());
                    return false;
                }

                QByteArray data = file.readAll();
                file.close();

                return ParseBencodedData(data);
            }

            bool TorrentParser::ParseBencodedData(const QByteArray& data) {
                try {
                    lt::error_code ec;
                    lt::bdecode_node root;
                    lt::bdecode(data.constData(), data.constData() + data.size(), root, ec);

                    if (ec) {
                        LOG_ERR("Failed to decode torrent data: {}", ec.message());
                        return false;
                    }

                    // 创建torrent_info对象
                    std::shared_ptr<lt::torrent_info> ti = std::make_shared<lt::torrent_info>(root, ec);
                    if (ec) {
                        LOG_ERR("Failed to parse torrent info: {}", ec.message());
                        return false;
                    }

                    // 填充基本信息
                    torrent_info_.name			= QString::fromStdString(ti->name());
                    torrent_info_.total_size	= ti->total_size();
                    torrent_info_.creation_date = QDateTime::fromSecsSinceEpoch(ti->creation_date());
                    torrent_info_.info_hash		= QString::fromStdString(ti->info_hash().to_string());

                    // 获取comment和created_by
                    if (auto comment_node = root.dict_find("comment")) {
                        auto comment_sv = comment_node.string_value();
                        torrent_info_.comment =
                            QString::fromStdString(std::string(comment_sv.data(), comment_sv.size()));
                    }
                    if (auto created_by_node = root.dict_find("created by")) {
                        auto created_by_sv = created_by_node.string_value();
                        torrent_info_.created_by =
                            QString::fromStdString(std::string(created_by_sv.data(), created_by_sv.size()));
                    }

                    // 获取announce信息
                    std::vector<lt::announce_entry> const& trackers = ti->trackers();
                    if (!trackers.empty()) {
                        torrent_info_.announce = QString::fromStdString(trackers[0].url);
                    }
                    for (auto const& entry : trackers) {
                        torrent_info_.announce_list.append(QString::fromStdString(entry.url));
                    }

                    // 获取文件列表
                    for (lt::file_index_t i(0); i < ti->files().end_file(); ++i) {
                        FileInfo file_info;
                        file_info.file_name = QString::fromStdString(ti->files().file_name(i).to_string());
                        file_info.file_path = QString::fromStdString(ti->files().file_path(i));
                        file_info.file_size = ti->files().file_size(i);

                        // 获取文件hash(如果有)
                        lt::sha1_hash file_hash = ti->files().hash(i);
                        if (file_hash != lt::sha1_hash{}) {
                            file_info.file_hash = QString::fromStdString(file_hash.to_string());
                        }

                        torrent_info_.files.append(file_info);
                    }

                    // 获取private标志
                    if (auto info_node = root.dict_find("info")) {
                        if (auto private_node = info_node.dict_find("private")) {
                            torrent_info_.is_private = private_node.int_value() == 1;
                        }
                    }

                    // 获取编码信息
                    if (auto encoding_node = root.dict_find("encoding")) {
                        auto encoding_node_sv = encoding_node.string_value();
                        torrent_info_.encoding =
                            QString::fromStdString(std::string(encoding_node_sv.data(), encoding_node_sv.size()));
                    }

                    return true;
                } catch (const std::exception& e) {
                    LOG_ERR("Failed to parse torrent data: {}", e.what());
                    return false;
                }
            }

            bool MetalinkParser::Parse(const QString& file_path) {
                QFile file(file_path);
                if (!file.open(QIODevice::ReadOnly)) {
                    LOG_ERR("Failed to open metalink file: {}", file_path.toStdString());
                    return false;
                }

                QByteArray data = file.readAll();
                file.close();

                return ParseXmlData(data);
            }

            bool MetalinkParser::ParseXmlData(const QByteArray& data) {
                try {
                    pugi::xml_document doc;
                    pugi::xml_parse_result result = doc.load_buffer(data.constData(), data.size());

                    if (!result) {
                        LOG_ERR("Failed to parse XML: {}", result.description());
                        return false;
                    }

                    bool is_meta4		= IsMeta4Format(data);
                    pugi::xml_node root = is_meta4 ? doc.child("metalink") : doc.child("metalink:metalink");

                    if (!root) {
                        LOG_ERR("Invalid metalink format: root node not found");
                        return false;
                    }

                    // 解析基本信息
                    if (is_meta4) {
                        // META4格式解析
                        metalink_info_.identity = QString::fromStdString(root.attribute("identity").as_string());
                        metalink_info_.version	= QString::fromStdString(root.attribute("version").as_string());

                        if (auto generator = root.child("generator")) {
                            metalink_info_.publisher_name = QString::fromStdString(generator.text().as_string());
                        }

                        if (auto published = root.child("published")) {
                            metalink_info_.published = QDateTime::fromString(
                                QString::fromStdString(published.text().as_string()), Qt::ISODate);
                        }
                    }
                    else {
                        // Metalink 3.0格式解析
                        if (auto publisher = root.child("publisher")) {
                            metalink_info_.publisher_name =
                                QString::fromStdString(publisher.child("name").text().as_string());
                            metalink_info_.publisher_url =
                                QString::fromStdString(publisher.child("url").text().as_string());
                        }
                    }

                    // 解析文件列表
                    for (pugi::xml_node file_node : root.children("file")) {
                        FileInfo file_info;

                        // 获取文件名
                        file_info.file_name = QString::fromStdString(file_node.attribute("name").as_string());

                        // 获取文件大小
                        if (auto size = file_node.child("size")) {
                            file_info.file_size = size.text().as_llong();
                        }

                        // 获取文件hash
                        if (auto hash = file_node.child("hash")) {
                            file_info.file_hash = QString::fromStdString(hash.text().as_string());
                        }

                        // 获取MIME类型
                        if (auto mime_type = file_node.child("metaurl")) {
                            file_info.mime_type = QString::fromStdString(mime_type.attribute("mediatype").as_string());
                        }

                        // 获取镜像链接
                        for (pugi::xml_node url : file_node.children("url")) {
                            metalink_info_.mirrors.append(QString::fromStdString(url.text().as_string()));
                        }

                        metalink_info_.files.append(file_info);
                    }

                    // 解析标签
                    for (pugi::xml_node tag : root.children("tags")) {
                        metalink_info_.tags.append(QString::fromStdString(tag.text().as_string()));
                    }

                    return true;
                } catch (const std::exception& e) {
                    LOG_ERR("Failed to parse metalink data: {}", e.what());
                    return false;
                }
            }

            bool MetalinkParser::IsMeta4Format(const QByteArray& data) const {
                pugi::xml_document doc;
                if (!doc.load_buffer(data.constData(), data.size())) {
                    return false;
                }

                pugi::xml_node root = doc.root();
                auto metalink_node	= root.child("metalink");

                if (!metalink_node) {
                    return false;
                }

                // 检查 Metalink 4.0 格式
                if (metalink_node.attribute("xmlns").as_string() == std::string("urn:ietf:params:xml:ns:metalink")) {
                    return true;
                }

                // 检查 Metalink 3.0 格式
                if (metalink_node.attribute("version").as_string() == std::string("3.0")) {
                    return false;
                }

                return false;
            }

        }  // namespace parser
    }  // namespace ui
}  // namespace gdl
