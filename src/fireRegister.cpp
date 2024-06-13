#include "fireRegister.h"

namespace {
    template <typename T>
    bool IsFormInVector(const std::vector<T>* a_vec, T a_form) {
        return std::find(a_vec->begin(), a_vec->end(), a_form) != a_vec->end();
    }

    double GetBaseSize(RE::TESBoundObject* a_bound) {
        auto& boundData = a_bound->boundData;

        double baseX = boundData.boundMax.x - boundData.boundMin.x;
        double baseY = boundData.boundMax.y - boundData.boundMin.y;
        double baseZ = boundData.boundMax.z - boundData.boundMin.z;
        double result = std::hypot(baseX, baseY, baseZ);
        return result;
    }
}

namespace CachedData {
    const FireData* Fires::GetFireData(RE::TESBoundObject* a_form) {
        if (a_form && this->fireDataMap.contains(a_form)) return &this->fireDataMap[a_form];
        return nullptr;
    }

    bool Fires::IsFireObject(RE::TESBoundObject* a_form) {
        return IsLitFire(a_form) || IsUnLitFire(a_form);
    }

    bool Fires::IsLitFire(RE::TESBoundObject* a_form) {
        return IsFormInVector<RE::TESBoundObject*>(&this->validFires, a_form);
    }

    bool CachedData::Fires::IsUnLitFire(RE::TESBoundObject* a_form) {
        return IsFormInVector<RE::TESBoundObject*>(&this->validOffFires, a_form);
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

        fireData.disableLight = this->checkLight;
        fireData.lightLookupRadius = this->lookupLightRadius;
        fireData.disableSmoke = this->checkSmoke;
        fireData.smokeLookupRadius = this->lookupSmokeRadius;

        //Interesting ideas, bounding is wrong.
        //fireData.sizeFactor = std::floor(GetBaseSize(a_litForm) / GetBaseSize(fireData.offVersion) * 100.0) / 100.0;

        this->fireDataMap[a_litForm] = fireData;
        this->validFires.push_back(a_litForm);
        this->validOffFires.push_back(fireData.offVersion);
        if (fireData.dyndolodFire) this->dyndolodFires.push_back(fireData.dyndolodVersion);
    }

    void Fires::Report() {
        std::vector<std::pair<RE::TESBoundObject*, std::string>> orderedPairVector{};

        for (auto& pair : this->fireDataMap) {
            auto edid = _debugEDID(pair.first);
            if (edid.empty()) edid = std::to_string(pair.first->GetLocalFormID());
            auto newPair = std::make_pair(pair.first, edid);
            orderedPairVector.push_back(newPair);
        }
        std::sort(orderedPairVector.begin(), orderedPairVector.end(), [](std::pair<RE::TESBoundObject*, std::string>& a, std::pair<RE::TESBoundObject*, std::string>& b) {
            return a.second < b.second;
            });

        _loggerInfo("Finished reading settings. Registered {} valid fires.", this->fireDataMap.size());
        _loggerInfo("    Check Lights: {}\n    Check Smoke: {}\n    Light Radius: {}\n    Smoke Radius: {}", 
            this->checkLight, this->checkSmoke, this->lookupLightRadius, this->lookupSmokeRadius);
        _loggerInfo("----------------------------------------------------------------");

        std::vector<double> sizes{};
        for (auto& vecObj : orderedPairVector) {
            auto& pair = this->fireDataMap[vecObj.first];
            _loggerInfo("    >{}", vecObj.second);

            auto edid = _debugEDID(pair.offVersion);
            if (edid.empty()) edid = std::to_string(pair.offVersion->formID);
            _loggerInfo("        ->Off Version: {}", edid);

            //Currently unused.
            //_loggerInfo("        ->Size Factor: {}", pair.sizeFactor);

            if (!pair.dyndolodFire) continue;
            edid = _debugEDID(pair.dyndolodVersion);
            _loggerInfo("        ->DynDOLOD Version: {}", edid);
        }
    }
}