#include "wled.h"

// Garden Smooth Random v4
// WLED 16.0.1, effect appended at the current MODE_COUNT by scripts/patch_wled.py.
//
// Key design points:
// - 64 hue anchors around the color wheel.
// - Only moves to an adjacent anchor (+1 / -1), with random reversals.
// - The R/G/B primaries stay saturated, while the regions between them bend
//   inward toward pale/low-saturation colors, because the garden hardware
//   reproduces that inner part of the color wheel much more smoothly.
// - NO software RGB inversion here. The standard WLED color path is preserved.
//
// UI controls:
//   Speed      (sx): full-circle speed. At 255, ~6 s per theoretical full turn.
//   Brightness (ix): HSV value for the effect (default 255).
//   Randomness (c1): chance to reverse direction at each 1/64 hue anchor.
//   Softness   (c2): how far toward white the midpoint of each transition goes.

static uint32_t gardenHsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
  if (s == 0) return RGBW32(v, v, v, 0);

  // Integer HSV -> RGB, six equal hue sectors.
  const uint8_t region = h / 43;                       // 0..5
  const uint8_t rem    = uint8_t((h - region * 43) * 6); // 0..252

  const uint8_t p = uint8_t((uint16_t(v) * (255U - s)) / 255U);
  const uint8_t q = uint8_t((uint16_t(v) * (255U - ((uint16_t(s) * rem) / 255U))) / 255U);
  const uint8_t t = uint8_t((uint16_t(v) * (255U - ((uint16_t(s) * (255U - rem)) / 255U))) / 255U);

  switch (region) {
    case 0: return RGBW32(v, t, p, 0);
    case 1: return RGBW32(q, v, p, 0);
    case 2: return RGBW32(p, v, t, 0);
    case 3: return RGBW32(p, q, v, 0);
    case 4: return RGBW32(t, p, v, 0);
    default:return RGBW32(v, p, q, 0);
  }
}

static uint16_t gardenSmoothstep16(uint16_t x) {
  // 3x^2 - 2x^3, normalized to 0..65535.
  const uint64_t X = x;
  const uint64_t M = 65535ULL;
  return uint16_t((X * X * (3ULL * M - 2ULL * X) + (M * M / 2ULL)) / (M * M));
}

static void mode_garden_smooth_random() {
  // Speed is expressed as a theoretical full 64-anchor turn:
  // sx=255 -> ~6 s, sx=0 -> ~180 s.
  // Quadratic mapping gives finer control at the fast end.
  const uint32_t d = 255U - SEGMENT.speed;
  const uint32_t fullCircleMs = 6000UL + uint32_t((uint64_t(d) * d * 174000ULL) / 65025ULL);
  const uint32_t rawStepMs = fullCircleMs / 64U;
  const uint32_t stepMs = (rawStepMs < 40U) ? 40U : rawStepMs;

  const uint8_t value = SEGMENT.intensity;

  // c1=0 => about 2% reversal chance per anchor
  // c1=255 => 50% reversal chance per anchor
  const uint8_t reverseThreshold = uint8_t(map(SEGMENT.custom1, 0, 255, 5, 128));

  // c2=0: almost no desaturation (midpoint sat ~240)
  // c2=255: midpoint goes close to white (sat ~16)
  const uint8_t minSaturation = uint8_t(map(SEGMENT.custom2, 0, 255, 240, 16));

  if (SEGENV.call == 0) {
    // 64 anchors: 0,4,8,...252
    SEGENV.aux0 = uint16_t((hw_random8() >> 2) << 2);
    // aux1: 0 = backward, 1 = forward
    SEGENV.aux1 = (hw_random8() & 0x01U) ? 1U : 0U;
    SEGENV.step = strip.now;
  }

  uint32_t elapsed = strip.now - SEGENV.step;

  // Catch up after delayed frames without making distant hue jumps.
  while (elapsed >= stepMs) {
    const int8_t dir = SEGENV.aux1 ? 1 : -1;
    SEGENV.aux0 = uint8_t(int16_t(SEGENV.aux0) + int16_t(dir) * 4);

    if (hw_random8() < reverseThreshold) {
      SEGENV.aux1 = !SEGENV.aux1;
    }

    SEGENV.step += stepMs;
    elapsed = strip.now - SEGENV.step;
  }

  const int8_t dir = SEGENV.aux1 ? 1 : -1;
  uint32_t progress32 = (elapsed * 65535UL) / stepMs;
  if (progress32 > 65535UL) progress32 = 65535UL;
  const uint16_t progress = uint16_t(progress32);
  const uint16_t eased = gardenSmoothstep16(progress);

  // Interpolate only to the immediately adjacent 1/64 hue anchor.
  const int16_t interpHue =
      int16_t(uint8_t(SEGENV.aux0)) +
      int16_t((int32_t(dir) * 4L * int32_t(eased)) / 65535L);
  const uint8_t hue = uint8_t(interpHue); // deliberate wrap at 255 -> 0

  // Hardware workaround: keep the RGB primaries saturated, but move toward
  // the pale/near-white region between primaries. This creates three broad
  // low-saturation valleys per color wheel (around yellow, cyan, magenta)
  // instead of pulsing toward white at every 1/64 anchor.
  //
  // hue*3 wraps every ~85 hue counts, i.e. once per R->G, G->B, B->R sector.
  // phase is 0/255 near a primary and ~128 halfway between primaries.
  const uint8_t phase = uint8_t(uint16_t(hue) * 3U);
  const uint8_t triangle8 = (phase <= 127U)
      ? uint8_t(uint16_t(phase) * 2U)
      : uint8_t(uint16_t(255U - phase) * 2U);
  const uint16_t soft = gardenSmoothstep16(uint16_t(triangle8) * 257U);
  const uint8_t saturation = uint8_t(
      255U - (uint32_t(255U - minSaturation) * soft) / 65535U
  );

  SEGMENT.fill(gardenHsvToRgb(hue, saturation, value));
}

static const char _data_FX_GARDEN_SMOOTH_RANDOM[] PROGMEM =
  "Garden Smooth Random@Speed,Brightness,Randomness,Softness;;;1;sx=220,ix=255,c1=64,c2=210,pal=0";

class UserFxUsermod : public Usermod {
public:
  void setup() override {
    // Final slot appended by scripts/patch_wled.py.
    // Do NOT use 255 here: on this build it can reuse a reserved gap and
    // some clients then fail to keep the effect in their visible menu.
    strip.addEffect(FX_MODE_GARDEN_SMOOTH_RANDOM,
                    &mode_garden_smooth_random,
                    _data_FX_GARDEN_SMOOTH_RANDOM);
  }

  void loop() override {}
  uint16_t getId() override { return USERMOD_ID_USER_FX; }
};

static UserFxUsermod user_fx;
REGISTER_USERMOD(user_fx);
