#include <spdlog/sinks/basic_file_sink.h>

#include "fireRegister.h"
#include "eventListener.h"
#include "papyrus.h"
#include "settingsReader.h"

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");

    auto pluginName = Version::PROJECT;
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);

    //Pattern
    spdlog::set_pattern("%v");
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
    bool success = true;
    switch (a_message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions);
        if (!Events::RegisterForEvents()) success = false;
        if (success && !Settings::InitializeINISettings()) success = false;
        if (success && !Settings::InitializeFireSettings()) success = false;

        if (!success) {
            _loggerInfo("One or more failures occured during loaded.");
        }
        else {
            _loggerInfo("Finished startup tasks successfully.");
            CachedData::Fires::GetSingleton()->Report();
        }
        _loggerInfo("----------------------------------------------------------------");

        break;
    case SKSE::MessagingInterface::kNewGame:
    case SKSE::MessagingInterface::kPostLoadGame:
        if (!success) break;
        Events::Weather::WeatherEventManager::GetSingleton()->UpdateWeatherFlag();
        break;
    default:
        break;
    }
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH });
    v.PluginName(Version::PROJECT);
    v.AuthorName(Version::AUTHOR);
    v.UsesAddressLibrary();
    v.UsesUpdatedStructs();
    v.CompatibleVersions({
        SKSE::RUNTIME_1_6_1130,
        _1_6_1170 });
    return v;
    }();
#else 
SKSEPluginInfo(
    .Name = "RainExtinguishesFires"
)
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {
    SetupLog();
    _loggerInfo("Starting up {}.", Version::PROJECT);
    _loggerInfo("Plugin Version: {}.{}.{}", Version::MAJOR, Version::MINOR, Version::PATCH);
    _loggerInfo("Version build:");

#ifdef SKYRIM_AE
    _loggerInfo("    >Latest Version. Maintained by: {}.", Version::AUTHOR);
#else
    _loggerInfo("    >1.5 Version. Do not report ANY issues with this version.");
#endif
    _loggerInfo("----------------------------------------------------------------");

    SKSE::Init(a_skse);
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);

    return true;
}