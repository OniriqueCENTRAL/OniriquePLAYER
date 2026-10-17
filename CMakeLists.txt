cmake_minimum_required(VERSION 3.22)

# Mac : binaire universel (Intel + Apple Silicon), à définir avant project()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(PlayerOnirique VERSION 1.0.0 LANGUAGES C CXX)

# JUCE : téléchargé au premier configure, ou repris d'un dossier local avec
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=<chemin vers juce>   (voir build_windows.bat)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

# AU (Logic, GarageBand) uniquement sur Mac
set(ONIRIQUE_FORMATS VST3)
if(APPLE)
    list(APPEND ONIRIQUE_FORMATS AU)
endif()

# Codes inchangés depuis la première version : les projets déjà enregistrés retrouvent le plugin.
juce_add_plugin(PlayerOnirique
    COMPANY_NAME               "Onirique"
    PLUGIN_MANUFACTURER_CODE   Onrq
    PLUGIN_CODE                Plo1
    FORMATS                    ${ONIRIQUE_FORMATS}
    AU_MAIN_TYPE               kAudioUnitType_Effect
    PRODUCT_NAME               "PLAYER ONIRIQUE"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    VST3_CATEGORIES            Fx
    COPY_PLUGIN_AFTER_BUILD    FALSE
    NEEDS_WEB_BROWSER          TRUE
    NEEDS_WEBVIEW2             TRUE)

# La page du lecteur (HTML), les polices et le logo de la charte sont embarqués dans le plugin
juce_add_binary_data(PlayerOniriqueData SOURCES
    Resources/player.html
    Resources/logo.png
    Resources/fonts/Unbounded800.ttf
    Resources/fonts/GrenzeGotisch600.ttf
    Resources/fonts/HankenGrotesk600.ttf
    Resources/fonts/JetBrainsMono500.ttf)

target_sources(PlayerOnirique PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

if(APPLE)
    target_sources(PlayerOnirique PRIVATE Source/MacFileAccess.mm)
endif()

target_compile_features(PlayerOnirique PRIVATE cxx_std_17)

target_compile_definitions(PlayerOnirique PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)

if(MSVC)
    target_compile_options(PlayerOnirique PRIVATE /utf-8)
endif()

# Outil de développement Windows (hors produit) : charge le VST3 comme un DAW et photographie la synchro, voir README
if(WIN32)
    juce_add_console_app(PlayerOniriqueHostCheck PRODUCT_NAME "PlayerOniriqueHostCheck")
    set_target_properties(PlayerOniriqueHostCheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
    target_sources(PlayerOniriqueHostCheck PRIVATE tools/host_check.cpp)
    target_compile_definitions(PlayerOniriqueHostCheck PRIVATE
        JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_compile_options(PlayerOniriqueHostCheck PRIVATE /utf-8)
    target_link_libraries(PlayerOniriqueHostCheck PRIVATE juce::juce_audio_processors juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags)
endif()

target_link_libraries(PlayerOnirique
    PRIVATE
        PlayerOniriqueData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
