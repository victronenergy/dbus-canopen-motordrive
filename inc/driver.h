#ifndef __DRIVER_H__
#define __DRIVER_H__

typedef struct _Driver Driver;

#include <node.h>
#include <velib/base/types.h>

typedef struct _Driver {
    const char *name;
    un16 productId;
    void (*readRoutine)(Node *node);
    void (*fastReadRoutine)(Node *node);
    void *(*createDriverContext)(Node *node);
    void (*destroyDriverContext)(Node *node, void *context);
    void (*onEMCYMessage)(Node *node, VeRawCanMsg *message);
} Driver;

#endif