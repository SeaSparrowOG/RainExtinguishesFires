## Rain Extinguishes Fires
SKSE plugin that allows fires to respond to the weather and ice/fire spells.

## Building
1. Install VCPKG and add its installation location in an environment variable called "VCPKG_ROOT".
2. Install CMake.
3. Clone the latest release of PO3's Commonlib fork, and add its location to an environment variable called "CommonLibSSE". 
 - Optionally, if building for pre 1130, clone PO3's Commonlib fork at commit at SHA db60c89b5c8bdd39a786dfdbe605efac24326793 and add its location to an environment variable called "CommonLibSSEOld".
  - Optionally, define your MO2 Mods folder in an environment variable called "SKYRIM_MODS_FOLDER". If defined, building outputs the DLL in a new mod.
4. Clone this repository, and open the containing folder with Visual Studio.
5. Build All for the version you want.
