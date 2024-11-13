"""
core.py

Main-Class for working with the RedPitayaBoard remotely from a Host-machine

when initalizing the Board it aleady requires a running C-Application on the RedPitaya
(this could be automated in the future with the call to the method: start_remote_server
getting called inside init before connection as tcp-client)

(to implement this we need to make sure to bring the system in a stable state before doing this)


"""

from ctypes import Structure
import numpy as np
import matplotlib.pyplot as plt
import time
from rp.calib_parameters import RP_GO_VALUES
from rp.misc.helpers import print_rp_tag

from rp.constants import (
    ADJ_LUT_VALUE,
    ALL_BRAM_DAC_PORTS,
    CONFIG_DONE,
    EN_PDM,
    GET_DAC_BRAM_SAMPLE_CNT,
    GET_DAC_BRAM_SIGNAL_CNT,
    GET_RF_ADC,
    GET_RF_ADC_CNT,
    GET_XADC,
    MSG_SIZE,
    NEW_CONFIG,
    NEXT_TRIGGER,
    HOLD_TRIGGER,
    RELEASE_TRIGGER,
    REARM_TRIGGER,
    REQ_WLENGTH,
    RP_SYS_CLK,
    SHIFT_ID,
    ACK,
    SHIFT_CH,
    EXIT_APP,
    SET_PDM,
    SET_LED,
    SET_AD_DAC,
    SET_RP_DAC,
    MAX_VOLTAGE_PER_CHANNEL,
    START_ADC_SAMPLING,
    START_DAC_SWEEP,
    STOP_DAC_SWEEP,
    STORE_LUT,
    START_TRIGGER_SWEEP,
    TERMINATE_CLIENT,
    DEBUG,
    RAM_TEST_BLOCK_MODE,
    RAM_TEST_CONTINUOS_MODE,
    HOSTNAME_RPID,
    SET_RP_DAC_NO_CALIB,
    START_CLK_DIV,
    STOP_CLK_DIV,
    SELECT_ADC_MUX_INPUT,
    SELECT_RAM_MUX_INPUT,
    LIA_DEBUG,
    CLOSE_SPI,
    GET_VOLTAGE_ADC24,
    START_SEQ_SAMPLING_ADC24,
    INIT_ADC24,
    INIT_ADC20,
    GET_VOLTAGE_ADC20,
    START_SEQ_SAMPLING_ADC20,
    SPI_TEST,
    READ_REG_ADC20,
    ADC20_DEBUG_CMD,
)

from rp.misc.helpers import scanPortForDevice
from rp.structs import LutValue
from rp.misc import tcp_client as tcp
from rp.misc import scp_server as scp
from rp.adc.helpers import unpackADCData, fix_sign
from rp.devices import howland_bridge as hw
from rp.tuning.lut import LUT
from rp.structs import TcpCommand


