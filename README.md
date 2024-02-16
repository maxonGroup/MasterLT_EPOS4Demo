# MasterLT_EPOS4Demo

This repository contains the example C++ code and structure of a project to program a maxon motor Australia MiniMaster LT/MicroMaster LT CANopen Master Controller to control an EPOS4 series digital positioning controller. 
You may use this as a "getting started" point from which you can develop your own custom application. 


<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about">About</a>
      <ul>
        <li><a href="#hardware-required">Hardware Required</a></li>
        <li><a href="#the-demo-program">The Demo Program</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#installation">Installation</a></li>
        <li><a href="#uploading">Uploading</a></li>
      </ul>
    </li>
    <li><a href="#useful-documents">Useful Documents</a></li>
    <li><a href="#feedback">Feedback</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>


<!-- ABOUT -->
## About
<p align="center">
    <img src="images\EPOS4Class_Cover.png" alt="maxon motor Australia MiniMaster LT, MicroMaster LT, EPOS4 Series" width="400">
</p>

maxon motor Australia's MiniMaster LT and MicroMaster LT CANopen Master Controllers are powerful PLCs capable of control of maxon EPOS4 series of digital positioning controllers. 

### Hardware Required 

To run this demo program, you need:

* A MasterLT board (MiniMaster LT/MicroMaster LT + MicroBreakout Evaluation Board)
* An EPOS4 positioning controller
* A CAN connection between the MasterLT board and the EPOS4.
* Power supply (24V)
* Micro-USB cable 
* PC

### The Demo Program

#### main.cpp

The entry point for the demo program is src/main.cpp, ```app_main()```.

The demo program provides a working example to configure the CAN bus, initialise the connected EPOS4 device, and perform motion examples on the connected motor. 

The demo program imports the [MasterLT_EPOS4Library](https://github.com/maxonGroup/MasterLT_EPOS4Library). 
Please refer to the [EPOS4 Class Library Documentation](www.google.com.au) for the software reference for the MasterLT_EPOS4Library.

The sequence of motion and actions of the connected motor and its driver are:

1. Enable the connected EPOS4.
2. In Profile Velocity Mode (PVM), move the motor at 120 RPM for 1 second.
3. In Profile Position Mode (PPM), move the motor 1000 encoder quadcounts (there are 4 quadcounts per "counts per turn").
4. Reconfigure Profile Acceleration and Profile Velocity.
5. In PPM, launch PDO SYNC motion of 4000 quadcounts.
6. Reconfigure Profile Acceleration and Profile Velocity to previous values. 
7. In a PPM loop, wait 3 seconds, move the motor 500 quadcounts, repeat.


The main program also contains threads/tasks that enable simultaneous programs to run on the master PLC. 

The receiver task handles incoming CAN bus messages into the PLC.
```cpp
static void receiverTask(void *pvParameters)
```

The heartbeat task handles broadcasting heartbeats to the CAN bus for monitoring communication bus health and power loss detection.
```cpp
static void heartbeatTask(void *pvParameters)
```


The PDO configuration function provides an example of setting up a receive PDO for motion control.
```cpp
ERROR_CODE_t PDOHelper(EPOS4 &node)
```

#### main.hpp

The main header file contains defines for the LED pins on the hardware.

The MiniMaster LT has three LED pins, whilst the MicroMaster LT has no on board LEDs. 

The header file shows example of using ```#ifdef``` to have separate code for MicroMaster LT and MiniMaster LT.

<!-- GETTING STARTED -->
## Getting Started

The instructions provided here are aimed to setup Visual Studio Code (VS Code) as the IDE. 

* Install [VS Code](https://code.visualstudio.com/)
* Add the platformio extension
* Add C/C++ instellisense extension (if not done automatically)
* Download [Git](https://git-scm.com/downloads)


### Installation

1. Open VSCode, and Authenticate use of Github for VSCode and sign in to github.
2. Clone this repo
   ```sh
   git clone https://github.com/maxonGroup/MasterLT_EPOS4Demo.git
   ```
2. Open project folder in VS Code
3. Save workspace (optional)
4. Build

The plaformio.ini file included should trigger the creation of a .pio folder containing all dependencies. The .ini file contains the following code:

```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html
[env:development]
platform = espressif32@6.5.0
board = esp32doit-devkit-v1
framework = espidf
monitor_speed = 115200
debug_tool = esp-prog
monitor_filters = esp32_exception_decoder
lib_deps = https://github.com/maxonGroup/MasterLT_EPOS4Library.git#v0.10.0
```

A key point of interest is the ```lib_deps``` field. This contains a reference to the public MasterLT_EPOS4Library.git repository, specifically to a tagged version of the repository.
To access a different tagged version or release of the MasterLT_EPOS4Library, modify the version number after the  ```#```.
Rebuild the platformio project after the .ini file has been modified. 


### Uploading

After plugging in the board to a USB port, confirm that the device driver is correctly detected by the host. The COM port should be detected automatically if it's correctly detected by the host. 

Upload the main program from VS Code. 


## Useful Documents

* [EPOS4 Class Library Documentation](https://www.google.com/) 
* [MiniMaster LT Hardware Reference](https://www.google.com/)
* [MicroMaster LT Hardware Reference](https://www.maxongroup.net.au/medias/sys_master/root/9224075935774/2401-MicroMaster-LT-Hardware-Reference.pdf)


<!-- Feedback -->
## Feedback

See the [open issues](https://github.com/maxonGroup/MasterLT_EPOS4Demo/issues).

Please post issues and comments here. 


<!-- CONTACT -->
## Contact

For any queries, please contact:
* Dr. Carlos Bacigalupo - carlos.bacigalupo@maxongroup.com
* Mr. Mihir Joshi - mihir.joshi@maxongroup.com

Project Link: [https://github.com/maxonGroup/MasterLT_EPOS4Demo](https://github.com/maxonGroup/MasterLT_EPOS4Demo.git)

<!-- Links, etc -->
[board-small]: images\PCB_v1_s.png
[board-boot]: images\PCB_v1_boot.png