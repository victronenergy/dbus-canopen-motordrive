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

motor_torque = 80  # in Nm
motor_torque_range = [75, 85]

swap = 0  # No swap


def voltage_to_sevcon_voltage(voltage):
    return int(voltage / 0.0625)  # in 0.0625V


def current_to_sevcon_current(current):
    return int(current / 0.0625)  # in 0.0625A


def temperature_to_sevcon_temperature(temperature):
    return int(temperature)


def create_sevcon_node(id):
    node = canopen.LocalNode(id, "./sevcon_gen4.eds")

    node.sdo["Device Name"].raw = "Gen4 Simulator"
    node.sdo["Identity Object"]["Serial Number"].raw = serial_number
    node.sdo["Battery and Controller Data"]["Battery Voltage"].raw = (
        voltage_to_sevcon_voltage(battery_voltage)
    )
    node.sdo["Battery and Controller Data"]["Battery Current"].raw = (
        current_to_sevcon_current(battery_current)
    )
    node.sdo["Motor Speed"].raw = motor_rpm
    node.sdo["Motor Temperature"]["Temperature"].raw = (
        temperature_to_sevcon_temperature(motor_temperature)
    )
    node.sdo["Motor Torque"]["Torque"].raw = motor_torque
    node.sdo["Battery and Controller Data"]["Controller Temperature"].raw = (
        temperature_to_sevcon_temperature(controller_temperature)
    )

    return node


def simulate_sevcon_data(node, update_interval=0.1):
    while True:
        node.sdo["Battery and Controller Data"]["Battery Voltage"].raw = (
            voltage_to_sevcon_voltage(random.uniform(*battery_voltage_range))
        )
        node.sdo["Battery and Controller Data"]["Battery Current"].raw = (
            current_to_sevcon_current(random.uniform(*battery_current_range))
        )
        node.sdo["Motor Speed"].raw = random.randint(*motor_rpm_range)
        node.sdo["Motor Temperature"]["Temperature"].raw = (
            temperature_to_sevcon_temperature(random.randint(*motor_temperature_range))
        )
        node.sdo["Motor Torque"]["Torque"].raw = random.randint(*motor_torque_range)
        node.sdo["Battery and Controller Data"]["Controller Temperature"].raw = (
            temperature_to_sevcon_temperature(
                random.randint(*controller_temperature_range)
            )
        )
        time.sleep(update_interval)


def listen(node):
    while True:
        command = sys.stdin.readline()
        if command == "overcurrent\n":
            print("EMCY: Motor Overcurrent Fault")
            node.emcy.send(0x1000, 0x00, b"\xC2\x52\x02\x00\x00")


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

    sevcon_node = create_sevcon_node(args.node)
    network.add_node(sevcon_node)
    simulation_thread = threading.Thread(
        target=simulate_sevcon_data, args=(sevcon_node,), daemon=True
    )
    simulation_thread.start()
    threading.Thread(target=listen, args=(sevcon_node,), daemon=True).start()

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
