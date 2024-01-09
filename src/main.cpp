#include <spdlog/sinks/basic_file_sink.h>
#include "fireRegister.h"
#include "hitManager.h"
#include "loadEventManager.h"
#include "hooks.h"
#include "papyrus.h"

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");

    auto pluginName = "RainExtinguishesFires";
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
        Hooks::Install();
        _loggerInfo("Hooked Weather Change event.");

        if (SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions)) {
            _loggerInfo("Registered new Papyrus Functions.");
        }
        else {
            _loggerError("Failed to register the new Papyrus functions. Aborting load...");
            break;
        }

        if (FireRegistry::FireRegistry::GetSingleton()->BuildRegistry()) {
            _loggerInfo("Built Lit/Unlit fire map.");
        }
        else {
            _loggerError("Failed to build the Lit/Unlit fire map. Aborting load...");
            break;
        }

        if (LoadManager::LoadManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Load/Unload manager.");
        }
        else {
            _loggerError("Failed to register the Load/Unload manager. Aborting load...");
            break;
        }

        if (HitManager::HitManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Hit manager.");
        }
        else {
            _loggerError("Failed to register the Hit manager. Aborting load...");
            break;
        }

        if (LoadManager::ActorCellManager::GetSingleton()->RegisterListener()) {
            _loggerInfo("Registered the Player Cell Change manager.");
        }
        else {
            _loggerError("Failed to register the Player Cell Change manager. Aborting load...");
            break;
        }

        _loggerInfo("All pre-game operation succeeded.");
        break;
    }
    default:
        break;
    }
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion(REL::Version(5));
    v.PluginName("Rain Extinguishes Fires");
    v.AuthorName("SeaSparrow");
    v.UsesAddressLibrary();
    v.UsesUpdatedStructs();
    v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

    return v;
    }();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "Rain Extinguishes Fires";
    a_info->version = Version::MAJOR;

    const auto ver = a_skse->RuntimeVersion();
    if (ver
#	ifndef SKYRIMVR
        < SKSE::RUNTIME_1_5_39
#	else
        > SKSE::RUNTIME_VR_1_4_15_1
#	endif
        ) {
        logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
        return false;
    }

    return true;
}
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse) {
    SetupLog();

    _loggerInfo("Rain Extinguishes Fires is performing startup tasks.");

    SKSE::Init(a_skse);
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);

    _loggerInfo("Rain Extinguishes Fires has initialized. Version: 5.0.0");

    return true;
}