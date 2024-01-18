#pragma once

namespace INIParser {
	class INISettings : 
		public ISingleton<INISettings> {
	public:
		bool CheckForOcclusion();
		bool ReadSettings();
		bool SearchForLights();
		bool SearchForSmoke();
	private:
		bool searchForSmoke;
		bool searchForLights;
		bool checkOcclusion;
	};
}