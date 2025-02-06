#pragma once
#include <QObject>
#include "singleton.hpp"
namespace gdl {
    namespace ui {
        namespace utils {
            class ClipboardWatcher : public QObject, public Singleton<ClipboardWatcher> {
                Q_OBJECT
                SINGLETON_DECLARE(ClipboardWatcher)
               public:
                ~ClipboardWatcher() override;
                Q_INVOKABLE QString GetClipboardText() const;
               Q_SIGNALS:
                void clipboardChanged(QString data);

               private:
                explicit ClipboardWatcher(QObject* parent = nullptr);
               private Q_SLOTS:
                void clipboardChangedSlot();

               private:
                QString clipboard_text_;
            };
        }  // namespace utils
    }  // namespace ui
}  // namespace gdl
