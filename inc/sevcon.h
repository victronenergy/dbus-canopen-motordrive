#ifndef __SEVCON_H__
#define __SEVCON_H__

#include <driver.h>
#include <velib/types/ve_item_def.h>

typedef struct {
    VeItem *directionInverted;
} SevconDriverContext;

extern Driver sevconDriver;

#endif