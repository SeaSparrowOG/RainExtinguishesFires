#include "papyrus.h"

#include "FireManipulation/Manipulator.h"

namespace Papyrus
{
	static std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	static bool IsRaining(STATIC_ARGS)
	{
		const auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			return false;
		}

		const auto* current = sky->currentWeather;
		if (!current) {
			return false;
		}

		const auto* last = sky->lastWeather;
		const auto& data = current->data;
		const auto* lastData = last ? &last->data : nullptr;
		if (data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
			if (!lastData) {
				return true;
			}

			if (lastData->flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
				return true;
			}

			const float fadeInPct = 1.0f + static_cast<float>(current->data.precipitationBeginFadeIn) / 255.0f;
			return fadeInPct <= sky->currentWeatherPct;
		}
		else if (lastData && lastData->flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
			const float fadeOutPct = static_cast<float>(current->data.precipitationEndFadeOut) / 255.0f;
			return fadeOutPct > sky->currentWeatherPct;
		}
		return false;
	}

	static void UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* ref) {
		static auto* manipulator = FireManipulator::Manipulator::GetSingleton();
		if (!manipulator) {
			a_vm->TraceStack("[UnFreezeFire]: Failed to get Fire Manipulator.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		if (!ref) {
			a_vm->TraceStack("[UnFreezeFire]: Cannot call this function with a NONE reference.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		manipulator->UnFreezeReference(ref);
	}

	static void FreezeFire(STATIC_ARGS, RE::TESObjectREFR* ref) {
		static auto* manipulator = FireManipulator::Manipulator::GetSingleton();
		if (!manipulator) {
			a_vm->TraceStack("[FreezeFire]: Failed to get Fire Manipulator.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		if (!ref) {
			a_vm->TraceStack("[FreezeFire]: Cannot call this function with a NONE reference.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		manipulator->FreezeReference(ref);
	}

	static void Bind(VM& a_vm) {
		logger::INFO("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::INFO("  >Binding IsRaining..."sv);
		BIND(IsRaining);
		logger::INFO("  >Binding ExtinguishAllLoadedFires..."sv);
		BIND(UnFreezeFire);
		logger::INFO("  >Binding RegisterForAllEvents..."sv);
		BIND(FreezeFire);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::INFO("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::INFO("Finished binding functions."sv);
		return true;
	}
}
