#ifndef __CURTIS_F_H__
#define __CURTIS_F_H__

#include <driver.h>

typedef struct _CurtisFContext {
    int swapMotorDirection;
} CurtisFContext;

extern Driver curtisFDriver;

#endif