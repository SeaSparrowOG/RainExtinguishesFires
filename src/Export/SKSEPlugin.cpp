#include "Cache/FormCache.h"
#include "Data/ModObjectManager.h"
#include "FireManipulation/Manipulator.h"
#include "Papyrus/Papyrus.h"
#include "Serialization/Serde.h"
#include "Settings/INI/INISettings.h"
#include "Settings/JSON/JSONSettings.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

// Extracted from Commonlib. I need to log stuff before SKSE::Init.
static void SetupLog()
{

#ifndef NDEBUG
	#define SPD_LOG_LEVEL spdlog::level::debug
#else
	#define SPD_LOG_LEVEL spdlog::level::info
#endif

	auto path = SKSE::log::log_directory();
	if (!path)
		return;

	*path /= std::format("{}.log", Plugin::NAME);

	std::vector<spdlog::sink_ptr> sinks{
		std::make_shared<spdlog::sinks::msvc_sink_mt>()
	};
	sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));

	auto logger = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
	logger->set_level(SPD_LOG_LEVEL);
	logger->flush_on(SPD_LOG_LEVEL);

	spdlog::set_default_logger(std::move(logger));
	spdlog::set_pattern("[%T.%e] [%=5t] [%L] %v");

	REX::INFO("{} v{}", Plugin::NAME, Plugin::VERSION.string());
}

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	static auto* jsonHolder = Settings::JSON::Holder::GetSingleton();
	if (!jsonHolder) {
		REX::FAIL("Failed to get internal JSON logger."sv);
	}

	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		if (!Data::PreloadModObjects()) {
			REX::FAIL(
				fmt::format("Failed to preload mod objects. Check the log at Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
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
		REX::INFO("Finished startup tasks, enjoy your game!"sv);
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v{};

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("SeaSparrow"sv);
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();

	return v;
}();

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	return true;
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface * a_skse)
{
	SetupLog();

	SECTION_SEPARATOR;
	if (!Settings::JSON::Preload()) {
#ifdef NDEBUG
		REX::FAIL(
			fmt::format("Failed to parse configs. Check the log (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
#endif
	}

	if (!Settings::INI::Read()) {
		REX::FAIL(
			fmt::format("Failed to load the INI settings. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
	}
	SECTION_SEPARATOR;

	SKSE::InitInfo info;
	info.log = true;
	info.hook = true;
	info.trampoline = true;
	info.trampolineSize = 28u;
	
	SKSE::Init(a_skse, info);
	REX::INFO("Author: SeaSparrow"sv);
	SECTION_SEPARATOR;

	const auto ver = a_skse->RuntimeVersion();

	static constexpr std::array<REL::Version, 2> supported = 
	{
		SKSE::RUNTIME_SSE_1_7_104,
		SKSE::RUNTIME_SSE_1_7_99
	};

	if (!std::ranges::contains(supported, ver)) {
		REX::CRITICAL("Game Version: {}"sv, ver.string());
		REX::CRITICAL("Supported Versions:"sv);
		for (const auto& allowed : supported) {
			REX::CRITICAL("  - {}"sv, allowed.string());
		}
		REX::FAIL(
			fmt::format("You are using a version not supported by this plugin. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME)
		);
	}

	REX::INFO("Performing startup tasks..."sv);
	SECTION_SEPARATOR;
	if (!Papyrus::RegisterFunctions()) {
		REX::FAIL(
			fmt::format("Failed to register the new Papyrus functions. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
	}

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	return true;
}