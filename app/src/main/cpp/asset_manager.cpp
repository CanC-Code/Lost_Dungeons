#include "asset_manager.h"
#include <android/log.h>
#include <vector>

namespace LostDungeons {
    AAssetManager* AssetManager::manager = nullptr;

    void AssetManager::init(JNIEnv* env, jobject assetManager) {
        manager = AAssetManager_fromJava(env, assetManager);
        __android_log_print(ANDROID_LOG_INFO, "LostDungeonsNative", "AssetManager initialized in C++");
    }

    std::string AssetManager::loadTextFile(const std::string& path) {
        if (!manager) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsNative", "AssetManager not initialized!");
            return "";
        }

        AAsset* asset = AAssetManager_open(manager, path.c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsNative", "Failed to open asset: %s", path.c_str());
            return "";
        }

        off_t length = AAsset_getLength(asset);
        std::vector<char> buffer(length + 1);
        AAsset_read(asset, buffer.data(), length);
        buffer[length] = '\0'; // Null-terminate the string
        AAsset_close(asset);

        return std::string(buffer.data());
    }
}
