#include "Serde.h"

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc) {
		logger::INFO("Starting save..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::CRITICAL("  >Failed to get internal serialization manager."sv);
			REX::FAIL(fmt::format("{}:  Failed to save. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Save(a_intfc)) {
			REX::FAIL(fmt::format("{}:  Failed to save. Check the log for more information.", Plugin::NAME));
		}
		logger::INFO("  >Save successful."sv);
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc) {
		logger::INFO("Starting load..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::CRITICAL("  >Failed to get internal serialization manager."sv);
			REX::FAIL(fmt::format("{}:  Failed to load. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Load(a_intfc)) {
			REX::FAIL(fmt::format("{}:  Failed to load. Check the log for more information.", Plugin::NAME));
		}
		logger::INFO("  >Load successful."sv);
	}

	void RevertCallback(SKSE::SerializationInterface* a_intfc) {
		logger::INFO("Starting revert..."sv);
		auto* serdeManager = ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::CRITICAL("  >Failed to get internal serialization manager."sv);
			REX::FAIL(fmt::format("{}:  Failed to revert. Check the log for more information.", Plugin::NAME));
		}
		if (!serdeManager->Revert(a_intfc)) {
			REX::FAIL(fmt::format("{}:  Failed to revert. Check the log for more information.", Plugin::NAME));
		}
		logger::INFO("  >Revert successful."sv);
	}

	bool ObjectManager::Save(SKSE::SerializationInterface* a_intfc) {
		bool success = true;
		if (recordObjectMap.empty()) {
			return success;
		}
		for (auto& obj : recordObjectMap) {
			bool serializableSuccess = obj.second && obj.second->Save(a_intfc);
			if (!serializableSuccess) {
				logger::CRITICAL("  >Serialization error reported for object: {}"sv, obj.second ? 
					DecodeTypeCode(obj.second->GetSerializationID()) : 
					"NULL");
			}
			success &= serializableSuccess;
		}
		return success;
	}

	bool ObjectManager::Load(SKSE::SerializationInterface* a_intfc) {
		bool success = true;
		if (recordObjectMap.empty()) {
			return success;
		}

		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;
		auto end = recordObjectMap.end();

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			auto it = recordObjectMap.find(type);
			if (it != end) {
				bool serializableSuccess = it->second && it->second->Load(a_intfc);
				if (!serializableSuccess) {
					logger::CRITICAL("  >Serialization error reported for object: {}"sv, DecodeTypeCode(type));
				}
				success &= serializableSuccess;
			}
		}
		return success;
	}

	bool ObjectManager::Revert(SKSE::SerializationInterface* a_intfc) {
		if (recordObjectMap.empty()) {
			return true;
		}

		bool success = true;
		for (auto& obj : recordObjectMap) {
			bool serializableSuccess = obj.second && obj.second->Revert(a_intfc);
			if (!serializableSuccess) {
				logger::CRITICAL("  >Serialization error reported for object: {}"sv, obj.second ?
					DecodeTypeCode(obj.second->GetSerializationID()) :
					"NULL");
			}
			success &= serializableSuccess;
		}
		return success;
	}

	void ObjectManager::RegisterObject(Serializable* a_newObject) {
		if (!a_newObject) {
			REX::FAIL("Attempted to register nullptr in Serialization Manager."sv);
		}
		auto recordType = a_newObject->GetSerializationID();
		if (recordObjectMap.contains(recordType)) {
			REX::FAIL(
				fmt::format("Duplicate serialization registration for record type {}", DecodeTypeCode(recordType)));
		}
		recordObjectMap.emplace(recordType, a_newObject);
	}

	void ObjectManager::UnRegisterObject(Serializable* a_object) {
		if (!a_object) {
			REX::FAIL("Attempted to register nullptr in Serialization Manager."sv);
		}
		auto it = recordObjectMap.find(a_object->GetSerializationID());
		if (it != recordObjectMap.end()) {
			recordObjectMap.erase(it);
		}
	}

	bool Serializable::Register(std::uint32_t a_id) {
		auto* manager = ObjectManager::GetSingleton();
		if (!manager) {
			logger::CRITICAL("Critical error registering serializable form: Internal ObjectManager returned invalid pointer."sv);
			return false;
		}

		serdeID = a_id;
		manager->RegisterObject(this);
		return true;
	}

	std::uint32_t Serializable::GetSerializationID() const { return serdeID; }

	bool Serializable::Save(SKSE::SerializationInterface* a_intfc) {
		(void)a_intfc;
		return false;
	}

	bool Serializable::Load(SKSE::SerializationInterface* a_intfc) {
		(void)a_intfc;
		return false;
	}

	bool Serializable::Revert(SKSE::SerializationInterface* a_intfc) {
		(void)a_intfc;
		return true;
	}
}