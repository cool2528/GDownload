#pragma once
/**
 * @brief Result type for conveniently returning various data, populating error messages, etc.
 */

#include <string>
#include <type_traits>
namespace gdl {

	enum class ErrorType : int {
		kSuccessful = 0,
		kUnknownError,
		kTimeOut,
		kNotFount,
	};

	struct Error {
		explicit Error(const std::int64_t& code, const std::string_view& msg) : code_(code), msg_(msg) {}
		Error(const Error& other) { *this = other; }
		Error& operator=(const Error& other) {
			this->code_ = other.code_;
			this->msg_	= other.msg_;
			return *this;
		}
		Error(Error&& other) { *this = other; }
		Error& operator=(Error&& other) {
			this->code_ = std::move(other.code_);
			this->msg_	= std::move(other.msg_);
			return *this;
		}
		std::int64_t Code() const { return code_; }
		const char* what() const { return msg_.data(); }

	   private:
		std::int64_t code_{0};
		std::string_view msg_;
	};

	template <typename T>
	class Result {
	   public:
		Result(const T& value) : value_(value) {}
		Result(T&& value) : value_(std::move(value)) {}
		Result(Error&& err) : err_(std::move(err)) {}
		Result(const Error& err) : err_(err) {}
		explicit operator bool() const {
			if constexpr (std::is_same_v<T, bool>) {
				return err_.Code() == 0 && value_ == true;
			}
			else {
				return err_.Code() == 0;
			}
		}
		Result(const Result& other) { *this = other; }
		Result& operator=(const Result& other) {
			this->err_	 = other.err_;
			this->value_ = other.value_;
			return *this;
		}
		Result(Result&& other) { *this = std::move(other); }
		Result& operator=(Result&& other) {
			this->err_	 = std::move(other.err_);
			this->value_ = std::move(other.value_);
			return *this;
		}
		bool operator!() const { return !static_cast<bool>(*this); }
		operator T() { return value_; }
		bool HasError() const { return err_.Code() != 0; }
		bool IsOk() const { return err_.Code() == 0; }
		Error& GetError() { return err_; }
		T& Value() { return value_; }
		const Error& GetError() const { return err_; }
		const T& Value() const { return value_; }

	   private:
		T value_;
		Error err_{0, ""};
	};

	template <>
	class Result<void> {
	   public:
		Result(const Error& err) : err_(err) {}
		Result(Error&& err) : err_(std::move(err)) {}
		explicit operator bool() const { return err_.Code() == 0; }
		Result(const Result& other) { err_ = other.err_; }

		Result& operator=(const Result& other) {
			err_ = other.err_;
			return *this;
		}

		Result(Result&& other) noexcept { err_ = std::move(other.err_); }

		Result& operator=(Result&& other) noexcept {
			err_ = std::move(other.err_);
			return *this;
		}
		bool operator!() const { return !static_cast<bool>(*this); }
		bool HasError() const { return err_.Code() != 0; }
		bool IsOk() const { return err_.Code() == 0; }
		Error& GetError() { return err_; }
		const Error& GetError() const { return err_; }

	   private:
		Error err_{0, ""};
	};

	inline static Error MakeFail(const std::int64_t& code, const std::string_view& msg = "") {
		return Error(code, msg);
	}
}  // namespace gdl
