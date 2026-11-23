#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int64_t& mock_esp_timer_ref() {
    static int64_t s_timeUs = 0;
    return s_timeUs;
}

inline int64_t esp_timer_get_time(void) {
    return mock_esp_timer_ref();
}

inline void mock_set_esp_timer_time(int64_t us) {
    mock_esp_timer_ref() = us;
}

inline void mock_advance_esp_timer_time(int64_t us) {
    mock_esp_timer_ref() += us;
}

#ifdef __cplusplus
}
#endif
