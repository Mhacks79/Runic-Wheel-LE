#pragma once
// Pure ownership rules. No TESV calls in this file: native boundaries stay
// inside the input-dispatcher thread and are separately verified by readback.
namespace rw09 {
inline bool wants_slowdown(bool wheelVisible, unsigned menuFlags, bool enabled) {
    return enabled && wheelVisible && (menuFlags & 3u) == 0;
}
inline bool still_owns_multiplier(float current, float requested) {
    return current > requested - 0.005f && current < requested + 0.005f;
}
inline float target_speed(float previous, int percent) {
    const float requested=previous * float(percent) / 100.f;
    return requested >= 0.01f ? requested : 0.01f;
}
}
