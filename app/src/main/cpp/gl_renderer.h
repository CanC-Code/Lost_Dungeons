2026-05-29T19:21:46.1442861Z ##[group]Run ./gradlew assembleDebug
./gradlew assembleDebug
shell: /usr/bin/bash -e {0}
env:
  JAVA_HOME: /opt/hostedtoolcache/Java_Temurin-Hotspot_jdk/17.0.19-10/x64
  JAVA_HOME_17_X64: /opt/hostedtoolcache/Java_Temurin-Hotspot_jdk/17.0.19-10/x64
Fetching distribution.
Downloading https://services.gradle.org/distributions/gradle-8.7-bin.zip
............10%.............20%.............30%.............40%............50%.............60%.............70%.............80%.............90%............100%

Welcome to Gradle 8.7!

Here are the highlights of this release:
 - Compiling and testing with Java 22
 - Cacheable Groovy script compilation
 - New methods in lazy collection properties

For more details see https://docs.gradle.org/8.7/release-notes.html

Starting a Gradle Daemon (subsequent builds will be faster)
> Task :app:preBuild UP-TO-DATE
> Task :app:preDebugBuild UP-TO-DATE
> Task :app:mergeDebugNativeDebugMetadata NO-SOURCE
> Task :app:checkKotlinGradlePluginConfigurationErrors
> Task :app:generateDebugResValues
> Task :app:checkDebugAarMetadata
> Task :app:mapDebugSourceSetPaths
> Task :app:generateDebugResources
> Task :app:packageDebugResources
> Task :app:mergeDebugResources
> Task :app:createDebugCompatibleScreenManifests
> Task :app:extractDeepLinksDebug
> Task :app:parseDebugLocalResources
> Task :app:processDebugMainManifest
> Task :app:processDebugManifest
> Task :app:processDebugManifestForPackage
> Task :app:javaPreCompileDebug
> Task :app:mergeDebugShaders
> Task :app:compileDebugShaders NO-SOURCE
> Task :app:generateDebugAssets UP-TO-DATE
> Task :app:mergeDebugAssets
> Task :app:desugarDebugFileDependencies
> Task :app:compressDebugAssets
> Task :app:processDebugResources
> Task :app:checkDebugDuplicateClasses
> Task :app:mergeDebugStartupProfile
> Task :app:kaptGenerateStubsDebugKotlin
> Task :app:mergeLibDexDebug
> Task :app:mergeExtDexDebug

> Task :app:configureCMakeDebug[arm64-v8a]
Checking the license for package CMake 3.22.1 in /usr/local/lib/android/sdk/licenses
License for package CMake 3.22.1 accepted.
Preparing "Install CMake 3.22.1 v.3.22.1".

> Task :app:kaptDebugKotlin

> Task :app:configureCMakeDebug[arm64-v8a]
"Install CMake 3.22.1 v.3.22.1" ready.
Installing CMake 3.22.1 in /usr/local/lib/android/sdk/cmake/3.22.1
"Install CMake 3.22.1 v.3.22.1" complete.
"Install CMake 3.22.1 v.3.22.1" finished.

