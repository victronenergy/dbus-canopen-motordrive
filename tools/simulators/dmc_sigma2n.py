#!/usr/bin/env python3

import sys
import canopen
import time
import threading
import random
import struct
import argparse

node_id = 0x26
serial_number = 12345678

battery_voltage = 51.2  # in V
battery_voltage_range = [51.0, 53.0]

battery_current = 40.0  # in A
battery_current_range = [35.0, 45.0]

motor_rpm = 2000
motor_rpm_range = [200, 2200]

motor_temperature = 42  # in °C
motor_temperature_range = [40, 45]

controller_temperature = 36  # in °C
controller_temperature_range = [35, 38]

motor_torque = 45.0  # in % of rated torque
motor_torque_range = [40.0, 50.0]

motor_torque_nm = 80.0  # in Nm
motor_torque_nm_range = [70.0, 90.0]

drive_hours = 1234  # in h
key_hours = 1502  # in h

# Object 0x3838, see DMC CAN Open Object Dictionary description V1.11 §2.55
DRIVE_STATUS_NEUTRAL = 2
DRIVE_STATUS_FORWARD = 3
DRIVE_STATUS_REVERSE = 4

# Temperatures are reported as an unsigned byte with a fixed offset.
TEMPERATURE_OFFSET = 51


def voltage_to_dmc_voltage(voltage):
    return int(voltage * 10)  # in 0.1V


def current_to_dmc_current(current):
    return int(current * 10)  # in 0.1A


def temperature_to_dmc_temperature(temperature):
    return int(temperature) + TEMPERATURE_OFFSET


def torque_to_dmc_torque(torque):
    return int(torque * 4096 / 100)  # in 100/4096 %


def torque_to_ds402_torque(torque):
    return int(torque * 10)  # in 0.1 %


def torque_to_dmc_torque_nm(torque):
    return int(torque * 10)  # in 0.1 Nm


def hours_to_seconds(hours):
    return int(hours * 3600)


def rpm_to_drive_status(rpm):
    if rpm > 0:
        return DRIVE_STATUS_FORWARD
    elif rpm < 0:
        return DRIVE_STATUS_REVERSE
    return DRIVE_STATUS_NEUTRAL


def set_speed(node, rpm):
    node.sdo[0x606C].raw = rpm
    node.sdo[0x3832].raw = rpm
    node.sdo[0x3838].raw = rpm_to_drive_status(rpm)


def set_torque(node, torque, torque_nm):
    node.sdo[0x6077].raw = torque_to_ds402_torque(torque)
    node.sdo[0x3834].raw = torque_to_dmc_torque(torque)
    node.sdo[0x411C].raw = torque_to_dmc_torque_nm(torque_nm)


# Sigma2N firmware V03.03.01 announces a 20 byte segmented upload of 0x1008 and
# then aborts every segment request with 0x06020000, so the device name is
# never readable and discovery falls back to the identity object. Reproduced
# here, otherwise the simulator only exercises a path the controller does not
# take. Confirmed on two controllers, at segment request intervals from 5ms to
# 1s, while every expedited read in the same capture succeeded.
def abort_segmented_name_upload(node):
    server = node.sdo
    upload_segment = server.segmented_upload

    def segmented_upload(command):
        if server._index == 0x1008:
            # The controller echoes a zero index and subindex in the abort
            # rather than the object being read.
            server._index = 0
            server._subindex = 0
            raise canopen.SdoAbortedError(0x06020000)
        upload_segment(command)

    server.segmented_upload = segmented_upload


