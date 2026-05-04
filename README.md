## License
As of v6.0.0, Rain Extinguishes Fires is AGPLv3. Previous versions were Apache 2.0, and will remain available should you need them for any reason. Nexus permissions remain unchanged.

## Rain Extinguishes Fires
SKSE plugin that allows fires to respond to the weather and ice/fire spells.

## Building
### Requirements:
* CMake
* VCPKG
  * Add the root to an environment variable called `VCPKG_ROOT`.
* Visual Studio (with desktop C++ development)
---
### Instructions:
```
git clone https://github.com/SeaSparrowOG/RainExtinguishesFires
cd SKSE-Plugin-Template
git submodule update --init --recursive
cmake --preset vs2022-windows-vcpkg-release
cmake --build --preset Release
```
---
### Automatic deployment to MO2:
You can automatically deploy to MO2's mods folder by defining an Environment Variable named SKYRIM_MODS_FOLDER and pointing it to your MO2 mods folder. It will create a new mod with the appropriate name. After that, simply refresh MO2 and enable the mod.
