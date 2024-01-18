#include <spdlog/sinks/basic_file_sink.h>

#include "startupTasks.h"

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

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
    switch (a_message->type) {
    case SKSE::MessagingInterface::kDataLoaded: {
        if (StartupTasks::ApplyDataLoadedChanges()) {
            _loggerInfo("Startup tasks completed, enjoy your game!");
        }
        else {
            _loggerInfo("Errors occured during loading.");
        }
        break;
    }
    default:
        break;
    }
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion({Version::MAJOR, Version::MINOR, Version::PATCH});
    v.PluginName(Version::PROJECT);
    v.AuthorName("SeaSparrow");
    v.UsesAddressLibrary();
    v.UsesUpdatedStructs();

#ifdef SUPPORT_OLDER_VERSION
    v.CompatibleVersions({ SKSE::RUNTIME_1_6_1130, SKSE::RUNTIME_1_6_659, SKSE::RUNTIME_1_6_640, SKSE::RUNTIME_1_6_629 });
#else
    v.CompatibleVersions({ SKSE::RUNTIME_LATEST });
#endif

    return v;
    }();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "Rain Extinguishes Fires";
    a_info->version = 1;

    const auto ver = a_skse->RuntimeVersion();
    if (ver
#	ifndef SKYRIMVR
        < SKSE::RUNTIME_1_5_39
#	else
        > SKSE::RUNTIME_VR_1_4_15_1
#	endif
        ) {
        SKSE::log::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
        return false;
    }

    return true;
}
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {
    SetupLog();
    _loggerInfo("Starting up Rain Extinguishes Fires.");
    _loggerInfo("Plugin Version: {}.{}.{}", Version::MAJOR, Version::MINOR, Version::PATCH);
    _loggerInfo("Version build:");

#ifdef SKYRIM_AE
#ifdef SUPPORT_OLDER_VERSION
    _loggerInfo("    >Pre-1.6.1130 Version.");
#else 
    _loggerInfo("    >1.6.1130 Version.");
#endif
#else 
    _loggerInfo("    >1.5.97 Version.");
#endif

    _loggerInfo("Rain Extinguishes Fires is performing startup tasks.");

    SKSE::Init(a_skse);
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);

    return true;
}