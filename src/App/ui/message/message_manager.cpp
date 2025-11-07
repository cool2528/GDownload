#include "message_manager.h"
#include "Definitions/appDef.h"

namespace gdl {
namespace ui {
namespace message {

MessageManager* MessageManager::create(QQmlEngine*, QJSEngine*) {
    return &Instance();
}

MessageManager::~MessageManager() {}

MessageManager::MessageManager(QObject* parent) 
    : QObject(parent), m_nextMessageId(1) {
}

// ========== 基础方法实现 ==========

int MessageManager::show(const QString& message, const QVariantMap& options) {
    int messageId = generateMessageId();
    
    // 构建完整的选项映射
    QVariantMap finalOptions = options;
    finalOptions["id"] = messageId;
    finalOptions["message"] = message;
    
    // 如果没有指定类型，默认为 info
    if (!finalOptions.contains("type")) {
        finalOptions["type"] = "info";
    }
    
    // 如果没有指定持续时间，默认为 3000ms
    if (!finalOptions.contains("duration")) {
        finalOptions["duration"] = 3000;
    }
    
    // 发送信号到 QML 层
    emit messageRequested(finalOptions);
    
    return messageId;
}

// ========== 便捷方法实现 ==========

int MessageManager::primary(const QString& message, int duration) {
    return show(message, buildOptions(message, "primary", duration));
}

int MessageManager::success(const QString& message, int duration) {
    return show(message, buildOptions(message, "success", duration));
}

int MessageManager::warning(const QString& message, int duration) {
    return show(message, buildOptions(message, "warning", duration));
}

int MessageManager::info(const QString& message, int duration) {
    return show(message, buildOptions(message, "info", duration));
}

int MessageManager::error(const QString& message, int duration) {
    return show(message, buildOptions(message, "error", duration));
}

// ========== 控制方法实现 ==========

void MessageManager::close(int messageId) {
    QVariantMap options;
    options["action"] = "close";
    options["id"] = messageId;
    emit messageRequested(options);
}

void MessageManager::closeAll() {
    QVariantMap options;
    options["action"] = "closeAll";
    emit messageRequested(options);
}

// ========== 私有辅助方法 ==========

int MessageManager::generateMessageId() {
    return m_nextMessageId++;
}

QVariantMap MessageManager::buildOptions(const QString& message, 
                                          const QString& type, 
                                          int duration) {
    QVariantMap options;
    options["message"] = message;
    options["type"] = type;
    options["duration"] = duration;
    return options;
}

// ========== QML 类型注册 ==========

void RegisterTypes(QQmlEngine* engine) {
    qmlRegisterSingletonInstance<MessageManager>(
        GEXPORT_MODULE_URL, 1, 0, "MessageManager",
        &MessageManager::Instance()
    );
}

}  // namespace message
}  // namespace ui
}  // namespace gdl
