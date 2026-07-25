#ifndef MODULE_H_
#define MODULE_H_


#include "ap_def.h"




typedef enum
{
  MODULE_PRI_HIGH = 1,
  MODULE_PRI_NORMAL,
  MODULE_PRI_LOW,
} ModulePriority_t;


typedef struct module_t_
{
  const char        name[32];
  ModulePriority_t  priority;
  bool            (*init)(void);
} module_t;


// 모듈은 .module 섹션에 등록되고 moduleInit() 이 우선순위 순으로 init() 을 호출한다.
// 각 모듈은 자기 스레드를 init() 안에서 생성한다.
//
#define MODULE_DEF(x_name) static __attribute__((section(".module"))) volatile module_t module_##x_name =




bool moduleInit(void);


#endif
