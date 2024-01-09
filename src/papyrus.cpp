#include "papyrus.h"
#include "fireRegister.h"
#include "settingsReader.h"

namespace Papyrus {
	/**
	* Returns the closest reference with the provided type.
	* @param a_center The reference from which to search.
	* @param a_radius The radius within which to search.
	* @param a_type The type of reference to check. 31 is lights, 36 Moveable statics.
	*/
	RE::TESObjectREFR* GetNearestReferenceOfType(RE::TESObjectREFR* a_center, float a_radius, RE::FormType a_type) {
		RE::TESObjectREFR* response = nullptr;
		bool found = false;
		auto mainLocation = a_center->data.location;
		float lastDistance = a_radius;

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			auto centerLocation = a_center->data.location;

			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR& a_ref) {
				if (a_ref.Is3DLoaded()) {
					auto* baseBound = a_ref.GetBaseObject();
					if (!baseBound) RE::BSContainer::ForEachResult::kContinue;
					if (baseBound->Is(a_type)) RE::BSContainer::ForEachResult::kContinue;
					if (a_ref.data.location.GetDistance(mainLocation) <= lastDistance) {
						found = true;
						lastDistance = a_ref.data.location.GetDistance(mainLocation);
						response = &a_ref;
					}
				}
				return RE::BSContainer::ForEachResult::kContinue;
				});
		}
		if (found) return response;
		return nullptr;
	}

	RE::TESObjectREFR* GetNearestMatchingDynDOLODFire(RE::TESObjectREFR* a_center, float a_radius, std::string a_name) {
		if (a_name.empty()) return nullptr;

		bool found = false;
		RE::TESObjectREFR* response = nullptr;

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			auto centerLocation = a_center->data.location;

			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR& a_ref) {
				if (a_ref.Is3DLoaded()) {
					auto* baseBound = a_ref.GetBaseObject();
					if (!baseBound) return RE::BSContainer::ForEachResult::kContinue;

					if (clib_util::editorID::get_editorID(baseBound->As<RE::TESForm>()).contains(a_name)) {
						response = &a_ref;
						found = true;
					}
				}
				return RE::BSContainer::ForEachResult::kContinue;
				});
		}

		if (found) return response;
		return nullptr;
	}

	bool Papyrus::IsRaining() { return this->isRaining; }

	void Papyrus::SendWeatherChangeEvent(RE::TESWeather* a_weather) {
		if (!a_weather) return;

		if (!currentWeather) {
			currentWeather = a_weather;
		}
		else {
			currentWeather = RE::Sky::GetSingleton()->lastWeather;
			if (!currentWeather) currentWeather = a_weather;
		}

		if (!FireRegistry::FireRegistry::GetSingleton()->IsValidLocation()) return;

		bool bWasRaining = false;

		if (currentWeather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy
			|| currentWeather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow) {
			bWasRaining = true;
		}

		bool bIsRaining = false;

		if (a_weather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy
			|| a_weather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow) {
			bIsRaining = true;
		}

		if (!bWasRaining && bIsRaining) {
			float weatherTransitionTime = 30.0f;
			float weatherPct = RE::Sky::GetSingleton()->currentWeatherPct;
			float precipitation = (- 0.2f) * a_weather->data.precipitationBeginFadeIn; //fadeBegin is int8_t
			float passedTime = weatherPct * weatherTransitionTime / 100.0f;
			float remainingTime = precipitation - passedTime + 4.0f;

			if (remainingTime < 0.0f) remainingTime = 0.0f;

			this->weatherTransition.QueueEvent(true, remainingTime);
			this->currentWeather = a_weather;
		}
		else if (bWasRaining && !bIsRaining) {
			this->weatherTransition.QueueEvent(false, 0.0f);
		}
	}

	void Papyrus::SendPlayerChangedInteriorExterior(bool a_movedToExterior) {
		this->movedToExterior.QueueEvent(a_movedToExterior);
	}

	void Papyrus::SendExtinguishEvent(RE::TESObjectREFR* a_fire, RE::TESForm* a_offVersion, bool a_dyndolodFire) {
		std::vector<RE::TESObjectREFR*> additionalExtinguishes = std::vector<RE::TESObjectREFR*>();

		auto* referenceExtraList = &a_fire->extraList;
		bool hasEnableChildren = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateChildren) : false;

		if (hasEnableChildren && !a_dyndolodFire) {
			return;
		}
		else if (hasEnableChildren && a_dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(a_fire->GetBaseObject()->As<RE::TESForm>());
			if (!editorID.contains("DYNDOLOD")) return;
		}

		if (a_dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(a_offVersion);
			auto* baseFire = a_fire->GetBaseObject()->As<RE::TESForm>();
			std::string baseEDID = clib_util::editorID::get_editorID(baseFire);

			if (!(baseEDID.empty() || baseEDID.contains("DYNDOLOD"))) {
				RE::TESObjectREFR* dyndolodREF = GetNearestMatchingDynDOLODFire(a_fire, 200.0f, baseEDID + "_DYNDOLOD_BASE");
				if (!dyndolodREF) {
					additionalExtinguishes.push_back(dyndolodREF);
				}
			}
		}

		//TODO: This.
		/*
		auto* settingsSingleton = Settings::Settings::GetSingleton();

		if (settingsSingleton->SearchForLights()) {
			auto* response = GetNearestReferenceOfType(a_fire, 200.0f, RE::FormType::Light);
			if (response) additionalExtinguishes.push_back(response);
		}

		if (settingsSingleton->SearchForSmoke()) {
			auto* response = GetNearestReferenceOfType(a_fire, 200.0f, RE::FormType::MovableStatic);
			if (response) additionalExtinguishes.push_back(response);
		}
		*/

		this->extinguishFire.QueueEvent(a_fire, a_offVersion, additionalExtinguishes);
	}

	void Papyrus::SendRelightEvent(RE::TESObjectREFR* a_fire) {
		this->relightFire.QueueEvent(a_fire);
	}

	void Papyrus::SetIsRaining(bool a_isRaining) { this->isRaining = a_isRaining; }

	void Papyrus::RegisterFormForWeatherEvent(const RE::TESForm* a_form) {
		this->weatherTransition.Register(a_form);
	}

	void Papyrus::RegisterFormForExtinguishEvent(const RE::TESForm* a_form) {
		this->extinguishFire.Register(a_form);
	}

	void Papyrus::RegisterFormForCellChangeEvent(const RE::TESForm* a_form) {
		this->movedToExterior.Register(a_form);
	}

	void Papyrus::RegisterFormForRelightEvent(const RE::TESForm* a_form) {
		this->relightFire.Register(a_form);
	}

	std::vector<int> GetVersion(STATIC_ARGS) {
		std::vector<int> response;
		response.push_back(5);
		response.push_back(0);
		response.push_back(0);

		return response;
	}

	void RegisterForExtinguishEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->RegisterFormForExtinguishEvent(a_form);
	}

	void RegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->RegisterFormForWeatherEvent(a_form);
	}

	void RegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->RegisterFormForCellChangeEvent(a_form);
	}

	void RegisterForRelightEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->RegisterFormForRelightEvent(a_form);
	}

	void ExtinguishAllLoadedFires(STATIC_ARGS) {
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(RE::PlayerCharacter::GetSingleton(), 0.0f, [&](RE::TESObjectREFR& a_ref) {
				if (!a_ref.Is3DLoaded()) return RE::BSContainer::ForEachResult::kContinue;

				auto* currentRef = &a_ref;
				auto* referenceBoundObject = currentRef ? currentRef->GetBaseObject() : nullptr;
				auto* referenceBaseObject = referenceBoundObject ? referenceBoundObject->As<RE::TESForm>() : nullptr;
				if (!referenceBaseObject) return RE::BSContainer::ForEachResult::kContinue;

				auto offVersion = FireRegistry::FireRegistry::GetSingleton()->GetMatch(referenceBaseObject);
				auto* offForm = offVersion.offVersion;
				if (!offForm) return RE::BSContainer::ForEachResult::kContinue;

				Papyrus::Papyrus::GetSingleton()->SendExtinguishEvent(currentRef, offForm, offVersion.dyndolodFire);
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
	}

	void SetRainingFlag(STATIC_ARGS, bool a_isRaining) {
		Papyrus::GetSingleton()->SetIsRaining(a_isRaining);
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
		BIND_EVENT(RegisterForAccurateWeatherChange, true);
		BIND_EVENT(RegisterForExtinguishEvent, true);
		BIND_EVENT(RegisterForPlayerCellChangeEvent, true);
		BIND_EVENT(RegisterForRelightEvent, true);
		BIND(ExtinguishAllLoadedFires);
		BIND(SetRainingFlag);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}