# The object dictionary is indexed by number rather than by name because the
# DMC object dictionary reuses parameter names across indexes.
def create_dmc_node(id):
    node = canopen.LocalNode(id, "./dmc_sigma2n.eds")

    node.sdo[0x1008].raw = "Sigma2N IPM Traction"
    abort_segmented_name_upload(node)
    node.sdo[0x1018][4].raw = serial_number
    node.sdo[0x383F].raw = voltage_to_dmc_voltage(battery_voltage)
    node.sdo[0x383E].raw = current_to_dmc_current(battery_current)
    set_speed(node, motor_rpm)
    set_torque(node, motor_torque, motor_torque_nm)
    node.sdo[0x3836].raw = temperature_to_dmc_temperature(motor_temperature)
    node.sdo[0x3837].raw = temperature_to_dmc_temperature(controller_temperature)
    node.sdo[0x4100].raw = hours_to_seconds(drive_hours)
    node.sdo[0x4101].raw = hours_to_seconds(key_hours)

    return node


def simulate_dmc_data(node, update_interval=0.1):
    elapsed = 0.0

    while True:
        node.sdo[0x383F].raw = voltage_to_dmc_voltage(
            random.uniform(*battery_voltage_range)
        )
        node.sdo[0x383E].raw = current_to_dmc_current(
            random.uniform(*battery_current_range)
        )
        set_speed(node, random.randint(*motor_rpm_range))
        set_torque(
            node,
            random.uniform(*motor_torque_range),
            random.uniform(*motor_torque_nm_range),
        )
        node.sdo[0x3836].raw = temperature_to_dmc_temperature(
            random.randint(*motor_temperature_range)
        )
        node.sdo[0x3837].raw = temperature_to_dmc_temperature(
            random.randint(*controller_temperature_range)
        )

        elapsed += update_interval
        node.sdo[0x4100].raw = hours_to_seconds(drive_hours) + int(elapsed)
        node.sdo[0x4101].raw = hours_to_seconds(key_hours) + int(elapsed)

        time.sleep(update_interval)


# EMCY payload, see DMC Advanced CAN Open manual V1.10 §EMCY message:
# bytes 0-1 emergency error code, byte 2 error register, byte 3 DMC fault code,
# bytes 4-7 DMC fault subcode.
def send_fault(node, error_code, fault_code, fault_subcode=0):
    error_register = 0x01 | (0x08 if error_code == 0x4000 else 0x00)
    node.sdo[0x1001].raw = error_register
    node.sdo[0x3840].raw = fault_code
    node.sdo[0x3841].raw = fault_subcode
    node.emcy.send(
        error_code, error_register, struct.pack("<BI", fault_code, fault_subcode)[:5]
    )


def listen(node):
    while True:
        command = sys.stdin.readline()
        if command == "motortemp\n":
            print("EMCY: F5 Motor temperature high")
            send_fault(node, 0x4000, 5)
        elif command == "controllertemp\n":
            print("EMCY: F6 Controller temperature high")
            send_fault(node, 0x4000, 6)
        elif command == "overvoltage\n":
            print("EMCY: F4 S1 Battery voltage above absolute maximum")
            send_fault(node, 0x3000, 4, 1)


def main(argv):
    parser = argparse.ArgumentParser(prog="ProgramName")
    parser.add_argument(
        "--bus",
        "-b",
        choices=["socketcan", "kvaser", "pcan", "ixxat", "nican"],
        default="socketcan",
        help="Bus type to connect to",
    )
    parser.add_argument(
        "--channel", "-c", default="vcan0", help="Channel/interface to connect to"
    )
    parser.add_argument(
        "--node", "-n", type=lambda x: int(x, 0), default=node_id, help="Node ID"
    )

    args = parser.parse_args(argv)

    network = canopen.Network()
    network.connect(channel=args.channel, interface=args.bus, baudrate=250000)

    dmc_node = create_dmc_node(args.node)
    network.add_node(dmc_node)
    simulation_thread = threading.Thread(
        target=simulate_dmc_data, args=(dmc_node,), daemon=True
    )
    simulation_thread.start()
    threading.Thread(target=listen, args=(dmc_node,), daemon=True).start()

    print("Simulator. Press Ctrl+C to exit.")
    try:
        while True:
            network.check()
            time.sleep(0.001)
    except KeyboardInterrupt:
        print("Shutting down")
        network.disconnect()


if __name__ == "__main__":
    main(sys.argv[1:])
