#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ap_def.h"


// 표시 장치(현재는 LED) 담당 모듈.
// 명령은 큐로 전달되고 스레드는 처리할 일이 없으면 블록한다.
//
bool displayLedOn(uint8_t ch);
bool displayLedOff(uint8_t ch);
bool displayLedToggle(uint8_t ch);
bool displayLedBlink(uint8_t ch, uint32_t period_ms);


#ifdef __cplusplus
}
#endif

#endif
