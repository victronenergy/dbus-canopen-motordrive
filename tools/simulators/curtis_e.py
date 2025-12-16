#!/usr/bin/env python3

import sys
import canopen
import time
import threading
import random
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

swap = 0  # No swap


CURTIS_INSTRUMENTS_VENDOR_ID = 0x4349
CURTIS_E_1236_MODEL_NUMBER = 0x00BCAE6D


def voltage_to_curtis_voltage(voltage):
    return int(voltage / 0.015625)  # in 0.015625V


def current_to_curtis_current(current):
    return int(current * 10)  # in 0.1A


def temperature_to_curtis_temperature(temperature):
    return int(temperature * 10)  # in 0.1°C


def create_curtis_e_node(id):
    node = canopen.LocalNode(id, "./curtis_e.eds")

    node.sdo["Identity Object"]["Serial Number"].raw = serial_number
    node.sdo["Identity Object"]["Vendor ID"].raw = CURTIS_INSTRUMENTS_VENDOR_ID
    node.sdo["Battery Voltage"].raw = voltage_to_curtis_voltage(battery_voltage)
    node.sdo["Battery Current"].raw = current_to_curtis_current(battery_current)
    node.sdo["Motor Speed"].raw = motor_rpm
    node.sdo["Motor Temperature"].raw = temperature_to_curtis_temperature(
        motor_temperature
    )
    node.sdo["Controller Temperature"].raw = temperature_to_curtis_temperature(
        controller_temperature
    )
    node.sdo["Swap"].raw = swap
    node.sdo["Model Number"].raw = CURTIS_E_1236_MODEL_NUMBER

    return node


def simulate_curtis_e_data(node, update_interval=0.125):
    while True:
        node.sdo["Battery Voltage"].raw = voltage_to_curtis_voltage(
            random.uniform(*battery_voltage_range)
        )
        node.sdo["Battery Current"].raw = current_to_curtis_current(
            random.uniform(*battery_current_range)
        )
        node.sdo["Motor Speed"].raw = random.randint(*motor_rpm_range)
        node.sdo["Motor Temperature"].raw = temperature_to_curtis_temperature(
            random.randint(*motor_temperature_range)
        )
        node.sdo["Controller Temperature"].raw = temperature_to_curtis_temperature(
            random.randint(*controller_temperature_range)
        )
        time.sleep(update_interval)


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

    curtis_e_node = create_curtis_e_node(args.node)
    network.add_node(curtis_e_node)
    simulation_thread = threading.Thread(
        target=simulate_curtis_e_data, args=(curtis_e_node,), daemon=True
    )
    simulation_thread.start()

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
