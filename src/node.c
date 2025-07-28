#include <curtis.h>
#include <node.h>
#include <sevcon.h>

Driver *getDriverForNodeName(un8 *name, un8 length) {
    if (length >= 4 && name[0] == 'G' && name[1] == 'e' && name[2] == 'n' &&
        name[3] == '4') {
        return &sevconDriver;
    } else if (length >= 4 && name[0] == 'A' && name[1] == 'C' &&
               name[2] == ' ' && name[3] == 'F') {
        return &curtisDriver;
    }

    return NULL;
}