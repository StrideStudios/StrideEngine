#pragma once

#include <filesystem>
#include <string>

#include "Singleton.h"

struct SPaths : SObject {

	REGISTER_CLASS(SPaths, SObject)
	MAKE_SINGLETON(SPaths)

	inline static TShared<SPaths> singletons;

private:

	struct Directory : std::filesystem::path {

		Directory() = default;

		Directory(const path& path): path(path) {
			remove_filename();
			std::filesystem::create_directory(*this);
		}

		Directory(const std::string& string): Directory(path(string)) {}

		operator std::string() const {
			return string();
		}

		operator const char*() const {
			return string().c_str();
		}
	};

public:

	SPaths() {
		const std::filesystem::path cwd = std::filesystem::current_path();
		mEnginePath = cwd.parent_path().string() + "\\";
		mSourcePath = mEnginePath.string() + "src\\";
		mShaderPath = mEnginePath.string() + "shaders\\";
		mAssetPath = mEnginePath.string() + "assets\\";

		mCachePath = mEnginePath.string() + "cache\\";
	}

	Directory mEnginePath;
	Directory mSourcePath;
	Directory mShaderPath;
	Directory mCachePath;
	Directory mAssetPath;
};
