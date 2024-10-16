#pragma once

#include <memory>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;    \
    TypeName& operator=(const TypeName&) = delete;

#define DISALLOW_MOVE(TypeName)    \
    TypeName(TypeName&&) = delete; \
    TypeName& operator=(TypeName&&) = delete;

#define SINGLETON_DECLARE(TypeName) friend class Singleton<TypeName>;

namespace gdl {
template <typename T>
class Singleton {
public:
    Singleton() = default;
    virtual ~Singleton() = default;

    template <typename... Args>
    static T& Instance(Args&&... args) {
        static std::once_flag flag;
        std::call_once(flag, [&args...]() { instance_.reset(new T(std::forward<Args>(args)...)); });
        return *instance_;
    }

    DISALLOW_COPY_AND_ASSIGN(Singleton);
    DISALLOW_MOVE(Singleton);

private:
    inline static std::unique_ptr<T> instance_{nullptr};
};
}  // namespace gdl