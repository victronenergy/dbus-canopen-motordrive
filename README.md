# dbus-sevcon

Venus OS driver for Sevcon Gen4 controller: enables communication and motordrive data reporting via D-Bus.

![dbus-sevcon](doc/dbus-sevcon-header.png)

## How to connect the Sevcon controller to a Victron GX product

Victron GX products: https://www.victronenergy.com/live/venus-os:start.  

Connect the CAN output from the Sevcon controller to the Victron product's VE.Can port #1.

A modified RJ45 cable can be sufficient.

| Role| RJ45 pin # | Controller pin |
|-----|------------|----------------|
| CAN Ground | 3 (Green/White) | B- |
| CAN High | 7 (Brown/White) | 13 or 16 |
| CAN Low | 8 (Brown) | 24 or 27 |

### CAN Ground

Ensure that there is a common ground connection for all nodes on the CAN bus.  
If there is a node on the bus which is galvanically isolated from the Gen4 controller then the CAN ground on this node must be connected to the Gen4 controller B-.

### CAN Termination

If your system has more than one CAN node, connect the nodes in a 'daisy chain' arrangement
and terminate the connections of the two end nodes with a 120 ohms resistor.  
If the end node is a Gen4, link pins 2 and 24 on the customer connector, a 120 ohms resistor is built into the controller.  
If you have a single node system the termination resistor should be connected so that the bus operates correctly when configuration tools are used.

On the VE.Can side, it can be terminated with a dedicated [VE.Can Terminator](https://www.victronenergy.com/accessories/ve-can-rj45-terminator).

Sevcon gen4 manual: https://www.thunderstruck-ev.com/images/Gen4%20Product%20Manual%20V3%204.pdf

### Sevcon Gen4 Model support

Driver has been tested on Sevcon Gen4 AC size 4 controller.  
It should work identically with size 2 and size 6 controllers.  
Not tested on Gen4 DC controllers.

## How to install the driver

1. Download the latest `venus-data.zip` from the [releases page](https://github.com/citolen/dbus-sevcon/releases).
2. Put `venus-data.zip` on an SD card or USB flash drive.
3. Put the SD card or USB flash drive into the Victron GX product.
4. Reboot.
5. Once rebooted, remove the SD card or USB flash drive.
6. Reboot again.

Once those steps are completed, if the controller is turned on and correctly connected, it will start reporting a motordrive.

## Where is the driver installed

The driver is stored in `/data/dbus-sevcon`.  
A symlink is created to `/opt/victronenergy/service/` to ensure it's loaded as a service on boot.

### How to remove the driver

SSH into venus. (See [guide on how to](https://www.victronenergy.com/live/ccgx:root_access))

Run:  
`rm /opt/victronenergy/service/dbus-sevcon`  
`rm -rf /data/dbus-sevcon`

## How does the driver work

Upon being installed, the driver will start looking for the Sevcon controller every 10 seconds.  
If one is detected, a new device (motordrive) will show up in Venus.  
The motor data will be refreshed once per second.

The driver doesn't make use of TPDO/PDOs, instead every second it's requesting the data through SDOs.

Turning off the engine/controller will remove the device.

> [!WARNING]
> The driver currently expects the controller to use CAN node ID 1. This will be configurable in the future.

## Which SDOs are used by the driver

| index | subindex | Description |
|------------|----------------|---|
| 0x1008 | 0 | Controller Name |
| 0x1018 | 4 | Controller Serial Number |
| 0x5100 | 1 | Battery Voltage / 0.0625 |
| 0x5100 | 2 | Battery Current / 0.0625 |
| 0x606c | 0 | Engine RPM |
| 0x4600 | 3 | Engine Temperature |
