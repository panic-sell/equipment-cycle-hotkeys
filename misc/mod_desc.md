# Equipment Cycle Hotkeys

Minimalist SKSE plugin to allow cycling multiple equipsets using a single hotkey.

Heavily inspired by [Serio's Cycle Hotkeys](https://www.nexusmods.com/skyrimspecialedition/mods/27184).

Mod was tested on SE (1.5.97) and AE (1.6.1130), but should theoretically support any version for which [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444) is available.


## Features

- SKSE-only, no Papyrus scripts. Install/uninstall at any time.
- Hold a hotkey to reset to the first equipset (a la Elden Ring).
- A hotkey can be bound to multiple key combos. For example, the same hotkey can be activated by KB/M or gamepad.
- Hotkeys remember their last selected equipset. Activating hotkeys A, then B, then A again will equip whatever A had selected prior to activating B.
- An equipset can be configured to ignore or unequip specific slots.
- Import/export hotkey profiles across saves.


## Getting Started

### Create a new hotkey

1. Equip whatever gear you want to hotkey.
1. Open the config UI (default `Shift+\`, can be customized through `Data/SKSE/Plugins/EquipmentCycleHotkeys.json`).
1. Click "New Hotkey". Optionally, name your hotkey.
1. Under "Keysets", click "New", then configure button bindings.
1. Under "Equipsets", click "Add Currently Equipped".
1. Close the UI.

Video demonstration: TODO

### Create a second hotkey

TODO video

### Manage profiles

TODO video


## Limitations

- Does not support armor; only supports items equipped to the hands, ammo, and voice slots.
- Hotkeys don't track items across upgrades. You'll have to rehotkey items you temper or enchant.
- When adding a new hotkey in the UI menu, make sure to configure at least one keyset or one equipset before closing the menu. Hotkeys with no keysets and no equipsets are discarded.
- When hotkeying bound equipment, make sure you have the spells equipped instead of their summoned forms.


## Source

https://github.com/panic-sell/equipment-cycle-hotkeys
