#pragma once

namespace Settings {
	class Settings : public ISingleton<Settings> {
	public:
		bool CheckForOcclusion();
		bool ReadSettings();
		bool SearchForLights();
		bool SearchForSmoke();

	private:
		bool checkForOcclusion;
		bool searchForLights;
		bool searchForSmoke;
	};
}