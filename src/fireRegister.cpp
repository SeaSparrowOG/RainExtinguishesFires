#include "fireRegister.h"

namespace {
    template <typename T>
    bool IsFormInVector(const std::vector<T>* a_vec, T a_form) {
        return std::find(a_vec->begin(), a_vec->end(), a_form) != a_vec->end();
    }
}

namespace CachedData {
    const FireData* Fires::GetFireData(RE::TESBoundObject* a_form) {
        if (this->fireDataMap.contains(a_form)) return &this->fireDataMap[a_form];
        return nullptr;
    }

    bool Fires::IsFireObject(RE::TESBoundObject* a_form) {
        return IsFormInVector<RE::TESBoundObject*>(&this->validFires, a_form);
    }

    bool Fires::IsSmokeObject(RE::TESBoundObject* a_form) {
        return IsFormInVector<RE::TESBoundObject*>(&this->smokeVector, a_form);
    }

    bool Fires::IsDynDOLODFire(RE::TESBoundObject* a_form) {
        return IsFormInVector<RE::TESBoundObject*>(&this->dyndolodFires, a_form);
    }

    void Fires::RegisterSmokeObject(RE::TESBoundObject* a_form) {
        if (IsFormInVector<RE::TESBoundObject*>(&this->smokeVector, a_form)) return;
        this->smokeVector.push_back(a_form);
    }

    void Fires::UpdateSetting(Setting a_setting, bool a_settingBool, double a_settingDouble) {
        switch (a_setting) {
        case kLightEnabled:
            this->checkLight = a_settingBool;
            break;
        case kSmokeEnabled:
            this->checkSmoke = a_settingBool;
            break;
        case kLightRadius:
            if (a_settingDouble < 50.0) a_settingDouble = 50.0;
            if (a_settingDouble > 1000.0) a_settingDouble = 1000.0;
            this->lookupLightRadius = a_settingDouble;
            break;
        case kSmokeRadius:
            if (a_settingDouble < 50.0) a_settingDouble = 50.0;
            if (a_settingDouble > 1000.0) a_settingDouble = 1000.0;
            this->lookupSmokeRadius = a_settingDouble;
            break;
        case kReferenceRadius:
            if (a_settingDouble < 50.0) a_settingDouble = 50.0;
            if (a_settingDouble > 1000.0) a_settingDouble = 1000.0;
            this->lookupReferenceRadius = a_settingDouble;
            break;
        case kResetTime:
            if (a_settingDouble < 0.1) a_settingDouble = 0.1;
            if (a_settingDouble > 100.0) a_settingDouble = 100.0;
            this->requiredOffTime = a_settingDouble;
            break;
        default:
            break;
        }
    }

    void Fires::RegisterPair(RE::TESBoundObject* a_litForm, FireData fireData) {
        if (!(a_litForm && fireData.offVersion)) return;
        if (this->fireDataMap.contains(a_litForm)) return;

        bool hasUniqueLightData = fireData.disableLight;
        if (!hasUniqueLightData) {
            fireData.disableLight = this->checkLight;
            fireData.lightLookupRadius = this->lookupLightRadius;
        }
        else {
            if (fireData.lightLookupRadius < 50.0) fireData.lightLookupRadius = 50.0;
            if (fireData.lightLookupRadius > 1000.0) fireData.lightLookupRadius = 1000.0;
        }

        bool hasUniqueSmokeData = fireData.disableSmoke;
        if (!hasUniqueSmokeData) {
            fireData.disableSmoke = this->checkSmoke;
            fireData.smokeLookupRadius = this->lookupSmokeRadius;
        }
        else {
            if (fireData.smokeLookupRadius < 50.0) fireData.smokeLookupRadius = 50.0;
            if (fireData.smokeLookupRadius > 1000.0) fireData.smokeLookupRadius = 1000.0;
        }

        this->fireDataMap[a_litForm] = fireData;
    }
}