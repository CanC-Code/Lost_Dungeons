#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>

namespace LostDungeons {
    class AssetManager {
    public:
        // Initializes the native manager from the Kotlin object
        static void init(JNIEnv* env, jobject assetManager);
        
        // Reads a file from the APK assets folder into a string
        static std::string loadTextFile(const std::string& path);
        
    private:
        static AAssetManager* manager;
    };
}

#endif
