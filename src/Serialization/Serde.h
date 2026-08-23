#pragma once

namespace Serialization
{
	constexpr std::uint32_t Version = 1;
	inline constexpr std::uint32_t ID = 'REFI';
	static_assert(Plugin::NAME == "TemplateProject"sv || ID != 'TRJT', "Make sure to use a unique ID for serialization.");

	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);
	void RevertCallback(SKSE::SerializationInterface* a_intfc);

	/// <summary>
	/// Important notes:
	/// * If Save, Load, or Revert fail, then the application state is considered corrupted and irrecoverable. Thus, the game closes.
	/// * When calling Register, provide an a_id that is unique for EACH class. If a duplicate insertion is attempted, the game closes.
	/// * Make sure to provide logging for each Save/Load/Revert failure as details. ObjectManager does note some things.
	/// * A pointer to a Serializable class is stored in the ObjectManager singleton after it is registered. To move/delete/whatever, first Unregister it from the ObjectManager.
	/// * Ideally, also inherit from REX::Singleton to deal with lifetime issues.
	/// </summary>
	class Serializable
	{
	public:
		virtual bool Save(SKSE::SerializationInterface* a_intfc);
		virtual bool Load(SKSE::SerializationInterface* a_intfc);
		virtual bool Revert(SKSE::SerializationInterface* a_intfc);

		bool Register(std::uint32_t a_id);
		std::uint32_t GetSerializationID() const;

		virtual ~Serializable() = default;
	private:
		std::uint32_t serdeID{ 0 };
	};

	class ObjectManager : public REX::Singleton<ObjectManager>
	{
	public:
		bool Save(SKSE::SerializationInterface* a_intfc);
		bool Load(SKSE::SerializationInterface* a_intfc);
		bool Revert(SKSE::SerializationInterface* a_intfc);

		void RegisterObject(Serializable* a_newObject);
		void UnRegisterObject(Serializable* a_object);

	private:
		std::unordered_map<uint32_t, Serializable*> recordObjectMap{};
	};

	/// <summary>
	/// Debug tool. When encountering unexpected RecordTypes, converts them to a readable string (HDEC, STEN, etc).
	/// </summary>
	/// <param name="a_typeCode">The unexpected record type.</param>
	/// <returns>The unexpected record type as a string.</returns>
	inline static std::string DecodeTypeCode(std::uint32_t a_typeCode)
	{
		std::string result(4, '\0');

		// Extract bytes from most significant to least
		result[0] = static_cast<char>((a_typeCode >> 24) & 0xFF);
		result[1] = static_cast<char>((a_typeCode >> 16) & 0xFF);
		result[2] = static_cast<char>((a_typeCode >> 8) & 0xFF);
		result[3] = static_cast<char>(a_typeCode & 0xFF);

		return result;
	}

	/// <summary>
	/// Helper function. Encodes a string into the interface.
	/// </summary>
	/// <param name="a_intfc">The serialization interface provided by SKSE.</param>
	/// <param name="a_str">The string to serialize.</param>
	/// <returns>True if encoding is successful, false otherwise.</returns>
	inline static bool WriteString(SKSE::SerializationInterface* a_intfc,
		const std::string& a_str)
	{
		std::size_t size = a_str.length() + 1;
		return a_intfc->WriteRecordData(size) && a_intfc->WriteRecordData(a_str.data(), static_cast<std::uint32_t>(size));
	}

	/// <summary>
	/// Helper function. Decodes a string from the interface, and stores it in a given variable.
	/// </summary>
	/// <param name="a_intfc">The serialization interface provided by SKSE.</param>
	/// <param name="a_str">The result is stored here.</param>
	/// <returns>True if successful, false otherwise.</returns>
	inline static bool ReadString(SKSE::SerializationInterface* a_intfc,
		std::string& a_str)
	{
		std::size_t size = 0;
		if (!a_intfc->ReadRecordData(size)) {
			return false;
		}
		a_str.reserve(size);
		a_str.resize(size);
		if (!a_intfc->ReadRecordData(a_str.data(), static_cast<std::uint32_t>(size))) {
			return false;
		}
		if (!a_str.empty() && a_str.back() == '\0') {
			a_str.pop_back();
		}
		return true;
	}

	enum class GetFormFromInterfaceResult
	{
		Success,
		ReadError,
		NoForm,
		IncorrectType
	};

	inline static std::string GetFormFromInterfaceResult_ToString(GetFormFromInterfaceResult a_flag) {
		switch (a_flag) {
		case GetFormFromInterfaceResult::Success: return "Success";
		case GetFormFromInterfaceResult::NoForm: return "NoForm";
		case GetFormFromInterfaceResult::IncorrectType: return "IncorrectType";
		default: return "ReadError";
		}
	}

	template <typename T>
	struct RetrievedForm
	{
		std::optional<T*> form{ std::nullopt };
		GetFormFromInterfaceResult status{ GetFormFromInterfaceResult::ReadError };
	};

	/// <summary>
	/// Decodes the form contained in the Serialization Interface.
	/// </summary>
	/// <typeparam name="T">The type of the form.</typeparam>
	/// <param name="a_intfc">The SKSE Serialization Interface.</param>
	/// <returns>A RetrievedForm struct.</returns>
	template <typename T>
	inline static RetrievedForm<T> GetFormFromInterface(SKSE::SerializationInterface* a_intfc) {
		RetrievedForm<T> response{};
		RE::FormID oldID = 0;
		if (!a_intfc->ReadRecordData(oldID)) {
			return response;
		}
		RE::FormID newID = 0;
		if (!a_intfc->ResolveFormID(oldID, newID)) {
			return response;
		}

		RE::TESForm* retrieved = RE::TESForm::LookupByID(newID);
		if (!retrieved) {
			response.status = GetFormFromInterfaceResult::NoForm;
			return response;
		}

		auto* converted = retrieved->As<T>();
		if (!converted) {
			response.status = GetFormFromInterfaceResult::IncorrectType;
			return response;
		}

		response.form = converted;
		response.status = GetFormFromInterfaceResult::Success;
		return response;
	}
}