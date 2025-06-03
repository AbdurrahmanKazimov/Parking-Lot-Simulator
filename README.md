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

## How Simulator works
When the simulator is running, there are 2 options:
1. Automatic mode: Activated by pressing the ’A’ key on the computer
keyboard. In this mode, the simulator generates random events every 100 ms. Possible
events include adding a car to the queue, notifying the exit of a parked car, requesting a
subscription, or no event at all.
2. Manuel Mode: Activated by pressing the ’M’ key on the computer
keyboard. In this mode, there are four buttons you can try:

  – ’R’ button: Adds a random car to the queue.
  – ’T’ button: Requests removal of random car from the parking lot
  – ’Y’ button: Requests subscription of a random car to a random place in the parking lot.
  – ’U’ button: Add a random subscribed car to the queue.

## Modes of 7-segment display
7-segment display operates on 2 modes. First one is the default one which shows total accumulated money, and the second one shows the number of empty spaces in the chosen floor. The floor can be chosen using the ADC potentiometer on the board. The switch between these 2 modes happens when RB4 button is released.

## Features

- Real-time parking lot state display.
- Automatic and manual modes for adding/removing cars.
- Subscription handling with fees.
- Fee calculation based on parking duration.
- ADC-based floor selection for display.
- Non-flickering 7-segment display output.



