#pragma once

#include <filesystem>
#include <ostream>
#include <utility>

#include <toml++/toml.h>

#include "Module/GDLCore/filesystem/atomic_file_replace.h"

namespace gdl {
	namespace config {
		namespace detail {

			template <typename ReplaceOperation>
			Result<void> SaveTomlAtomically(const std::filesystem::path& target, const toml::table& root,
											ReplaceOperation&& replaceOperation) {
				return std::forward<ReplaceOperation>(replaceOperation)(
					target, [&root](std::ostream& output) -> Result<void> {
						output << root;
						if (!output.good()) {
							return MakeFail(static_cast<std::int64_t>(filesystem::AtomicFileReplaceError::kWrite),
											"Failed to serialize TOML configuration");
						}
						return Error(0, "");
					});
			}

			inline Result<void> SaveTomlAtomically(const std::filesystem::path& target, const toml::table& root) {
				return SaveTomlAtomically(target, root, filesystem::AtomicFileReplace);
			}

		}  // namespace detail
	}  // namespace config
}  // namespace gdl
