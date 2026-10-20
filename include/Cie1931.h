#pragma once

#include <cmath>
#include <cstdint>

// CIE 1931 perceptual brightness conversion.
//
// Input is the perceived lightness L* in percent (0..100). The output is a
// 12-bit LEDC duty cycle (0..4095) for 4 kHz PWM.
//
//   Y = L* / 902.3                    for L* <= 8
//   Y = ((L* + 16) / 116)^3           for L* > 8
//   PWM = round(Y * 4095)
//
// Fading must interpolate L* (not the duty cycle) and call percentToDuty()
// for every step; otherwise low brightness levels jump visibly.
namespace cie1931 {

constexpr float kMinPercent = 0.0f;
constexpr float kMaxPercent = 100.0f;
constexpr uint16_t kMaxDuty = 4095;

// Clamps to [0..100]. NaN and negative values map to 0.
inline float clampPercent(float percent) {
  if (!(percent > kMinPercent)) return kMinPercent;
  if (percent > kMaxPercent) return kMaxPercent;
  return percent;
}

// Perceived lightness L* [%] -> relative luminance Y [0..1].
inline float percentToLuminance(float percent) {
  const float lightness = clampPercent(percent);
  if (lightness <= 8.0f) {
    return lightness / 902.3f;
  }
  const float t = (lightness + 16.0f) / 116.0f;
  return t * t * t;
}

// Perceived lightness L* [%] -> 12-bit PWM duty.
inline uint16_t percentToDuty(float percent) {
  const float luminance = percentToLuminance(percent);
  long duty = std::lroundf(luminance * static_cast<float>(kMaxDuty));
  if (duty < 0) return 0;
  if (duty > kMaxDuty) return kMaxDuty;
  return static_cast<uint16_t>(duty);
}

// Inverse mapping, useful for diagnostics/tests. Quantisation means the
// round-trip is approximate.
inline float dutyToPercent(uint16_t duty) {
  if (duty == 0) return 0.0f;
  const float luminance = static_cast<float>(duty) / static_cast<float>(kMaxDuty);
  const float lightness =
      luminance <= 0.008856f ? luminance * 902.3f
                             : 116.0f * std::cbrtf(luminance) - 16.0f;
  return clampPercent(lightness);
}

}  // namespace cie1931
