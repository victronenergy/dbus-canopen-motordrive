T = dbus-canopen-motordrive$(EXT)
VERSION = 0.4

TARGETS += $T
INSTALL_BIN += $T

SUBDIRS += velib
INCLUDES += velib/inc
$T_DEPS += $(call subtree_tgts,$(d)/velib)


SUBDIRS += src
INCLUDES += ./inc
$T_DEPS += $(call subtree_tgts,$(d)/src)

$T_LIBS += -lpthread -ldl $(shell pkg-config --libs dbus-1) -levent -lm
CC_CONFIG_CFLAGS += -DDBUS $(shell pkg-config --cflags dbus-1) -DVERSION=\"$(VERSION)\"

# LDFLAGS += -s