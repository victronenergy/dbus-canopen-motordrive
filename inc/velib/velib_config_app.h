#define VELIB_EXPECTED_TRACKING_NR		11

/* Use libevent, no threads and no special arguments */
#define CFG_WITH_TASK					1
#define CFG_WITH_VERSION				1
#define CFG_WITH_TASK_LIBEVENT			1
#define CFG_WITH_LIBEVENT				1
#define CFG_NO_PREEMPTION				1
#define CFG_WITH_DEFAULT_ARGUMENTS		1

/* Setup a CAN bus connection using a driver found at runtime */
#define CFG_INIT_CANBUS					1
#define CFG_WITH_CANHW_DRIVER			1
#define CFG_WITH_J1939_SF_MEM			1

#define CFG_TASK_EARLY_INIT				1
#define CFG_CANHW_DRIVER_STATIC			1
#define CFG_WITH_CANHW_SOCKETCAN		1
#define CFG_WITH_CANHW					1

/* LG resu CAN bus is little endian */
#define CFG_WITH_VE_STREAM				1
#define CFG_WITH_VE_STREAM_LE			1
#define CFG_WITH_VE_STREAM_BE			1
#define CFG_WITH_VE_STREAM_ITEMS		1
#define CFG_WITH_VE_STREAM_ITEMS_LE		1
#define CFG_WITH_VE_STREAM_VARIANT_LE	1
#define CFG_WITH_VARIANT_PRINT			1

/* Enable dbus support */
#define CFG_WITH_VE_VALUES				1
#define CFG_WITH_VE_ITEM				1
#define CFG_DBUS_ITEM_NO_TYPE_CHANGE	1
#define CFG_WITH_ITEM_UTILS				1
#define CFG_WITH_VE_DBUS_ITEM			1
#define CFG_WITH_VE_DBUS_ITEM_CONSUMER	1
#define CFG_WITH_FLOAT					1
#define CFG_WITH_STRING					1
#define CFG_WITH_VARIANT				1
#define CFG_VARIANT_HEAP				1
#define CFG_DBUS_NON_BLOCKING			1

#define CFG_WITH_LOG                    1
#define CFG_WITH_VE_WAIT                1
#define CFG_WITH_VE_TIMER               1
#define CFG_WITH_TIMER                  1

#define CFG_WITH_VECAN_PRODUCT_NAMES	1