> Task :app:buildCMakeDebug[arm64-v8a]
C/C++: ninja: Entering directory `/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a'
C/C++: /usr/local/lib/android/sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ --target=aarch64-none-linux-android26 --sysroot=/usr/local/lib/android/sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot -Dengine_EXPORTS -I/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a/_deps/json-src/include -I/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a/_deps/glm-src/glm/.. -g -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security  -std=c++17 -fno-limit-debug-info  -fPIC -MD -MT CMakeFiles/engine.dir/gl_renderer.cpp.o -MF CMakeFiles/engine.dir/gl_renderer.cpp.o.d -o CMakeFiles/engine.dir/gl_renderer.cpp.o -c /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:399:38: error: too few arguments to function call, expected 2, have 1
C/C++:             drawCompassHUD(engineTime);
C/C++:             ~~~~~~~~~~~~~~           ^
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.h:46:21: note: 'drawCompassHUD' declared here
C/C++:         static void drawCompassHUD(float engineTime, const glm::mat4& proj);
C/C++:                     ^
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:406:29: error: too few arguments to function call, single argument 'proj' was not specified
C/C++:             drawMenuOverlay();
C/C++:             ~~~~~~~~~~~~~~~ ^
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.h:48:21: note: 'drawMenuOverlay' declared here
C/C++:         static void drawMenuOverlay(const glm::mat4& proj);
C/C++:                     ^
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:458:22: error: out-of-line definition of 'drawCompassHUD' does not match any declaration in 'LostDungeons::GLRenderer'
C/C++:     void GLRenderer::drawCompassHUD(float engineTime) {
C/C++:                      ^~~~~~~~~~~~~~
C/C++: /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:581:22: error: out-of-line definition of 'drawMenuOverlay' does not match any declaration in 'LostDungeons::GLRenderer'
C/C++:     void GLRenderer::drawMenuOverlay() {
C/C++:                      ^~~~~~~~~~~~~~~
C/C++: 4 errors generated.

> Task :app:buildCMakeDebug[arm64-v8a] FAILED

> Task :app:compileDebugKotlin
FAILURE: Build failed with an exception.
28 actionable tasks: 28 executed

* What went wrong:
Execution failed for task ':app:buildCMakeDebug[arm64-v8a]'.
> com.android.ide.common.process.ProcessException: ninja: Entering directory `/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a'
  [1/5] Building CXX object CMakeFiles/engine.dir/engine_core.cpp.o
  [2/5] Building CXX object CMakeFiles/engine.dir/gl_renderer.cpp.o
  FAILED: CMakeFiles/engine.dir/gl_renderer.cpp.o 
  /usr/local/lib/android/sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ --target=aarch64-none-linux-android26 --sysroot=/usr/local/lib/android/sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot -Dengine_EXPORTS -I/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a/_deps/json-src/include -I/home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a/_deps/glm-src/glm/.. -g -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security  -std=c++17 -fno-limit-debug-info  -fPIC -MD -MT CMakeFiles/engine.dir/gl_renderer.cpp.o -MF CMakeFiles/engine.dir/gl_renderer.cpp.o.d -o CMakeFiles/engine.dir/gl_renderer.cpp.o -c /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:399:38: error: too few arguments to function call, expected 2, have 1
              drawCompassHUD(engineTime);
              ~~~~~~~~~~~~~~           ^
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.h:46:21: note: 'drawCompassHUD' declared here
          static void drawCompassHUD(float engineTime, const glm::mat4& proj);
                      ^
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:406:29: error: too few arguments to function call, single argument 'proj' was not specified
              drawMenuOverlay();
              ~~~~~~~~~~~~~~~ ^
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.h:48:21: note: 'drawMenuOverlay' declared here
          static void drawMenuOverlay(const glm::mat4& proj);
                      ^
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:458:22: error: out-of-line definition of 'drawCompassHUD' does not match any declaration in 'LostDungeons::GLRenderer'
      void GLRenderer::drawCompassHUD(float engineTime) {
                       ^~~~~~~~~~~~~~
  /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/src/main/cpp/gl_renderer.cpp:581:22: error: out-of-line definition of 'drawMenuOverlay' does not match any declaration in 'LostDungeons::GLRenderer'
      void GLRenderer::drawMenuOverlay() {
                       ^~~~~~~~~~~~~~~
  4 errors generated.
  [3/5] Building CXX object CMakeFiles/engine.dir/asset_manager.cpp.o
  [4/5] Building CXX object CMakeFiles/engine.dir/jni_bridge.cpp.o
  ninja: build stopped: subcommand failed.
  
  C++ build system [build] failed while executing:
      /usr/local/lib/android/sdk/cmake/3.22.1/bin/ninja \
        -C \
        /home/runner/work/Lost_Dungeons/Lost_Dungeons/app/.cxx/Debug/2o27371v/arm64-v8a \
        engine
    from /home/runner/work/Lost_Dungeons/Lost_Dungeons/app

* Try:
> Run with --stacktrace option to get the stack trace.
> Run with --info or --debug option to get more log output.
> Run with --scan to get full insights.
> Get more help at https://help.gradle.org.

BUILD FAILED in 1m 31s
Process completed with exit code 1.