#include "papyrus.h"

namespace Papyrus {
	bool IsFormInVector(RE::TESForm* a_form, std::vector<RE::TESForm*> a_vec) {
		if (!a_vec.empty()) {
			for (auto* form : a_vec) {
				if (form == a_form) return true;
			}
		}
		return false;
	}

	/**
	* Returns the closest reference with the provided type.
	* @param a_center The reference from which to search.
	* @param a_radius The radius within which to search.
	* @param a_type The type of reference to check. 31 is lights, 36 Moveable statics.
	*/
	RE::TESObjectREFR* GetNearestReferenceOfType(RE::TESObjectREFR* a_center, float a_radius, RE::FormType a_type, RE::TESForm* a_matchAgainst = nullptr) {
		RE::TESObjectREFR* response = nullptr;
		bool found = false;
		float lastDistance = a_radius;
		auto refLocation = a_center->GetPosition();

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR* a_ref) {
				const auto baseBound = a_ref->GetBaseObject();
				if (!baseBound) return RE::BSContainer::ForEachResult::kContinue;
				auto* baseForm = baseBound->As<RE::TESForm>();
				if (!baseForm) return RE::BSContainer::ForEachResult::kContinue;
				if (!baseBound->Is(a_type)) return RE::BSContainer::ForEachResult::kContinue;
				if (a_matchAgainst && a_matchAgainst == baseForm) return RE::BSContainer::ForEachResult::kContinue;

				auto lightLocation = a_ref->GetPosition();
				float currentDistance = lightLocation.GetDistance(refLocation);
				if (currentDistance > lastDistance) return RE::BSContainer::ForEachResult::kContinue;

				found = true;
				response = a_ref;
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

			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR* a_ref) {
				auto* baseBound = a_ref->GetBaseObject();
				if (!baseBound) return RE::BSContainer::ForEachResult::kContinue;

				if (clib_util::editorID::get_editorID(baseBound->As<RE::TESForm>()).contains(a_name)) {
					response = a_ref;
					found = true;
				}
				return RE::BSContainer::ForEachResult::kContinue;
				});
		}

		if (found) return response;
		return nullptr;
	}

	void Papyrus::Papyrus::AddWeatherChangeListener(const RE::TESForm* a_form, bool a_listen) {
		if (this->disable) return;

		if (a_listen) {
			this->weatherTransition.Register(a_form);
		}
		else {
			this->weatherTransition.Unregister(a_form);
		}
	}

	void Papyrus::Papyrus::AddInteriorExteriorListener(const RE::TESForm* a_form, bool a_listen) {
		if (this->disable) return;
		if (a_listen) {
			this->movedToExterior.Register(a_form);
		}
		else {
			this->movedToExterior.Unregister(a_form);
		}
	}

	void Papyrus::Papyrus::RelightFire(RE::TESObjectREFR* a_litFire) {
		auto trick = a_litFire->GetPositionX();
		if (this->disable) return;
	}

	void Papyrus::SendWeatherChange(RE::TESWeather* a_weather) {
		if (this->disable) return;
		if (!a_weather) return;

		if (!currentWeather) {
			currentWeather = RE::Sky::GetSingleton()->lastWeather;
		}

		if (!currentWeather) return;
		
		auto* playerCell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
		if (!playerCell) return;
		if (playerCell->IsInteriorCell()) return;

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

		float weatherTransitionTime = 25.0f;
		float weatherPct = RE::Sky::GetSingleton()->currentWeatherPct;

		if (!bWasRaining && bIsRaining) {
			float precipitation = (- 0.2f) * a_weather->data.precipitationBeginFadeIn; //fadeBegin is int8_t
			float passedTime = weatherPct * weatherTransitionTime / 100.0f;
			float remainingTime = precipitation - passedTime;

			if (remainingTime < 1.0f) remainingTime = 1.0f;

			this->weatherTransition.QueueEvent(true, remainingTime);
			this->currentWeather = a_weather;
		}
		else if (bWasRaining && !bIsRaining) {
			float clearPrecipitation = (-0.2f) * a_weather->data.precipitationEndFadeOut;
			float passedTime = weatherPct * weatherTransitionTime / 100.0f;
			float remainingTime = clearPrecipitation - passedTime;

			if (remainingTime < 1.0f) remainingTime = 1.0f;

			this->weatherTransition.QueueEvent(false, remainingTime);
			this->currentWeather = a_weather;
		}
	}

	void Papyrus::SendPlayerChangedInteriorExterior(bool a_movedToExterior) {
		if (this->disable) return;
		this->movedToExterior.QueueEvent(a_movedToExterior);
	}

	void Papyrus::ExtinguishFire(RE::TESObjectREFR* a_fire, CachedData::FireData a_data) {
		if (this->disable) return;
		if (this->frozenFiresRegister.find(a_fire) != this->frozenFiresRegister.end()) {
			return;
		}

		auto* settingsSingleton = CachedData::FireRegistry::GetSingleton();
		auto offData = settingsSingleton->GetOffForm(a_fire);
		auto* offForm = offData.offVersion;

		if (!offForm) return;
		this->frozenFiresRegister[a_fire] = true;
		std::vector<RE::TESObjectREFR*> additionalExtinguishes = std::vector<RE::TESObjectREFR*>();

		auto* referenceExtraList = &a_fire->extraList;
		bool hasEnableChildren = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateChildren) : false;
		bool hasEnableParents = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateParent) : false;
		bool dyndolodFire = a_data.dyndolodFire;

		if (hasEnableParents || (hasEnableChildren && !dyndolodFire)) {
			this->frozenFiresRegister.erase(a_fire);
			return;
		}
		else if (hasEnableChildren && dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(a_fire->GetBaseObject()->As<RE::TESForm>());
			if (!editorID.contains("DYNDOLOD")) {
				this->frozenFiresRegister.erase(a_fire);
				return;
			}
		}

		std::vector<RE::TESForm*> validSmoke = settingsSingleton->GetStoredSmokeObjects();
		
		if (settingsSingleton->GetCheckOcclusion()) {
			//TODO: Include this.
		}

		if (settingsSingleton->GetCheckLights()) {
			auto foundLight = GetNearestReferenceOfType(a_fire, 300.0f, RE::FormType::Light);
			if (foundLight) {
				additionalExtinguishes.push_back(foundLight);
			}
		}

		if (settingsSingleton->GetCheckSmoke()) {
			for (auto* smokeObject : validSmoke) {
				auto foundSmoke = GetNearestReferenceOfType(a_fire, 300.0f, RE::FormType::MovableStatic, smokeObject);
				if (foundSmoke) additionalExtinguishes.push_back(foundSmoke);
			}
		}

		if (dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(offForm);
			auto* baseFire = a_fire->GetBaseObject()->As<RE::TESForm>();
			std::string baseEDID = clib_util::editorID::get_editorID(baseFire);

			if (!(baseEDID.empty() || baseEDID.contains("DYNDOLOD"))) {
				RE::TESObjectREFR* dyndolodREF = GetNearestMatchingDynDOLODFire(a_fire, 200.0f, baseEDID + "_DYNDOLOD_BASE");
				if (!dyndolodREF) {
					additionalExtinguishes.push_back(dyndolodREF);
				}
			}
		}
		//TODO: Extinguish this fire.
	}

	void Papyrus::SetIsRaining(bool a_isRaining) {
		if (this->disable) return;
		this->isRaining = a_isRaining; 
	}

	void Papyrus::Papyrus::DisablePapyrus() {
		this->disable = true;
	}

	std::vector<int> GetVersion(STATIC_ARGS) {
		std::vector<int> response;
		response.push_back(Version::MAJOR);
		response.push_back(Version::MINOR);
		response.push_back(Version::PATCH);

		return response;
	}

	void RegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->AddWeatherChangeListener(a_form, true);
	}

	void UnRegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->AddWeatherChangeListener(a_form, false);
	}

	void RegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->AddInteriorExteriorListener(a_form, true);
	}

	void UnRegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Papyrus::GetSingleton()->AddInteriorExteriorListener(a_form, false);
	}

	void ExtinguishAllLoadedFires(STATIC_ARGS) {
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(RE::PlayerCharacter::GetSingleton(), 0.0f, [&](RE::TESObjectREFR* a_ref) {
				if (!a_ref->Is3DLoaded()) return RE::BSContainer::ForEachResult::kContinue;

				auto* referenceBoundObject = a_ref ? a_ref->GetBaseObject() : nullptr;
				auto* referenceBaseObject = referenceBoundObject ? referenceBoundObject->As<RE::TESForm>() : nullptr;
				if (!referenceBaseObject) return RE::BSContainer::ForEachResult::kContinue;
				if (CachedData::FireRegistry::GetSingleton()->IsManagedFire(a_ref)) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				CachedData::FireData fireData = CachedData::FireRegistry::GetSingleton()->GetOffForm(referenceBaseObject);
				Papyrus::Papyrus::GetSingleton()->ExtinguishFire(a_ref, fireData);
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
	}

	void SetRainingFlag(STATIC_ARGS, bool a_isRaining) {
		Papyrus::GetSingleton()->SetIsRaining(a_isRaining);
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
		BIND(ExtinguishAllLoadedFires);
		BIND(SetRainingFlag);
		BIND_EVENT(RegisterForAccurateWeatherChange, true);
		BIND_EVENT(RegisterForPlayerCellChangeEvent, true);
		BIND_EVENT(UnRegisterForAccurateWeatherChange, true);
		BIND_EVENT(UnRegisterForPlayerCellChangeEvent, true);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}