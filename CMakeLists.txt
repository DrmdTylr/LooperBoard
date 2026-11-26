cmake_minimum_required(VERSION 3.22)
project(Looper VERSION 0.1.0 LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(WIN32)
    FetchContent_Declare(asio_sdk
        GIT_REPOSITORY https://github.com/audiosdk/asio.git
        GIT_TAG 496a0765b8bb9c26f764f22f9a9712a937177db2)
    FetchContent_MakeAvailable(asio_sdk)

    FetchContent_Declare(webview2
        URL https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.2903.40/microsoft.web.webview2.1.0.2903.40.nupkg
        URL_HASH SHA256=ef128016dd1e51c59178c827ed5b8aa3322c57afa8675d930f8109505542ad74
        SOURCE_DIR "${CMAKE_BINARY_DIR}/webview2/Microsoft.Web.WebView2.1.0.2903.40")
    FetchContent_MakeAvailable(webview2)
    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${CMAKE_BINARY_DIR}/webview2")
endif()

if(WIN32)
    juce_add_gui_app(Looper PRODUCT_NAME "Looper" NEEDS_WEBVIEW2 TRUE)
else()
    juce_add_gui_app(Looper PRODUCT_NAME "Looper")
endif()
    juce_add_binary_data(LooperWebAssets SOURCES
        mockups/index.html
        mockups/styles.css
        mockups/demo.js
        mockups/native.js
        mockups/assets/black-plugged.png
        mockups/assets/right-angle-plugged.png)
target_sources(Looper PRIVATE src/Main.cpp)
if(WIN32)
    target_include_directories(Looper PRIVATE "${asio_sdk_SOURCE_DIR}/common")
    target_compile_definitions(Looper PRIVATE JUCE_ASIO=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
target_compile_definitions(Looper PRIVATE
    JUCE_WEB_BROWSER=$<BOOL:${WIN32}> JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Looper,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(Looper PRIVATE
    LooperWebAssets
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

enable_testing()
add_executable(LooperTests tests/LooperTests.cpp)
target_include_directories(LooperTests PRIVATE src)
add_test(NAME LooperTests COMMAND LooperTests)

juce_add_console_app(LoopAudioFileTests PRODUCT_NAME "LoopAudioFileTests")
target_sources(LoopAudioFileTests PRIVATE tests/LoopAudioFileTests.cpp)
target_compile_definitions(LoopAudioFileTests PRIVATE JUCE_USE_CURL=0)
target_link_libraries(LoopAudioFileTests PRIVATE juce::juce_audio_formats juce::juce_audio_processors
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME LoopAudioFileTests COMMAND LoopAudioFileTests)

if(WIN32)
    juce_add_console_app(AudioDeviceTests PRODUCT_NAME "AudioDeviceTests")
    target_sources(AudioDeviceTests PRIVATE tests/AudioDeviceTests.cpp)
    target_include_directories(AudioDeviceTests PRIVATE "${asio_sdk_SOURCE_DIR}/common")
    target_compile_definitions(AudioDeviceTests PRIVATE JUCE_ASIO=1 JUCE_USE_CURL=0)
    target_link_libraries(AudioDeviceTests PRIVATE juce::juce_audio_devices
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME AudioDeviceTests COMMAND AudioDeviceTests)
endif()
