## Notice about the License Update
As of v5.1.0, Rain Extinguishes Fires is GPLv3 with a modding exception. Previous versions were Apache 2.0, and will remain available should you need them for any reason. Nexus permissions remain unchanged.

## Rain Extinguishes Fires
SKSE plugin that allows fires to respond to the weather and ice/fire spells. Companion for the Skyrim Mod on Nexus with the same name: [Rain Extinguishes Fires](<https://www.nexusmods.com/skyrimspecialedition/mods/80419>).

## Defining Additional Fires
Rain Extinguishes Fires makes use of JSON configuration files in order to determine what **Lit** fire should be replaced with which **Unlit** fire. In order to make your own, do this:
1. Create a json file in `Data/SKSE/Plugins/RainExtinguishesFires`.
1. Create your "unlit" variant (or use one from my mod). Note that the base form should be an **ACTIVATOR**. This is important, because the DLL will actively reject non-activator forms.
1. Attach the `REF_ObjectRefOffController` script to it. The compiled script is inside the `RainExtinguishesFires.bsa`, and the mod ships with the source script for ease of use. You can also download it from this repository.

> [!IMPORTANT]
> Under no circumstances should you modify the properties of `REF_ObjectRefOffController`. Modifying the behavior is fine (but discouraged).

For reference, this is the expected structure of the configuration file:
```json
[
  {
    "Lit": "0x800|Modname.esm",
    "Unlit": "0x801|Modname.esp",
    "Overrides": {
      "resize": 0.9
    }
  }
]
```
You can declare multiple lit/unlit pairs in a single configuration file by separating them with a comma (standard JSON rules apply). The keys are not case sensitive, so `Lit` is treated the same as `lit`.
```json
[
  {
    "Lit": "0x800|Modname.esm",
    "Unlit": "0x801|Modname.esp",
    "Overrides": {
      "resize": 0.9
    }
  },
  {
    "Lit": "0x800|Modname.esm",
    "Unlit": "0x801|Modname.esp",
    "Overrides": {
      "resize": 0.9
    }
  }
]
```

| Field Name | Explanation | Notes |
| :--- | :--- | :--- |
| Lit | The base form of the lit fire. The value should be written as such: `<Local FormID>`\|`<Modname>.<extension>`| You can optionally use EditorIDs if you add PO3's Tweaks as a requirement. |
| Unlit | The base form of the unlit fire. Make sure that it has the `REF_ObjectRefOffController` script attached! | |
| Overrides | You can use this field to specify some overrides for this fire. | This field is optional and can be omitted. |
| Resize | A float value. The resulting fire's scale will be multiplied by this value. | This field is optional and can be omitted. |

## Building
### Requirements:
* CMake
* VCPKG
  * Add the root to an environment variable called `VCPKG_ROOT`.
* Visual Studio (with desktop C++ development)

### Instructions:

```
git clone https://github.com/SeaSparrowOG/RainExtinguishesFires
cd SKSE-Plugin-Template
git submodule update --init --recursive
cmake --preset github-steam
cmake --build --preset Release
```

> [!TIP]
> Automatic deployment to MO2

You can automatically deploy to MO2's mods folder by defining an Environment Variable named SKYRIM_MODS_FOLDER and pointing it to your MO2 mods folder. It will create a new mod with the appropriate name. After that, simply refresh MO2 and enable the mod.

