# WLED Garden Smooth Random v4

Target: official WLED **16.0.1 (Niji)**, classic ESP32, `esp32dev` base.

## What changed

1. The effect is now assigned the fixed **FX the final appended effect ID**.
2. WLED `MODE_COUNT` is patched from **220 to 221**, so the custom effect is a real final entry in the effect table instead of being placed into a reserved gap.
3. Registration uses WLED's official `user_fx` usermod mechanism.
4. The old software RGB inversion was removed. Standard WLED RGB values are sent to the WS281x output.
5. The hue still moves only through adjacent 1/64 anchors, but the color path now bends inward toward pale/low-saturation colors between the R/G/B primaries. The primaries remain saturated; the yellow/cyan/magenta regions become progressively paler. This uses the part of the color wheel that your garden hardware reproduces much more smoothly, without pulsing toward white at every tiny anchor step.
6. Speed was rescaled: maximum speed corresponds to roughly a 6-second theoretical full hue turn (~2 s from one RGB primary to the next if direction stays constant).

## Controls

- **Speed** — transition speed.
- **Brightness** — effect HSV value (default 255).
- **Randomness** — probability of reversing direction at each of the 64 hue anchors.
- **Softness** — how strongly the path bends toward white between the R/G/B primaries. Higher = paler intermediate colors and usually smoother transitions on this garden hardware.

Recommended first test: `Speed=220`, `Brightness=255`, `Randomness=64`, `Softness=210`.

Keep **Gamma correction for color OFF** while testing, as already configured on the garden controller.

## Build

Upload the contents of this folder to the root of your GitHub repository, replacing the old build files.

Then:

1. Open **Actions**.
2. Select **Build WLED Garden firmware v4**.
3. Press **Run workflow**.
4. Download artifact **WLED-16.0.1-ESP32-GardenSmoothRandom-v4**.
5. Flash `WLED_16.0.1_ESP32_GardenSmoothRandom_v4.bin` from WLED's update page.

The workflow is pinned to exact tag `v16.0.1`. This matters because WLED 16.1 later uses effect the final appended effect ID for a built-in effect.

## After flashing

Check these URLs:

- `/json/info` -> `fxcount` should be **221**.
- `/json/eff` -> the final entry (ID **220**) should be **Garden Smooth Random**.
- `/json/fxdata` -> entry 220 should contain the four controls above.

Then reload the WLED UI. `Garden Smooth Random` should remain selectable in **Effects**, including after switching to another effect and back.
