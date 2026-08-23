#pragma once

namespace Data
{
	template <typename T, RE::FormID ID, SKSE::stl::nttp::string File>
	struct CachedForm
	{
		[[nodiscard]] static T* get()
		{
			static constinit T* form = nullptr;
			if (!form) {
				if (const auto dataHandler = RE::TESDataHandler::GetSingleton()) {
					constexpr std::string_view modName{ File.data(), File.size() };
					form = dataHandler->LookupForm<T>(ID, modName);
				}
			}
			return form;
		}

		[[nodiscard]] static bool resolved() { return get() != nullptr; }

		operator bool() const { return resolved(); }
		operator T* () const { return get(); }

		T* operator->() const { return get(); }
	};

	namespace Literals
	{
		template <SKSE::stl::nttp::string S>
		inline auto operator""_gv()
		{
			static constinit RE::TESGlobal* global = nullptr;
			if (!global) {
				constexpr std::string_view name{ S.data(), S.size() };
				global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(name);
			}
			return global ? std::optional(global->value) : std::nullopt;
		}
	}
}

using namespace Data::Literals;