class RedPitayaBoard:
    def __init__(
        self, ip="", port=1002, debug=False, verbose=False, autoStartServer=False
    ):
        """
        Inializes the TCP-connection with the RedPitaya-Board connected via Ethernet

        After we connected as a Client to the tcp-server running in C on the RP
        we can send commands via TCP/IP to controll the RedPitaya:

                - set voltage on DACs (either internal or external connected devices)
                - read out ADC-Voltages

                - Config:
                        - this configures the various IP-Cores inside the RedPitaya-FPGA:
                                - ADC-Config
                                - DAC-Config
                                - DAC-BRAM-CONTROLLER-Config
                                - Aquisition-Trigger-Config
                                - RAM-Config
                                        - ??
                                        - select between which interface (ADC/dummy-ADC) is connected to RAM-Interface

                        - this allows us to fully configure each different module

                - start and stop DAC-BRAM-Outputs:
                        - this starts outputting the voltages stored in each LUT on selected device

                - start ADC-Sampling
                        - either we configured the ADC:
                                - to be synchronized with a DAC-BRAM-Controller interface
                                - to be "free running" with internal

                - send LUT-Files to RedPitaya for (DAC-BRAM-CONTROLLER/LockIn-Amp)

                - (FUTURE):
                        - Enable Lockin

                - exit / close application

        """
        self.hw_debug = debug  # debug mode => not using tcp methods
        self.verbose = verbose
        self.ip = ip
        self.port = port
        print_rp_tag()
        self.printImageInfo()

        self.id = self.get_rp_id()  # get RP-ID from hostname using ssh
        # dont do this now.. this could be something for the future
        # start C-Appliation:
        if autoStartServer:
            self.startServerApplication()

        # connect to running tcp server on RedPitaya-C-Application
        self.connectToTcpServer()

    def connectToTcpServer(self):
        # check if IP was provided when creating RedPitaya-Object
        # if not scan port for connected RedPitaya to get IP
        if self.ip == "":
            self.ip = self.scanPortForRedpitaya()

        # create tcp client, from here we can send and receive
        if not (self.hw_debug):
            self.rp_tcp = tcp.TCP_CLIENT(self.ip, self.port, verbose=self.verbose)

            if self.rp_tcp.connected:
                print(f"RedPitayaBoard RP{self.id} connected succesfully!")
            else:
                print(f"RedPitayaBoard RP{self.id} not found!")
                print("Be sure to start the C-Application before")
        else:
            print("Started RedPitaya in Debug-Mode")

    def scanPortForRedpitaya(self, host_ip="10.42.0.1") -> str:
        """
        scans ethernetport at 10.42.0.1 and searches for connected
        RedPitaya device
        returns local IP of RedPitaya
        """
        if not (self.hw_debug):
            rp_ip = scanPortForDevice(host_ip, "RedpitayaBoard")
        else:
            rp_ip = ""

        return rp_ip

    def printImageInfo(self):
        """
        method to read the info.txt saved in SD-Card which contains info
        about fpga- and linux-image.
        """
        if self.ip == "":
            self.ip = self.scanPortForRedpitaya()

        scpCon = scp.SCP_SERVER(self.ip, "root", "root")

        print("Reading the fpga- and linux-image on the SD-Card.......")
        print("")

        exitStatus, response = scpCon.execute("cd /media/mmcblk0p1 && cat info.txt")
        output = "".join(response)
        print(output)
        scpCon.close()

    def get_rp_id(self) -> int:
        """
        returns ID of conencted RedPitaya-Board
        """

        # check if IP was provided when creating RedPitaya-Object
        # if not scan port for connected RedPitaya to get IP
        if self.ip == "":
            self.ip = self.scanPortForRedpitaya()

        hostname = ""

        if not (self.hw_debug):
            scpCon = scp.SCP_SERVER(self.ip)
            hostname = scpCon.get_hostname()
            scpCon.close()

        # get id from hostname-struct:
        try:
            rp_id = HOSTNAME_RPID[hostname]  # [0].replace("\n", "")]
        except KeyError:
            rp_id = 1
            print(f"Error: RP-ID could not be resolved correctly, Select RP-ID {rp_id}")

        return rp_id

    def startServerApplication(self):
        """
        method to start c-application from remote..
        .. this is not used for now but could be used later in an application
        """
        app = "./server"

        if self.ip == "":
            self.ip = self.scanPortForRedpitaya()

        scpCon = scp.SCP_SERVER(self.ip, "root", "root")

        exitStatus, response = scpCon.execute("ps")
        print("\nChecking if server instance is running.......")

        for process in response:
            if app in process:
                ps_id = process.lstrip().split()[0]
                kill_ps = f"kill -9 {ps_id}"
                scpCon.execute(kill_ps)
                print("Killed running server instance......")
        print("")
        print("Start server application on RP")
        app = f"cd app/ && setsid {app} &>/dev/null"
        scpCon.execute(app)
        scpCon.close()
        print("Started Server-Application on RedPitaya\n")

    def sendCommand(self, cmd_id: int, value: float | int = 0, channel: int = 0):
        """general method to assemble a command from id, value and channel and send it to RedPitaya

        cmd_id is mandatory, value and ch are optional arguments

        rp_command-strut:
            - cmd_id, int
            - value, float
            - channel, int
        """
        # create tcp command struct
        rp_command = TcpCommand(cmd_id, value, channel)

        if not (self.hw_debug):
            self.rp_tcp.send_struct(rp_command)
        if self.verbose:
            print(
                f"SEND: ID: {rp_command.id} | VAL: {rp_command.val} | CH: {rp_command.ch}"
            )

    def waitForAnswer(self, answerID: int = ACK):
        """
        used in other methods that need some kind of sync inbetween host and RP

        wait for any acknowledge from RedPitaya:
        can be ACK (default) or any other Command-ID (depending on context)

        """
        if self.verbose:
            print("wait for answer from RedPitaya...")
        if not (self.hw_debug):
            # wait for Acknowlegde from RedPitaya:
            while True:
                response = self.rp_tcp.receive_int()
                if response == answerID:
                    break
                else:
                    print("received wrong response...")

    def close(self):
        """
        closes c-client-socket on RP and python-socket on host
        """
        if not (self.hw_debug):
            # close client-socket in C
            self.sendCommand(TERMINATE_CLIENT)
            # close python-socket
            self.rp_tcp.close()

    def exitApplication(self):
        """
        exits c-application
        """
        self.sendCommand(EXIT_APP)

    def runDebug(self, value: float = 0, channel: int = 0):
        self.sendCommand(DEBUG, value, channel)

    ############################################################################
    # setter/getter methods for application server "static":
    ############################################################################
    def set_voltage_pdm(self, ch: int, voltage_V: float):
        """
        set voltage on channel 1,2,3,4
        voltage range: 0-1.8 V
        """
        self.sendCommand(SET_PDM, voltage_V, ch)

    def set_leds(self, val: float | int):
        """
        set led to certain value ( 1 = on, 0 = off)
        """
        self.sendCommand(SET_LED, value=val)

    def enable_pdm_channel(self, ch: int):
        """
        enable PDM channel 1,2,3,4
        send val = 1 to enable
        """
        self.sendCommand(EN_PDM, value=1, channel=ch)

    def disable_pdm_channel(self, ch: int):
        """
        enable PDM channel 1,2,3,4
        send val = 0 to disable
        """
        self.sendCommand(EN_PDM, value=0, channel=ch)

    def set_voltage_rp_dac(self, ch: int, voltage_V: float):
        """
        set voltage on RP-DAC (fast DAC)

        for chanel 1,2
        voltage range: 0-1V (not using neg. range for now)
        """
        self.sendCommand(SET_RP_DAC, value=voltage_V, channel=ch)

    def set_voltage_rp_dac_no_calib(self, ch: int, voltage_V: float):
        """
        set raw voltage on RP-DAC (fast DAC) (used only when calibrating the dac)

        for chanel 1,2
        voltage range: -1V to 1V
        """
        self.sendCommand(SET_RP_DAC_NO_CALIB, value=voltage_V, channel=ch)

    def set_voltage_ad_dac(self, ch: int, voltage_V: float):
        """
        set voltage on AD-DAC (external DAC-Board)
        for channel A B and C (0,1,2)
        """
        # check for overvoltage and if voltage exceeds this value clamp it to MAX-VAL
        if voltage_V > MAX_VOLTAGE_PER_CHANNEL[ch]:
            voltage_V = MAX_VOLTAGE_PER_CHANNEL[ch]

        self.sendCommand(SET_AD_DAC, value=voltage_V, channel=ch)

    def set_current_ad_dac(self, ch: int, current_mA: float):
        """
        set current using howland bridge (u-i converter)
        calculate current using transfer-function
        """
        voltage_V = hw.convert_i_u(current_mA)
        self.set_voltage_ad_dac(ch, voltage_V)

    def get_voltage_slow(self, ch: int):
        # send read voltage command for specified channel from XADC

        # send get command to receive voltage read from channel
        # convert received voltage in mV to V
        voltage_V = 0

        # send command to request XADC-ReadOut
        self.sendCommand(GET_XADC, channel=ch)

        if not (self.hw_debug):
            voltage_V = fix_sign(self.rp_tcp.receive_int(), MSG_SIZE) * 1e-3
        else:
            voltage_V = 0

        return round(voltage_V, 6)

    def get_voltage_fast(self, ch: int, no_avg=1):
        # returns voltage in V

        voltage_V = 0

        # send read voltage command for fast ADCs
        self.sendCommand(GET_RF_ADC, value=no_avg, channel=ch)

        if not (self.hw_debug):
            # convert unsigend to signed integer
            voltage_V = fix_sign(self.rp_tcp.receive_int(), MSG_SIZE) * 1e-3
        else:
            voltage_V = 0

        return round(voltage_V, 6)

    def get_adc_cnt_fast(self, ch: int):
        """
        returns adc-value of fast rf adc => used for calibration
        """

        # send command to RP to measure RF-ADC-CNT (bin)
        self.sendCommand(GET_RF_ADC_CNT, channel=ch)

        adc_cnt = 0

        # receive measured adc-count from RedPitaya
        if not (self.hw_debug):
            adc_cnt = self.rp_tcp.receive_int()

        return adc_cnt

    def get_dac_bram_signal_rate(
        self, port: int = 1, dac_clk_freq_MHz: float = RP_SYS_CLK
    ) -> float:
        """
        read out current signal rate in kHz fromd dac-bram-controller for given port
        """
        signal_rate_cnt = 1

        if not (self.hw_debug):
            self.sendCommand(GET_DAC_BRAM_SIGNAL_CNT, channel=port)
            signal_rate_cnt = self.rp_tcp.receive_int()

        signal_rate_kHz = (dac_clk_freq_MHz * 1e3) / signal_rate_cnt

        return signal_rate_kHz

    def get_dac_bram_sample_rate(
        self, port: int = 1, dac_clk_freq_MHz: float = RP_SYS_CLK
    ) -> float:
        """
        read out current sample rate in kHz fromd dac-bram-controller for given port
        """

        sample_rate_cnt = 1

        if not (self.hw_debug):
            self.sendCommand(GET_DAC_BRAM_SIGNAL_CNT, channel=port)
            sample_rate_cnt = self.rp_tcp.receive_int()

        sample_rate_kHz = (dac_clk_freq_MHz * 1e3) / sample_rate_cnt

        return sample_rate_kHz

    ############################################################################
    # Methods for configuring and controlling the RedPitaya-Board
    ############################################################################

    def sendConfigParams(self, configParams: Structure, configID: int):
        """
        method to send config-params packaged in c-struct via TCP
        """
        if not (self.hw_debug):

            if self.verbose:
                print(f"Send new configParams for ID: {configID} to RedPitaya...")

            # send command to RP to send a new Config
            self.sendCommand(NEW_CONFIG, value=configID)

            if self.verbose:
                print("wait for response from RedPitaya....")

            # wait for ACK from RedPitaya to be ready to receive a new Config
            self.waitForAnswer(answerID=ACK)

            if self.verbose:
                print("Sending struct to RedPitaya....")

            # send new config params:
            self.rp_tcp.send_struct(configParams)

            # wait for response from Config-Command:
            self.waitForAnswer(answerID=CONFIG_DONE)

        if self.verbose:
            print("Done sending Config Params...")

    def start_adc_sampling(self, noTcpPackages: int):
        """
        send start-adc-sampling command to RedPitaya to C-Application
        containing the no. of tcp packages we expect from the RedPitaya
        """
        if self.verbose:
            print("send start sampling command")

        self.sendCommand(START_ADC_SAMPLING, value=noTcpPackages)

    def receive_adc_data_package(self, tcp_pkg_size_bytes: int, time_out: bool = False):
        """
        Receive a ADC-TCP-Package comtaining TCP_PACKAGE_SIZE amount of ADC-Samples
        burstPackageSize: amount of tcp-packages we want to receive in a burst
        """
        if self.verbose:
            print("receiving ADC package from RedPitaya")
            print(f"receiving {tcp_pkg_size_bytes} Bytes")

        resp = self.rp_tcp.receive_data(tcp_pkg_size_bytes, time_out)
        return resp

    def sample_lut_sweep(
        self, noSteps: int, noSweeps: int, ch: int, idx: int, plotPreview: bool = False
    ):
        """
        Record voltage on RP-ADC for ADC-ADJ-Mode (laser-tuning)

        packageSizeBytes: no_steps * adc_no_sweeps * 4
        and also telling the adc-sample-command how many tcp-packages we want to receive
        """
        print(f"sample lut sweep with {noSteps} steps and {noSweeps} sweeps")

        packageSizeBytes = noSteps * noSweeps * 4

        print(f"receiving {packageSizeBytes} Bytes from RedPitaya")

        # send command to start sampling
        self.sendCommand(START_ADC_SAMPLING)

        # now we receive data from RedPitaya
        newData_raw = self.rp_tcp.receive_data(packageSizeBytes)
        newData = unpackADCData(np.frombuffer(newData_raw, dtype=np.int32), self.id)
        print(f"Received data for channel {ch} with {len(newData[ch])} samples")

        if noSweeps > 1:
            newData = np.mean(
                np.reshape(newData[ch], (noSweeps, noSteps)), 0
            )  # Calc mean over captured voltage sweep
        else:
            newData = newData[ch]
        # print(newData[ch])

        if plotPreview:
            plt.plot(newData, marker=".")
            plt.vlines(idx, -1, 1, color="red")
            plt.show()

        return newData

    def start_dac_sweep(self, port: int = ALL_BRAM_DAC_PORTS):
        """
        send start DAC-BRAM-Controller-sweep command to RedPitaya for
        """
        print("Start Laser Tuning...")

        # create command with ID and the BRAM-DAC-Port as channel
        self.sendCommand(START_DAC_SWEEP, channel=port)

    def stop_dac_sweep(self, port: int = ALL_BRAM_DAC_PORTS):
        """
        send command to stop laser tuning
        """
        self.sendCommand(STOP_DAC_SWEEP, channel=port)

    def start_trigger_sweep(self, bram_port: int, noTotalTriggers: int):
        """
        send command to start dac-sweep with trigger-handshaking
        """
        self.sendCommand(START_TRIGGER_SWEEP, value=noTotalTriggers, channel=bram_port)

    def wait_for_wavelength_request(self):
        """
        stalls programm and waits till wavelength-measurement is requested by RedPitaya Board
        """
        self.waitForAnswer(REQ_WLENGTH)

    def adjust_lut_value(
        self,
        lutValue: LutValue,
    ):
        """
        send request to RedPitaya
        """
        # send request to adjust lut-value in redpitaya
        self.sendCommand(ADJ_LUT_VALUE)

        # wait till RedPitaya is ready to receive a new lut value
        self.waitForAnswer(answerID=ACK)

        if not (self.hw_debug):
            # send new lutValue to RedPitaya
            self.rp_tcp.send_struct(lutValue)

    def select_next_wlength_trigger(self):
        """
        send command to RedPitaya(PS->PL) to select next wlength trigger
        """
        # select next trigger
        self.sendCommand(NEXT_TRIGGER)

    def rearm_current_trigger(self):
        """
        send command to RedPitaya(PS-PL) to rearm the current trigger again
        """
        # rearm the current trigger
        self.sendCommand(REARM_TRIGGER)

    def hold_current_trigger(self):
        """
        send command to RedPitaya to hold the current trigger
        """

        # send command to hold current trigger
        self.sendCommand(HOLD_TRIGGER)

    def release_current_trigger(self, cooldown_time_ms: float = 0):
        """
        send command to Redpitaya to release a trigger which is currently hold
        """

        # send command to release trigger
        self.sendCommand(RELEASE_TRIGGER)

        # wait for ACK so trigger was released
        self.waitForAnswer(ACK)

        time.sleep(cooldown_time_ms / 1000)

    def store_adjusted_lut_to_file(self, bramPort: int):
        """
        stores current LUT stored in BRAM @ selected Dac-Bram-Ctrl-Port to .csv-file on RedPitaya
        """
        self.sendCommand(STORE_LUT, channel=bramPort)

    def send_lut_file(self, lut: LUT):
        """
        sends LUT (stored as .csv on local machine at sourceLutPath)
        to RedPitaya at apps/lut/ with the given lutFileName
        """
        if not (self.hw_debug):
            scpCon = scp.SCP_SERVER(self.ip)
            scpCon.send_file(lut.local_file, lut.remote_file)
            scpCon.close()

    def receive_lut_file(self, lut: LUT, dest: str, adjusted: bool = True):
        """
        receives a LUT-File (most likely after adjustment)
        from RedPitaya and stores in locally on machine
        Parameter:
            lut: LUT-object (containing paths and filenames for LUT)
            dest: path to where lut file is stored to
            adjusted: select if recveicing adjusted lut
        """
        # ..here we could add a time-stamp to received lut-file...
        # .. for current date... and time?
        # .. and some index or date+time is enough?
        if not (self.hw_debug):
            scpCon = scp.SCP_SERVER(self.ip)
            if adjusted:
                scpCon.receive_file(lut.remote_adj_file, dest)
            else:
                scpCon.receive_file(lut.remote_file, dest)
            scpCon.close()

    def start_remote_server(self):
        """
        function to execute remove Server on RedPitaya (app_server-static)
        Starting the C-Application from Remote-Host (this could be cool to use)
        After this method-call we can start the connection with the client...
        So therefore this should be called inside the init function..????
        """
        if not (self.hw_debug):
            scpCon = scp.SCP_SERVER(self.ip)
            scpCon.execute("./app_server -m static")  # adjust to selected mode...
            scpCon.close()

    ############################################################################
    # Methods for testing the RAM-Writer (ram_writer_tester project)
    ############################################################################

    def start_RAM_test_block_mode(self):
        """
        Start RAM-Test-Block-Mode:
        Write RAM with data from dummy data generator until it's full
        """
        self.sendCommand(RAM_TEST_BLOCK_MODE)

    def start_RAM_test_conti_mode(self, noTcpPackages: int):
        """
        Write RAM with data from dummy data generator. Activate continous mode
        on the RAM writer and send content via TCP over to host PC.
        Data is received using the receive_adc_data_package method.

        Args:
            noTcpPackages (int): Number of TCP packages to be sent from RP
        """
        self.sendCommand(RAM_TEST_CONTINUOS_MODE, value=noTcpPackages)

    def start_clk_div(self):
        self.sendCommand(START_CLK_DIV)

    def stop_clk_div(self):
        self.sendCommand(STOP_CLK_DIV)

    ############################################################################
    # Methods for testing the LIA-Module (lia_impl project)
    ############################################################################
    def select_adc_mux_imput(self, input: int):
        """
        Select the input of the axis-mux
        """
        self.sendCommand(SELECT_ADC_MUX_INPUT, value=input)

    def select_ram_mux_imput(self, input: int):
        """
        Select the input of the axis-mux
        """
        self.sendCommand(SELECT_RAM_MUX_INPUT, value=input)

    def lia_debug(self):
        """
        Write RAM with data from dummy data generator until it's full
        """
        self.sendCommand(LIA_DEBUG)

    def close_spi(self):
        """
        Close the SPI file descriptor and release the resources
        """
        self.sendCommand(CLOSE_SPI)

    def spi_test(self):
        """
        Test the SPI interface. Read back mode, speed and bits per word set.
        """
        self.sendCommand(SPI_TEST)

    def init_adc24(self):
        """
        Initialise ADC24 at the very beginning by sending dummy data. This ensures that the other configurations (command) sent will be correctly set.
        """
        self.sendCommand(INIT_ADC24)

    def get_voltage_adc24(self, ch: int):
        """
        Get the voltage reading from a selected channel on ADC24Click over SPI.
        """
        voltage_V = 0

        # send read voltage command
        self.sendCommand(GET_VOLTAGE_ADC24, channel=ch)

        if not (self.hw_debug):
            # convert unsigend to signed integer
            voltage_V = fix_sign(self.rp_tcp.receive_int(), MSG_SIZE) * 1e-3
        else:
            voltage_V = 0

        return round(voltage_V, 6)

    def start_seq_sampling_adc24(self, ch: int):
        """
        Start sampling data on sequence mode on ADC24Click from channe0 to the user-defined channel.
        """
        self.sendCommand(START_SEQ_SAMPLING_ADC24, channel=ch)

        if not (self.hw_debug):
            data = self.rp_tcp.receive_intArray(ch + 1)
            voltage_V = [round(mV / 1000.0, 6) for mV in data]
        else:
            voltage_V = [0]

        return voltage_V

    def init_adc20(self):
        """
        Initialise ADC20 to select all the channels as analog Input, set sampling rate and output format.
        """
        self.sendCommand(INIT_ADC20)

    def get_voltage_adc20(self, ch: int, average: int = 0):
        """
        Get voltage reading from the selected channel on ADC20Click over SPI. Set the average value from 1 to 7 if you want to enable averaging sampled data using oversampling.
        Average = 0 => 2⁰ = 1, no averaging
        Average = 1 => 2¹ = 2, data is averaged from 2 samples
        Average = 2 => 2² = 4, data is averaged from 4 samples..... and so on.
        """
        voltage_V = 0

        # send read voltage command
        self.sendCommand(GET_VOLTAGE_ADC20, value=average, channel=ch)
        
        if not (self.hw_debug):
            voltage_V = self.rp_tcp.receive_int() * 1e-3
        else:
            voltage_V = 0

        return round(voltage_V, 6)

    def start_seq_sampling_adc20(self, ch: int, average: int = 0):
        """
        Start sampling data on sequence mode on ADC24Click from channe0 to the user-defined channel.
        """
        self.sendCommand(START_SEQ_SAMPLING_ADC20, value=average, channel=ch)

        if not (self.hw_debug):
            data = self.rp_tcp.receive_intArray(ch + 1)
            voltage_V = [round(mV / 1000.0, 6) for mV in data]
        else:
            voltage_V = [0]

        return voltage_V
    
    def read_register_adc20(self, reg_addr: int):
        self.sendCommand(READ_REG_ADC20, channel=reg_addr)
        
    def adc20_debug_communication(self):
        self.sendCommand(ADC20_DEBUG_CMD)
