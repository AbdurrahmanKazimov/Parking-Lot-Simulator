# Parking Lot Simulator with PIC18F8722 and Python

This project implements a multi-storey parking lot management system using a PIC18F8722 microcontroller and a Python-based simulator.

## Overview

- PIC18F8722 firmware controls parking logic, communicates with the simulator via serial port.
- Python program (`cengParkSimulator.py`) simulates cars entering/exiting, displays parking lot state visually using Pygame.
- Supports subscriptions, parking fee calculations, and real-time updates.
- Uses ADC input to select floors, interrupts for button control, and timed serial communication.

## Files

- `main.c`: PIC18F8722 microcontroller firmware source code.
- `pragmas.h`: PIC configuration bits header file.
- `cengParkSimulator.py`: Python parking lot simulator using Pygame and pyserial.

## Requirements

- PIC18F8722 development board, MPLAB XC8 compiler.
- Python 3.10+ with `pygame` and `pyserial` packages installed.
  - Install packages via: `pip install pygame pyserial`
- Correct serial port configuration in `cengParkSimulator.py` (default `/dev/ttyUSB0`).

## How to Run

1. Compile and upload `main.c` to PIC18F8722.
2. Run `cengParkSimulator.py` on host PC.
3. Use the simulator controls to start the simulation and interact.

## Features

- Real-time parking lot state display.
- Automatic and manual modes for adding/removing cars.
- Subscription handling with fees.
- Fee calculation based on parking duration.
- ADC-based floor selection for display.
- Non-flickering 7-segment display output.



