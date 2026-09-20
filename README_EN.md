# Sofle

## Sofle Keymap

![Sofle](keymap-drawer/eyelash_sofle.svg)

## Keymap Configuration Guide

The active keyboard definition is
[`config/eyelash_sofle.keymap`](config/eyelash_sofle.keymap). Its `bindings`
arrays follow the physical matrix rather than the labels in the keymap image:
each of the first four rows contains the six left-hand keys, the joystick
direction, and the six right-hand keys. The final row contains thumb keys and
the joystick click. `&kp` sends a normal HID key, `&mo` enables a layer while
held, `&trans` inherits the key from a lower active layer, and `&mkp` sends a
mouse button.

| Section | What it configures |
|---|---|
| Pointing settings (`&mmv`, `&msc`) | Joystick pointer speed, acceleration, and scroll scaling. |
| `scroll_encoder`, `rsr_vol`, and `rsr_trans` | Encoder rotation behaviors: scrolling, volume changes, and transparent pass-through. |
| `softoff` combo | Holding Q, S, and Z together for two seconds enters deep sleep. |
| Macros | Modifier-safe shortcuts for Layer 2 joystick actions: Alt+Tab, Alt+Esc, Ctrl+Tab, Ctrl+Shift+Tab, and Win+Tab. |
| `layer0` | Daily typing layout, arrows on the joystick, and volume on encoder rotation. |
| `layer_1` | Function keys, navigation, mouse buttons/movement, RGB controls, and encoder scrolling with middle-click. |
| `layer_2` | Bluetooth profile/clear actions, USB/BLE output selection, system controls, joystick shortcuts, and encoder zoom/reset. |

The board wiring and LED strip are defined in
[`boards/shields/eyelash_sofle/eyelash_sofle.dtsi`](boards/shields/eyelash_sofle/eyelash_sofle.dtsi).
It selects the 7-pixel WS2812 strip, battery source, matrix transform,
external RGB power switch, display SPI bus, and optional encoder.
[`config/eyelash_sofle.conf`](config/eyelash_sofle.conf) enables the ZMK
features used by the keymap, including RGB underglow, pointing, encoder,
soft-off, and power-management settings.

### LED Status Effects

The seven physical RGB groups remain controlled by ZMK's normal underglow
renderer. `src/rgb_overlay.c` overlays status colors only on the configured
groups, so RGB brightness, color, and effect controls remain available.

| Event or Layer 2 action | LED effect |
|---|---|
| Host Caps Lock LED report | The configured Caps Lock group lights white while Caps Lock is active. |
| Layer 2 + ESC | The configured ESC group flashes the active Bluetooth profile, then lights blue when connected. |
| Layer 2 + TAB | The configured TAB group shows green above 60%, yellow from 30–60%, and red below 30%, for three seconds. |

The PCB exposes seven independently addressable RGB groups, not 29 per-key
LEDs. The initial group indices are `ESC = 0`, `TAB = 1`, and `Caps Lock = 2`;
adjust these values in the `status_leds` node if the physical group order differs.
