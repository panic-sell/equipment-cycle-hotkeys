# **Equipment Cycle Hotkeys**

Cycle through multiple gearsets using a single hotkey.

This is a minimalist hotkey mod heavily inspired by [Serio's Cycle Hotkeys](https://www.nexusmods.com/skyrimspecialedition/mods/27184).


## **Features**

- SKSE-only, no Papyrus scripts. Install/uninstall at any time.
- Bind any number of equipsets to each hotkey. Or just bind one, and have this mod function as a basic, non-cycling hotkey mod.
- Bind up to four keys (including modifier keys) per button combo.
- Bind any number of button combos to each hotkey. This enables setting up one combo for the keyboard and another for the gamepad, and using both to trigger the same hotkey.
- Hotkeys remember their last selected equipset. Activating hotkeys A, then B, then A again will re-equip whatever A had selected prior to activating B.
- Hold a hotkey to reset to the first equipset (a la Elden Ring).
- Equipsets can be configured to ignore or unequip specific slots.
- Hotkey profiles can be exported and transferred across saves.

## **Configuration**

See **`Data/SKSE/Plugins/EquipmentCycleHotkeys.json`** for configurable options, including UI toggle keys and UI font size.


## **Getting Started**

To create a new hotkey:
1. Equip whatever gear you want to hotkey.
1. Open the UI (default **`Shift+\`**).
1. Click "New Hotkey". Optionally, name your hotkey.
1. Under "Keysets", click "New", then configure button bindings for the keyset. Optionally, add additional keysets.
1. Under "Equipsets", click "Add Currently Equipped".
1. Close the UI.
1. To add more equipsets to the same hotkey, repeat steps 1, 2, 5, and 6.

Video demo (make sure to enable captions):
[youtube]PX1rzGusuKs[/youtube]


## **Limitations**

- You can only hotkey items/spells equipped to the hands, ammo, and voice slots. In particular, you can't hotkey armor.
- Hotkeys don't track items across upgrades. You'll have to rehotkey items you temper or enchant.
- When adding a new hotkey in the UI menu, make sure to configure at least one keyset or one equipset before closing the menu. Hotkeys with no keysets and no equipsets are discarded.
- When hotkeying bound equipment, make sure you have the spells equipped instead of their summoned forms.


## **Source**

https://github.com/panic-sell/equipment-cycle-hotkeys
