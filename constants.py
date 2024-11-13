"""
Global place for storing all constants that are used within the RedPitaya-Environment
"""

# FPGA-Design-Related
RP_SYS_CLK = 125  # MHz
ADC_MAX_SAMPLE_RATE = 125  # MHz

# RP-ADC:
ADC_TRIGGER_MODE_ASYNC = 0
ADC_TRIGGER_MODE_SYNC = 1

# maximum speed for continous ADC-Sampling
ADC_MAX_SAMPLE_RATE_CONT = 15.625  # MHz

# hostname to RPID
# using this dict we can determine the RP-ID from the hostname
HOSTNAME_RPID = {
    "rp-f09bba": 1,
    "rp-f08c98": 2,
    "rp-f08cad": 3,
    "rp-f09bd0": 4,
    "rp-f0adfb": 5,
    "rp-f0c183": 6,
    "rp-f0cad3": 7,
}


# Config for RAM-Module
ADC_CONTINOUS_MODE = 0
ADC_BLOCK_MODE = 1
ADC_LIA_MODE = 2

# Config for DAC-Modules (Stream: LUT-operation, Single: static output of voltages via TCP)
DAC_MODE_SINGLE = 0  # (ASYNC update for AD-DAC)
DAC_MODE_STREAM = 1  # (SYNC update for AD-DAC)

# to start all bram-dac-ports at once use this ID for start_dac_sweep()
ALL_BRAM_DAC_PORTS = 10

# Trigger-Module
# Aquisition-Mode
# sending triggers for wavelength-measurement, with oversampling
TRIG_MODE_AQU = 0
# Adjustment-Mode
# sending trigger for wavelength-measurement, no oversampling, with adjustmenet of LUT-Values
TRIG_MODE_ADJ = 1

# DAC-Device-IDs
AD_DAC_ID = 0
RP_DAC_ID = 1
LT_DAC_ID = 2

# Port-IDs for AD-DAC
AD_DAC_PORT_A = 0
AD_DAC_PORT_B = 1
AD_DAC_PORT_C = 2
AD_DAC_PORT_D = 3
AD_DAC_ALL_PORTS = 4
AD_DAC_CLOCK_RATIO = 5  # ratio between DAC-Clock and SYS-Clock

# Port-IDs for RP-DAC
RP_DAC_PORT_1 = 0
RP_DAC_PORT_2 = 1
RP_DAC_CLOCK_RATIO = 1  # ratio between DAC-Clock and SYS-Clock
# max binary value for 16bit RP-DAC-Voltage in binary representation
RP_DAC_MAX_VALUE = 16384
# put clock divider ratios in array and select the requested one via DAC_ID
DAC_CLOCK_RATIO = [AD_DAC_CLOCK_RATIO, RP_DAC_CLOCK_RATIO]

# Max Voltage Settings for DAC-Conversion Functions..
# TODO: Move max-voltage-Constants to a more project specific location since this is different for each project
MAX_PDM_VOLTAGE = 1.8  # V max. voltage on PDM
MAX_RF_DAC_VOLTAGE = 2.0  # V max. voltage , with 1V Offset (-1V to +1V).
MAX_AD_DAC_VOLTAGE = 10  # V max. voltage on AD-DAC
VALUE_RES = 16  # 16bit resolution for transmission of values transmitted
MAX_VOLTAGE_PER_CHANNEL = [7.5, 7.5, 6]  # for AD-DAC in static operation

LUT_VALUE_PRECISION = 6  # round lut-values to 6 decimal places

# Command-IDs for App-Server: ##################################################################################
# Constants for creating TCP-Command-Message
SHIFT_ID = 24  # shift id by 24 bits to left
SHIFT_CH = 16  # shift channel information by 16 bits to the left
MSG_SIZE = 32  # size of the tcp-command (in bit)

# set voltage on RF-DAC (fast DAC)
SET_RP_DAC = 10
SET_RP_DAC_NO_CALIB = 11  # without calibration
# set voltage on AD-DAC (external)
SET_AD_DAC = 15
# set voltage on PDM (slow DAC)
SET_PDM = 20
# enable PDM channels
EN_PDM = 25
# get voltage from XADC (slow ADC)
GET_XADC = 30
# get voltage from RF-ADC (fast ADC) ... to be implemented
GET_RF_ADC = 40
# Initialise ADC24 Click Board
INIT_ADC24 = 41
# Initialise ADC20 Click Board
INIT_ADC20 = 42
# get voltage from ADC20CLick
GET_VOLTAGE_ADC20 = 43
# start sequence mode to sample multiple channels on ADC20 Click
START_SEQ_SAMPLING_ADC20 = 44
# get voltage from ADC24 Click
GET_VOLTAGE_ADC24 = 45
# start sequence mode to sample multiple channels on ADC24 Click
START_SEQ_SAMPLING_ADC24 = 46
# command to close the SPI interface
CLOSE_SPI = 47
# get adc cnt (ditial value) from rf adc
GET_RF_ADC_CNT = 50
# 4 user leds (right side, closest to ethernet port)
SET_LED = 60

