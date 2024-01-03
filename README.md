# Equipment Cycle Hotkeys

Simple Skyrim SE mod to allow cycling multiple equipsets using a single hotkey. Heavily inspired by [Serio's Cycle Hotkeys](https://www.nexusmods.com/skyrimspecialedition/mods/27184).

## Features
- SKSE-only, no Papyrus scripts. Install/uninstall at any time.
- Hold a hotkey to reset to the first equipset (a la Elden Ring).
- Hotkeys remember their last selected equipset. Activating hotkeys A, then B, then A again will equip whatever A had selected prior to activating B.
- A hotkey can be bound to multiple key combos. For example, the same hotkey can be activated by KB/M or gamepad.
- An equipset can be configured to ignore slots or unequip items.
- Import/export hotkey configs across saves.

### Limitations
- Does not support armor; only supports items equipped to the hands, ammo, and voice slots.
- Hotkeys don't track items across upgrades. You'll have to rehotkey items you temper or enchant.

## Installation

Install like any other SKSE mod.

### Requirements
- [SKSE](https://skse.silverlock.org)
- [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Quick Start

### Creating a new hotkey and adding one equipset

### Add an equipset to an existing hotkey

## Warnings
- When adding a new hotkey in the UI menu, make sure to add at least one keyset or one equipset before closing the menu. Hotkeys with no keysets and no equipsets are not saved.
- When hotkeying bound equipment, make sure you have the spells equipped instead of their summoned forms.
