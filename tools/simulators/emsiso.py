#!/usr/bin/env python3

import sys
import canopen
import time
import threading
import random
import argparse

node_id = 0x26
serial_number = "ABCD5678"

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

coolant_temperature = 30  # in °C
coolant_temperature_range = [28, 32]

motor_torque = 80  # in Nm
motor_torque_range = [75, 85]


def create_emsiso_node(id):
    node = canopen.LocalNode(id, "./emsiso.eds")

    node.sdo["Manufacturer Device Name"].raw = "EMDI-2-L30B-144-SD"
    node.sdo["emDrive_Info"]["Serial_Number"].raw = serial_number
    node.sdo["Ctrl_Measurements"]["Udc"].raw = battery_voltage
    node.sdo["Ctrl_Measurements"]["Idc"].raw = battery_current
    node.sdo["Ctrl_Gen_Stat"]["RPMact"].raw = motor_rpm
    node.sdo["Ctrl_Gen_Stat"]["MotorWireTemp"].raw = motor_temperature
    node.sdo["Ctrl_Gen_Stat"]["Tact"].raw = motor_torque
    node.sdo["Ctrl_Bridge_StatusMaxTemp"].raw = controller_temperature
    node.sdo["Ctrl_Bridge_StatusCalculatedCoolantTemp"].raw = coolant_temperature
    return node


def simulate_emsiso_data(node, update_interval=0.125):
    while True:
        node.sdo["Ctrl_Measurements"]["Udc"].raw = random.uniform(
            *battery_voltage_range
        )
        node.sdo["Ctrl_Measurements"]["Idc"].raw = random.uniform(
            *battery_current_range
        )
        node.sdo["Ctrl_Gen_Stat"]["RPMact"].raw = random.randint(*motor_rpm_range)
        node.sdo["Ctrl_Gen_Stat"]["MotorWireTemp"].raw = random.randint(
            *motor_temperature_range
        )
        node.sdo["Ctrl_Gen_Stat"]["Tact"].raw = random.randint(*motor_torque_range)
        node.sdo["Ctrl_Bridge_StatusMaxTemp"].raw = random.randint(
            *controller_temperature_range
        )
        node.sdo["Ctrl_Bridge_StatusCalculatedCoolantTemp"].raw = random.randint(
            *coolant_temperature_range
        )
        time.sleep(update_interval)


def listen(node):
    while True:
        command = sys.stdin.readline()
        # if command == "overcurrent\n":
        # print("EMCY: Controller Overcurrent Phase W")
        # node.emcy.send(0xFF12, 0x01, b'\x10\x25\x02\x00\x00')


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

    curtis_f_node = create_emsiso_node(args.node)
    network.add_node(curtis_f_node)
    simulation_thread = threading.Thread(
        target=simulate_emsiso_data, args=(curtis_f_node,), daemon=True
    )
    simulation_thread.start()
    threading.Thread(target=listen, args=(curtis_f_node,), daemon=True).start()

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
