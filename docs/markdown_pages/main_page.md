@mainpage Project work 2024

# About the project
This project work was made for the "Projektfeladat mechatronikusoknak" course of the 2024/25/I semester.

The goal of the project is to demonstrate the student's capabilities in creating an integrated software.

The project itself is a measurement station, that is capable of recording real-life measurements and storing them in an EEPROM.

# Commands

|Command        |Effect                                                     |Argument|
|---------------|-----------------------------------------------------------|--------|
|enterMeas      |Enter Meas state                                           |-|
|enterComm      |Enter Comm state                                           |-|
|getState       |Display the current state                                  |-|
|whoami         |Display 'MEAS_STATION'                                     |-|
|setFrequency   |Set the measurement frequency in seconds                   |uint32 integer |
|displayMeas    |Sets if the measurement is displayed                       |ON or OFF|
|IDLE           |If current state is Comm, enter thea Comm state IDLE       |-|
|INIT           |If current state is Comm, enter thea Comm state INIT       |-|
|READOUT        |If current state is Comm, enter thea Comm state READOUT    |-|