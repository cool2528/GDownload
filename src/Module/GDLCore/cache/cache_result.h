#pragma once

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace gdl::cache {

	enum class CacheOperation {
		kCreateDirectory,
		kOpen,
		kConfigure,
		kInspect,
		kMigrate,
		kPrepare,
		kBind,
		kStep,
		kCommit,
		kClose,
	};

	inline const char* CacheOperationName(CacheOperation operation) {
		switch (operation) {
			case CacheOperation::kCreateDirectory: return "create-directory";
			case CacheOperation::kOpen: return "open";
			case CacheOperation::kConfigure: return "configure";
			case CacheOperation::kInspect: return "inspect";
			case CacheOperation::kMigrate: return "migrate";
			case CacheOperation::kPrepare: return "prepare";
			case CacheOperation::kBind: return "bind";
			case CacheOperation::kStep: return "step";
			case CacheOperation::kCommit: return "commit";
			case CacheOperation::kClose: return "close";
		}
		return "unknown";
	}

	struct CacheError {
		CacheOperation operation{CacheOperation::kOpen};
		int primary_code{0};
		int extended_code{0};
		std::string path;
		std::string context;

		[[nodiscard]] std::string Describe() const {
			std::ostringstream stream;
			stream << CacheOperationName(operation) << " failed"
				   << " primary=" << primary_code << " extended=" << extended_code;
			if (!path.empty()) stream << " path=" << path;
			if (!context.empty()) stream << " context=" << context;
			return stream.str();
		}
	};

	template <typename T>
	class CacheResult {
	   public:
		template <typename U>
			requires std::is_constructible_v<T, U&&>
		static CacheResult Success(U&& value) {
			return CacheResult(std::in_place_index<0>, std::forward<U>(value));
		}

		static CacheResult Failure(CacheError error) {
			return CacheResult(std::in_place_index<1>, std::move(error));
		}

		[[nodiscard]] bool IsOk() const { return data_.index() == 0; }
		[[nodiscard]] bool HasError() const { return data_.index() == 1; }
		[[nodiscard]] T& Value() {
			if (HasError()) throw std::logic_error("CacheResult has no value");
			return std::get<0>(data_);
		}
		[[nodiscard]] const T& Value() const {
			if (HasError()) throw std::logic_error("CacheResult has no value");
			return std::get<0>(data_);
		}
		[[nodiscard]] CacheError& GetError() {
			if (IsOk()) throw std::logic_error("CacheResult has no error");
			return std::get<1>(data_);
		}
		[[nodiscard]] const CacheError& GetError() const {
			if (IsOk()) throw std::logic_error("CacheResult has no error");
			return std::get<1>(data_);
		}

	   private:
		template <std::size_t Index, typename U>
		explicit CacheResult(std::in_place_index_t<Index> index, U&& value)
			: data_(index, std::forward<U>(value)) {}

		std::variant<T, CacheError> data_;
	};

	template <>
	class CacheResult<void> {
	   public:
		static CacheResult Success() { return CacheResult(std::in_place_index<0>); }

		static CacheResult Failure(CacheError error) {
			return CacheResult(std::in_place_index<1>, std::move(error));
		}

		[[nodiscard]] bool IsOk() const { return data_.index() == 0; }
		[[nodiscard]] bool HasError() const { return data_.index() == 1; }
		[[nodiscard]] CacheError& GetError() {
			if (IsOk()) throw std::logic_error("CacheResult has no error");
			return std::get<1>(data_);
		}
		[[nodiscard]] const CacheError& GetError() const {
			if (IsOk()) throw std::logic_error("CacheResult has no error");
			return std::get<1>(data_);
		}

	   private:
		explicit CacheResult(std::in_place_index_t<0>) : data_(std::in_place_index<0>) {}
		explicit CacheResult(std::in_place_index_t<1>, CacheError error)
			: data_(std::in_place_index<1>, std::move(error)) {}

		std::variant<std::monostate, CacheError> data_;
	};

}  // namespace gdl::cache
