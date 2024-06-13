## Rain Extinguishes Fires
SKSE plugin that allows fires to respond to the weather and ice/fire spells.

## Building
1. Install VCPKG and add its installation location in an environment variable called "VCPKG_ROOT".
2. Install CMake.
3. Install VS with C++ Desktop Development
4. Clone this repository.
5. Open a Git Bash/Powershell Window in the project folder, and run this command: `git submodule update --init --recursive`
6. Open the project in VS, wait for CMake to finish, and build the project.

## Automagically deploy
If you want to deploy directly in a MO2 instance, you can add the mods folder of that instance in an
environment variable called "SKYRIM_MODS_FOLDER". Upon building, it will automatically create a mod
folder in that location with the built DLL inside.
