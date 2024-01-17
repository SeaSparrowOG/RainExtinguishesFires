#include "papyrus.h"
#include "fireRegister.h"
#include "settingsReader.h"

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
	RE::TESObjectREFR* GetNearestReferenceOfType(RE::TESObjectREFR* a_center, float a_radius, RE::FormType a_type, std::vector<RE::TESForm*> a_matchAgainst) {
		RE::TESObjectREFR* response = nullptr;
		bool found = false;
		float lastDistance = a_radius;
		auto refLocation = a_center->GetPosition();

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR& a_ref) {
				const auto baseBound = a_ref.GetBaseObject();
				if (!baseBound) return RE::BSContainer::ForEachResult::kContinue;
				auto* baseForm = baseBound->As<RE::TESForm>();
				if (!baseForm) return RE::BSContainer::ForEachResult::kContinue;
				if (!baseBound->Is(a_type)) return RE::BSContainer::ForEachResult::kContinue;
				if (!a_matchAgainst.empty() && !IsFormInVector(baseForm, a_matchAgainst)) return RE::BSContainer::ForEachResult::kContinue;

				auto lightLocation = a_ref.GetPosition();
				float currentDistance = lightLocation.GetDistance(refLocation);
				if (currentDistance > lastDistance) return RE::BSContainer::ForEachResult::kContinue;

				found = true;
				response = &a_ref;
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
				auto* baseBound = a_ref.GetBaseObject();
				if (!baseBound) return RE::BSContainer::ForEachResult::kContinue;

				if (clib_util::editorID::get_editorID(baseBound->As<RE::TESForm>()).contains(a_name)) {
					response = &a_ref;
					found = true;
				}
				return RE::BSContainer::ForEachResult::kContinue;
				});
		}

		if (found) return response;
		return nullptr;
	}

	RE::NiPoint3 angles2dir(const RE::NiPoint3& angles) {
		RE::NiPoint3 ans;

		float sinx = sinf(angles.x);
		float cosx = cosf(angles.x);
		float sinz = sinf(angles.z);
		float cosz = cosf(angles.z);

		ans.x = cosx * sinz;
		ans.y = cosx * cosz;
		ans.z = -sinx;

		return ans;
	}

	RE::NiPoint3 rotate(float r, const RE::NiPoint3& angles) { return angles2dir(angles) * r; }

	bool IsOccluded(RE::TESObjectREFR* a_fire) {
		if (!Settings::Settings::GetSingleton()->CheckForOcclusion()) return false;

		auto* fireCell = a_fire->GetParentCell();
		const auto bhkWorld = fireCell ? fireCell->GetbhkWorld() : nullptr;
		if (!bhkWorld) return false;

		auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		RE::bhkPickData pick_data;
		RE::NiPoint3 ray_start, ray_end;

		ray_start = a_fire->data.location + rotate(150.0f, a_fire->data.angle);
		ray_end = ray_start + rotate(450.0f, a_fire->data.angle);
		pick_data.rayInput.from = ray_start * havokWorldScale;
		pick_data.rayInput.to = ray_end * havokWorldScale;
		pick_data.rayInput.filterInfo = SKSE::stl::to_underlying(RE::COL_LAYER::kStatic); //RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup() << 16 | SKSE::stl::to_underlying(RE::COL_LAYER::kStatic);

		if (bhkWorld->PickObject(pick_data); pick_data.rayOutput.HasHit()) return true;
		return false;
	}

	bool Papyrus::IsRaining() { return this->isRaining; }

	void Papyrus::SendWeatherChangeEvent(RE::TESWeather* a_weather) {
		if (!a_weather) return;

		if (!currentWeather) {
			currentWeather = RE::Sky::GetSingleton()->lastWeather;
		}

		if (!currentWeather) return;

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
		this->movedToExterior.QueueEvent(a_movedToExterior);
	}

	void Papyrus::SendExtinguishEvent(RE::TESObjectREFR* a_fire, RE::TESForm* a_offVersion, bool a_dyndolodFire, bool a_force) {
		if (this->frozenFiresRegister.find(a_fire) != this->frozenFiresRegister.end()) {
			return;
		}
		this->frozenFiresRegister[a_fire] = true;
		std::vector<RE::TESObjectREFR*> additionalExtinguishes = std::vector<RE::TESObjectREFR*>();

		auto* referenceExtraList = &a_fire->extraList;
		bool hasEnableChildren = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateChildren) : false;
		bool hasEnableParents = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateParent) : false;

		if (hasEnableParents || (hasEnableChildren && !a_dyndolodFire)) {
			this->frozenFiresRegister.erase(a_fire);
			return;
		}
		else if (hasEnableChildren && a_dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(a_fire->GetBaseObject()->As<RE::TESForm>());
			if (!editorID.contains("DYNDOLOD")) {
				this->frozenFiresRegister.erase(a_fire);
				return;
			}
		}

		auto* settingsSingleton = Settings::Settings::GetSingleton();
		std::vector<RE::TESForm*> validSmoke = std::vector<RE::TESForm*>();

		if (!a_force && settingsSingleton->CheckForOcclusion()) {
			if (IsOccluded(a_fire)) {
				this->frozenFiresRegister.erase(a_fire);
				return;
			}
		}

		if (settingsSingleton->SearchForLights()) {
			auto foundLight = GetNearestReferenceOfType(a_fire, 300.0f, RE::FormType::Light, validSmoke);
			if (foundLight) {
				_loggerInfo("Found light {}.",clib_util::editorID::get_editorID(foundLight->GetBaseObject()));
				additionalExtinguishes.push_back(foundLight);
			}
		}

		if (settingsSingleton->SearchForSmoke()) {
			validSmoke = FireRegistry::FireRegistry::GetSingleton()->GetMatch(a_fire->GetBaseObject()->As<RE::TESForm>()).validSmokes;
			auto foundSmoke = GetNearestReferenceOfType(a_fire, 300.0f, RE::FormType::MovableStatic, validSmoke);
			if (foundSmoke) additionalExtinguishes.push_back(foundSmoke);
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
		this->extinguishFire.QueueEvent(a_fire, a_offVersion, additionalExtinguishes);
	}

	void Papyrus::SendRelightEvent(RE::TESObjectREFR* a_fire, bool a_bForce) {
		if (this->frozenFiresRegister.find(a_fire) != this->frozenFiresRegister.end()) {
			return;
		}
		this->frozenFiresRegister[a_fire] = true;
		this->relightFire.QueueEvent(a_fire, a_bForce);
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

	void Papyrus::RemoveFireFromRegistry(RE::TESObjectREFR* a_fire) {
		if (this->frozenFiresRegister.find(a_fire) != this->frozenFiresRegister.end()) {
			this->frozenFiresRegister.erase(a_fire);
		}
	}

	void Papyrus::ResetFrozenMaps() {
		this->frozenFiresRegister.clear();
	}

	void Papyrus::AddFireToRegistry(RE::TESObjectREFR* a_fire) {
		if (this->frozenFiresRegister.find(a_fire) == this->frozenFiresRegister.end()) {
			frozenFiresRegister[a_fire] = true;
		}
	}

	std::vector<int> GetVersion(STATIC_ARGS) {
		std::vector<int> response;
		response.push_back(5);
		response.push_back(0);
		response.push_back(0);

		return response;
	}

	bool IsRaining(STATIC_ARGS) {
		return Papyrus::GetSingleton()->IsRaining();
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

	void FreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
		if (!a_fire || !a_fire->Is3DLoaded()) return;
		Papyrus::GetSingleton()->AddFireToRegistry(a_fire);
	}

	void UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
		Papyrus::GetSingleton()->RemoveFireFromRegistry(a_fire);
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
		BIND(FreezeFire);
		BIND(UnFreezeFire);
		BIND(IsRaining);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}