# Start and Stop DAC-Sweep (DAC-BRAM-Controller output)
START_DAC_SWEEP = 70  # start DAC-Lut-sweep
START_TRIGGER_SWEEP = 79  # start DAC-Sweep with trigger-handshake for wlength-detection
STOP_DAC_SWEEP = 73  # stop DAC-Lut-sweep
# Commands for Aquisition/Adjustment-Triggers
REQ_WLENGTH = 71  # request wlength measurement
# new wavelength measured => send request to select next trigger for next wavelength-point
NEXT_TRIGGER = 72
ADJ_LUT_VALUE = 74  # adjust LUT-Value inside BRAM on RedPitaya
REARM_TRIGGER = 75  # rearm current trigger
HOLD_TRIGGER = 81  # hold current trigger high
RELEASE_TRIGGER = 82

# Command to send and apply a new config
NEW_CONFIG = 76
CONFIG_DONE = 78

# store Current LUT from BRAM to .csv-file
STORE_LUT = 77

# start sampling data with ADC
START_ADC_SAMPLING = 80

# Command to receive BRAM values
RECV_BRAM_DATA = 83
RECV_BRAM_DATA_DONE = 84

# temrinate client-socket connection on RedPitaya
TERMINATE_CLIENT = 85

# exit RedPitaya-Application
EXIT_APP = 90

# test SPI interface
SPI_TEST = 91

# run debug routine:
DEBUG = 99

# RAM Writer test mode, using Dummy Data Generator
RAM_TEST_BLOCK_MODE = 100
RAM_TEST_CONTINUOS_MODE = 101

START_CLK_DIV = 105
STOP_CLK_DIV = 106

# Multiplexer selection commands
SELECT_RAM_MUX_INPUT = 110
SELECT_ADC_MUX_INPUT = 111

# LIA Testcases
LIA_DEBUG = 120

# DAC-BRAM-Commands
GET_DAC_BRAM_SAMPLE_CNT = 130
GET_DAC_BRAM_SIGNAL_CNT = 131

# Acknowledge signal (used for all Comands where we need an ACK-Feedback (both ways))
ACK = 1

# Dummy-Data-Generator Modes
DUMMY_COUNTER_MODE = 0
DUMMY_CONTINOUS_MODE = 1

# LIA-Mixer Configuration
LIA_MIXER_CHAN_1 = 0
LIA_MIXER_CHAN_2 = 1
LIA_MIXER_NORMAL_MODE = 0
LIA_MIXER_DEBUG_MODE = 1

# Inputs for ADC Interface 4x1 AXIS-Multiplexer
ADC_MUX_IN_RP_ADC = 0
ADC_MUX_IN_DUMMY = 1

# Inputs for RAM-Writer 8x1 AXIS-Multiplexer
RAM_MUX_IN_FILTER = 3
RAM_MUX_IN_LIA_MIX_SIN_DEBUG = 4
RAM_MUX_IN_LIA_MIX_COS_DEBUG = 5
RAM_MUX_IN_ADC_DEBUG = 6
RAM_MUX_IN_DUMMY_DEBUG = 7

# IDs for sending different Config-Structs to RedPitaya
DAC_CONFIG_ID = 0
ADC_CONFIG_ID = 1
DAC_BRAM_CONFIG_ID = 2
LUT_CONFIG_ID = 3
TRIGGER_CONFIG_ID = 4
RAM_INIT_CONFIG_ID = 5
DUMMY_DATA_GEN_CONFIG_ID = 6
DUMMY_DATA_GEN_BRAM_ID = 7
LIA_MIXER_CONFIG_ID = 8
LIA_MIXER_BRAM_ID = 9
LIA_IIR_CONFIG_ID = 10
CLK_DIVIDER_CONFIG_ID = 11
SPI_CONFIG_ID = 20
READ_REG_ADC20 = 1000
ADC20_DEBUG_CMD = 1001

# More python-related constants:
RP_HOST_IP = "10.42.0.1"  # on linux computers we use this as host-ip for ethernet-port

# SPI modes
SPI_MODE0 = 0
SPI_MODE1 = 1
SPI_MODE2 = 2
SPI_MODE3 = 3

# SPI bit-order
SPI_MSB_FIRST = 0
SPI_LSB_FIRST = 1
