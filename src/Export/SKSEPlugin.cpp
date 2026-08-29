#include "Cache/FormCache.h"
#include "Data/ModObjectManager.h"
#include "FireManipulation/Manipulator.h"
#include "Papyrus/Papyrus.h"
#include "Serialization/Serde.h"
#include "Settings/INI/INISettings.h"
#include "Settings/JSON/JSONSettings.h"

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		SECTION_SEPARATOR;
		if (!Data::PreloadModObjects()) {
			REX::FAIL("Failed to preload mod objects. Check the log for more information."sv);
		}
		SECTION_SEPARATOR;
		if (!Cache::InitializeCache()) {
			REX::FAIL("Failed to initialize cache. Check the log for more information."sv);
		}
		SECTION_SEPARATOR;
		if (!FireManipulator::Install()) {
			REX::FAIL("Failed to initialize cache. Check the log for more information."sv);
		}
		SECTION_SEPARATOR;
		logger::INFO("Finished startup tasks, enjoy your game!"sv);
		Settings::JSON::Holder::GetSingleton()->Release();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v{};

		v.PluginVersion(Plugin::VERSION);
		v.PluginName(Plugin::NAME);
		v.AuthorName("SeaSparrow"sv);
		v.UsesAddressLibrary();
		v.UsesUpdatedStructs();

		return v;
	}();
#endif

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		logger::CRITICAL("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
#ifdef SKYRIM_AE
	if (ver < SKSE::RUNTIME_SSE_LATEST) {
#else
	if (ver < SKSE::RUNTIME_1_5_39) {
#endif
		logger::CRITICAL(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
	}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface * a_skse)
{
	SKSE::InitInfo info = SKSE::InitInfo();
	info.hook = true;
	info.log = true;
	info.logLevel = REX::ELogLevel::Trace;
	info.logName = Plugin::NAME.data();
	info.trampoline = true;
	info.trampolineSize = 14u;
	SKSE::Init(a_skse, info);
	logger::INFO("Author: SeaSparrow"sv);
	SECTION_SEPARATOR;

#ifdef SKYRIM_AE
	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_LATEST) {
		return false;
	}
#endif

	logger::INFO("Performing startup tasks..."sv);

	if (!Settings::INI::Read()) {
		REX::FAIL("Failed to load INI settings. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;

	SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	logger::INFO("Setting up serialization system..."sv);
	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);
	logger::INFO("  >Registered necessary functions."sv);
	SECTION_SEPARATOR;


	if (!Settings::JSON::Preload()) {
		REX::FAIL("Failed to preload JSON configs. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;
	return true;
}