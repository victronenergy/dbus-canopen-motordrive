#ifndef __CURTIS_E_H__
#define __CURTIS_E_H__

#include <driver.h>

typedef struct _CurtisEContext {
    int swapMotorDirection;
    float modulationDepth;
} CurtisEContext;

extern Driver curtisEDriver;

#endif