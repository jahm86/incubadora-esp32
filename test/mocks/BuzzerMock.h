#pragma once

#ifdef __cplusplus
#include "output/Buzzer.h"

struct MockBuzzerTracker {
    BufferTone lastBuffer = {nullptr, 0, false};
    int playCount = 0;
    int stopCount = 0;

    void reset() {
        lastBuffer = {nullptr, 0, false};
        playCount = 0;
        stopCount = 0;
    }
};

inline MockBuzzerTracker& getMockBuzzer() {
    static MockBuzzerTracker tracker;
    return tracker;
}

inline void Buzzer::play(BufferTone buffer) {
    getMockBuzzer().lastBuffer = buffer;
    getMockBuzzer().playCount++;
}

inline void Buzzer::stop() {
    getMockBuzzer().stopCount++;
}
#endif
