import serial
import serial.tools.list_ports
import time
import pandas as  pd
import struct
from datetime import datetime, timezone, timedelta
from tqdm import tqdm

###
# @brief Class to access the functions of the MEAS station
# 
# @note The serial port is freed automatically
# 
class MeasStation:

    ###
    # @brief Constructor for the MeasStation class
    # 
    # The device is automatically detected
    # #
    def __init__(self) -> None:
        if( self.autoDetect() == False ):
            raise Exception("Instance could not be initialized as autodetection failed!")

    ###
    # @brief Automatically detects the COM-port that is used by the device
    # #
    def autoDetect(self) -> bool:
        print("searching for device...")
        ports = serial.tools.list_ports.comports()

        for port, desc, hwid in sorted(ports):
            print(f"\t in {port}:")
            try:
                response = self._send_command_single_reply(port, "whoami")

                if("MEAS_STATION" in response):
                    print("\t\tfound")

                    ###
                    # @brief The COM-port used by the device (e.g.: COM9). (string)
                    # #
                    self.serialPort = port
                    return True
                else:
                    print("\t\tdifferent device on port")
            except:
                print(f"\t\tport {port} could not be opened.")
        return False

    ###
    # @brief Sends a command to the device, without returning any response
    # 
    # @param port       The COM port that will be used e.g.: "COM9"
    # @param command    The command that will be sent to the device e.g.: "enterComm"
    # #
    def _send_command_no_reply(self, port: str, command: str) -> None:
        with serial.Serial(port, 115200, timeout=1) as serialPort:
            serialPort.write((command + '\r\n').encode())

    ###
    # @brief Sends a command to the device and returns the first line 
    # 
    # @param port       The COM port that will be used e.g.: "COM9"
    # @param command    The command that will be sent to the device e.g.: "getState"
    #
    # @note Example of use: During MEAS state if displayMeas is turned on, 
    # the measured data is constantly being printed. In order not to get stuck reading these replies this command can be used
    # 
    def _send_command_single_reply(self, port: str, command: str) -> str:
        with serial.Serial(port, 115200, timeout=1) as serialPort:
            serialPort.write((command + '\r\n').encode())
            line = serialPort.readline().decode('utf-8').strip()
            return line

    ###
    # @brief Sends a command to the device and returns the first line 
    # 
    # @param command    The command that will be sent to the device e.g.: "getState"
    #
    # @note Example of use: During MEAS state if displayMeas is turned on, 
    # the measured data is constantly being printed. In order not to get stuck reading these replies this command can be used
    # 
    def send_command_single_reply(self, command: str) -> str:
        with serial.Serial(port, 115200, timeout=1) as serialPort:
            serialPort.write((command + '\r\n').encode())
            line = serialPort.readline().decode('utf-8').strip()
            return line

    ###
    # @brief Sends a command to the device and returns the replies
    # 
    # @param command    The command that will be sent to the device e.g.: "getState"
    #
    # @note Not to use if for the device is periodically sending some data e.g.: MEAS mode if displayMeas is turned on 
    # 
    def send_command(self, command: str) -> list:
        with serial.Serial(self.serialPort, 115200, timeout=1) as serialPort:
            serialPort.write((command + '\r\n').encode())
            lines = serialPort.readlines()
            return lines

    ###
    # @brief Enter the MEAS mode of the device
    #
    # @param display    If True, every time a measurement is taken the data is sent via serial. Else the device will measure in "silence". (default value is False)
    #
    # #
    def enterMeasMode(self, display: bool = False) -> bool:
        self._send_command_no_reply(self.serialPort, "enterMeas")
        
        displayState = "ON" if display else "OFF"

        self._send_command_no_reply(self.serialPort, f"displayMeas {displayState}")

        if( self._send_command_single_reply(self.serialPort, "getState") == "MEAS"):
            return True
        else:
            return False
            
    ###
    # @brief Enter the COMM mode of the device
    #
    # @note This state of the device can be used to access the READOUT and INIT modes
    #
    # 
    def enterCommMode(self) -> bool:
        self._send_command_no_reply(self.serialPort, "enterComm")
        if( self._send_command_single_reply(self.serialPort, "getState") == "COMM"):
            return True
        else:
            return False

    ###
    # @brief Initializes the storage on the device with the current time
    # 
    # @warning After this function the device will be in COMM mode, to start the measurement call the enterMeasMode() function to enter the MEAS mode.
    # 
    def initStorage(self) -> None:
        local_now = datetime.now()
        utcTimestamp = (local_now - datetime(1970, 1, 1)).total_seconds()

        self.enterCommMode()
        self._send_command_no_reply(self.serialPort, f"INIT {utcTimestamp}")

    ###
    # @brief Reads out the storage on the device and returns a pandas data frame containing the records.
    # 
    # @warning After this function the device will be in COMM mode, to start the measurement call the enterMeasMode() function to enter the MEAS mode.
    # 
    def readoutStorage(self) -> pd.DataFrame:
        with serial.Serial("COM9", 115200, timeout=1) as serialPort:
            serialPort.write(("enterComm" + '\r\n').encode())
            serialPort.write(("READOUT" + '\r\n').encode())
            line = serialPort.readline().decode('utf-8').strip()
            timeStamp = int(line.split(';')[0])
            entryCnt = int(line.split(';')[1])

            # Get date time from the timestamp
            startTime = datetime.fromtimestamp(timeStamp, timezone.utc)
            
            CumulativeTime = 0
            data = []
            for i in tqdm(range(entryCnt), desc="Loading..."):
                raw_line = serialPort.readline().decode('utf-8').strip()
                fields = raw_line.split(', ')
                
                # Átalakítás és adat hozzáadása a listához
                measurement_type = int(fields[0])
                delta_seconds = int(fields[1])

                CumulativeTime += delta_seconds
                
                time = startTime + timedelta(seconds=CumulativeTime)
                
                # Visszaalakítás float-tá
                uint_value = int(fields[2].strip(';'))
                float_value = struct.unpack('!f', struct.pack('!I', uint_value))[0]
                
                data.append([measurement_type, time, float_value])
            
            if serialPort.readline().decode('utf-8').strip() != "END":
                raise Exception("End signal not received")

            # DataFrame létrehozása
            df = pd.DataFrame(data, columns=['Measurement Type', 'Time', 'Value'])
            return df

    ###
    # @brief Set how often a measurement should be taken in seconds. The smallest increment is 1 second.
    # 
    def setMeasFreq(self, frequency: int) -> None:
        self._send_command_no_reply(self.serialPort, f"setFrequency {frequency}")

    
