#include <spdlog/sinks/basic_file_sink.h>

#include "fireRegister.h"
#include "hitManager.h"
#include "hooks.h"
#include "loadEventManager.h"
#include "papyrus.h"

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
        bool unregisterAll = false;
        bool registeredLoad = false;
        bool registeredChange = false;
        bool registeredHit = false;

        Hooks::Install();
        _loggerInfo("Hooked functions.");

        if (!unregisterAll && LoadManager::LoadManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Load/Unload manager.");
            registeredLoad = true;
        }
        else {
            unregisterAll = true;
        }

        if (!unregisterAll && LoadManager::ActorCellManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Player manager.");
            registeredChange = true;
        }
        else {
            unregisterAll = true;
        }

        if (!unregisterAll && HitManager::HitManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Hit manager.");
            registeredHit = true;
        }
        else {
            unregisterAll = true;
        }

        if (!unregisterAll && SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions)) {
            _loggerInfo("Registered the new Papyrus functions.");
        }
        else {
            unregisterAll = true;
        }

        if (CachedData::FireRegistry::GetSingleton()->BuildRegistry()) {
            _loggerInfo("Built the cache system.");
        }
        else {
            unregisterAll = true;
        }

        if (unregisterAll) {
            _loggerInfo("Error(s) occured while preparing the plugin. Loading stopped.");
            LoadManager::LoadManager::GetSingleton()->UnRegisterListener();
            HitManager::HitManager::GetSingleton()->UnRegisterListener();
            Papyrus::Papyrus::GetSingleton()->DisablePapyrus();
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
    v.CompatibleVersions({ 
        SKSE::RUNTIME_1_6_659, 
        SKSE::RUNTIME_1_6_640, 
        SKSE::RUNTIME_1_6_629 });
#else
    v.CompatibleVersions({ 
        SKSE::RUNTIME_1_6_1130, 
        _1_6_1170 });
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
    _loggerInfo("    >Latest Version.");
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