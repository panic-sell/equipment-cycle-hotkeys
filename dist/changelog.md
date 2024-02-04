# Changelog

## 1.4.0
- Make UI font configurable, so that non-English equipment names can be displayed properly.
- Replace config option "menu_font_scale" with "menu_font_size". Font scale 1.0 translates to font size 13.

## 1.3.0
- Hotkey activation now shows a HUD message listing newly equipped gear. This feature can be disabled in EquipmentCycleHotkeys.json if you find it too distracting.
- Distribute as a single DLL for all supported game versions.

## 1.2.0
- Prevent hotkey activation in QuickLoot menu.
- Fix shields not being recognized during equipset creation.
- For items with custom names, hotkeys no longer track tempering suffixes like Fine, Superior, Exquisite, etc. Prior to this change, if you named a weapon "Awesome Weapon" and then upgraded it such that its name became "Awesome Weapon (Fine)", hotkeying "Awesome Weapon (Fine)" makes the hotkey not recognize "Awesome Weapon" at other upgrade levels.

## 1.1.0
- Hotkeys now pick the most upgraded version of whatever is in your inventory (assuming custom gear name and enchantment already match). Your gear no longer needs to be at the exact same upgrade level as when you hotkeyed it.

## 1.0.0
- Supported game versions: 1.5.97, 1.6.640+
