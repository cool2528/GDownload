#pragma once
#include <QObject>
#include <QVariantMap>
#include <QQmlEngine>
#include "GDLCore/singleton.hpp"

namespace gdl {
namespace ui {
namespace message {

/**
 * MessageManager - 消息管理器单例
 * 
 * 提供类似 Element Plus ElMessage 的 API
 * 支持多种消息类型和位置配置
 */
class MessageManager : public QObject, public Singleton<MessageManager> {
    Q_OBJECT
    SINGLETON_DECLARE(MessageManager)
    QML_SINGLETON

public:
    static MessageManager* create(QQmlEngine*, QJSEngine*);
    ~MessageManager() override;

    // ========== 基础方法 ==========
    
    /**
     * 显示消息（完整配置）
     * @param message 消息内容
     * @param options 配置选项 (type, duration, showClose, placement, etc.)
     * @return 消息ID
     */
    Q_INVOKABLE int show(const QString& message, const QVariantMap& options = QVariantMap());
    
    // ========== 便捷方法 ==========
    
    /**
     * 显示 Primary 类型消息
     */
    Q_INVOKABLE int primary(const QString& message, int duration = 3000);
    
    /**
     * 显示成功消息
     */
    Q_INVOKABLE int success(const QString& message, int duration = 3000);
    
    /**
     * 显示警告消息
     */
    Q_INVOKABLE int warning(const QString& message, int duration = 3000);
    
    /**
     * 显示信息消息
     */
    Q_INVOKABLE int info(const QString& message, int duration = 3000);
    
    /**
     * 显示错误消息
     */
    Q_INVOKABLE int error(const QString& message, int duration = 3000);
    
    // ========== 控制方法 ==========
    
    /**
     * 关闭指定 ID 的消息
     */
    Q_INVOKABLE void close(int messageId);
    
    /**
     * 关闭所有消息
     */
    Q_INVOKABLE void closeAll();

private:
    explicit MessageManager(QObject* parent = nullptr);
    
    /**
     * 生成唯一的消息 ID
     */
    int generateMessageId();
    
    /**
     * 构建完整的选项映射
     */
    QVariantMap buildOptions(const QString& message, 
                             const QString& type, 
                             int duration);

    // 下一个消息 ID
    int m_nextMessageId;

Q_SIGNALS:
    /**
     * 请求显示消息的信号（与 QML 层通信）
     */
    void messageRequested(const QVariantMap& options);
};

/**
 * 注册到 QML 引擎
 */
void RegisterTypes(QQmlEngine* engine);

}  // namespace message
}  // namespace ui
}  // namespace gdl
