#pragma once

namespace FireManipulator
{
	static void Extinguish(RE::TESObjectREFR* fire);
	static void Relight(RE::TESObjectREFR* fire);
	static void ExtinguishCell(RE::TESObjectCELL* cell);

	class Manipulator final : 
		public REX::Singleton<Manipulator>,
		public SKSE::detail::TaskDelegate
	{
	public:
		void MassExtinguish(RE::TESObjectCELL* cell);
		void Extinguish(RE::TESObjectREFR* fire);
		void Relight(RE::TESObjectREFR* fire);

		void Run() override;
		void Dispose() override;

	private:
		struct PendingData
		{
			RE::TESBoundObject* unlit = nullptr;
			RE::TESObjectREFR* light = nullptr;
			RE::TESObjectREFR* smoke = nullptr;
			RE::TESObjectREFR* fire = nullptr;
		};

		using Stored = RE::FormID;
		std::unordered_set<Stored> _frozen;
		std::vector<PendingData>   _pending;

		bool running = false;

		void ExtinguishImpl(const PendingData& data);
	};
}