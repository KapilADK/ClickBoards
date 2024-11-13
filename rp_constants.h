/*
 * rp_constants.h
 *
 *  Created on: 27.11.2023
 *      Author: walter
 *
 *    Put all used constants in the C-Lib source inside here:
 *
 *
 *     -- Command IDs (TCP)
 *
 *     -- Device specific constants (ADCs,DACs...)
 *
 *     -- TCP-Communication
 *
 *     -- RAM-Init...
 *
 *
 */

#ifndef SRC_RP_CONSTANTS_H
#define SRC_RP_CONSTANTS_H

// for SC_PAGESIZE
#include <unistd.h>
// for CMA_ALLOC:
#include <math.h>
#include <sys/ioctl.h>

///////////////////////////////////////////////////////////////////////////////////////
// DAC-BRAM-Controller
// when enabling dac-bram-controller select this to enable all Ports at once in snyc
#define ALL_BRAM_DAC_PORTS 10

///////////////////////////////////////////////////////////////////////////////////////
// ADC-Constants:
#define XADC_RESOLUTION 12
#define ADC_RESOLUTION 14
#define ADC_VOLTAGE_RANGE 20
#define ADC_MAX_VALUE 8191

// defines for selecting between BLOCK-MODE and CONTINOUS-MODE
#define ADC_CONTINOUS_MODE 0
#define ADC_BLOCK_MODE 1
#define ADC_LIA_MODE 2

///////////////////////////////////////////////////////////////////////////////////////
// AXI-Devices:
#define AXI_SLAVE_REG_RANGE sysconf(_SC_PAGESIZE)
#define AXI_BRAM_RANGE 16 * sysconf(_SC_PAGESIZE)

// RAM-Writer: ////////////////////////////////////////////////////////////////////////
// highest, in C-SW reachable address (STATUS_REG)
#define HIGHEST_POS_ADDR 0xFFFFF
#define HIGHEST_POS_ADDR_CONT_MODE 0xFFFF0

// config for CMA-Alloc-Command
#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

// mode for RAM-Writer
#define RAM_WRITER_CONTI_MODE 0
#define RAM_WRITER_BLOCK_MODE 1

///////////////////////////////////////////////////////////////////////////////////////
// TCP-Config
#define TCP_PORT 1002

///////////////////////////////////////////////////////////////////////////////////////
// Command IDs from TCP-App-Server-Comunication..
// Command IDs for RedPitaya Control:
#define NO_BITS_VAL 16  // sending values with 16 Bit resolution
#define SHIFT_ID 24     // shift id by 24 bits to right
#define SHIFT_CH 16
// Set voltage on RF-DAC (fast DAC)
#define SET_RP_DAC 10
#define SET_RP_DAC_NO_CALIB 11  // without calibration
// Set voltage on AD-DAC (external)
#define SET_AD_DAC 15
// Set voltage on PDM (slow DAC)
#define SET_PDM 20
// Enable PDM channels
#define EN_PDM 25
// Get voltage from XADC (slow ADC)
#define GET_XADC 30
// Get voltage from RF-ADC (fast ADC) ... to be implemented
#define GET_RF_ADC 40
// Initialise ADC24Click
#define INIT_ADC24 41
// Initialise ADC20Click
#define INIT_ADC20 42
// Get voltage from ADC20Click
#define GET_VOLTAGE_ADC20 43
// Start sequence mode to sample multiple channels on ADC20 Click
#define START_SEQ_SAMPLING_ADC20 44
// Get voltage from ADC24 Click
#define GET_VOLTAGE_ADC24 45
// Start sequence mode to sample multiple channels on ADC24 Click
#define START_SEQ_SAMPLING_ADC24 46
// Command to close the SPI interface
#define CLOSE_SPI 47
// Get ADC count (digital value) from RF ADC
#define GET_RF_ADC_CNT 50
// Set LED on the RedPitaya board
#define SET_LED 60
// Start and Stop DAC-Sweep (DAC-BRAM-Controller output)
#define START_DAC_SWEEP 70      // Start DAC-LUT-sweep
#define START_TRIGGER_SWEEP 79  // Start DAC-Sweep with trigger-handshake for wlength-detection
#define STOP_DAC_SWEEP 73       // Stop DAC-LUT-sweep
// Commands for Acquisition/Adjustment-Triggers
#define REQ_WLENGTH         71  // Request wavelength measurement
#define NEXT_TRIGGER        72  // send request to select next trigger for the next wavelength-point
#define ADJ_LUT_VALUE       74  // Adjust LUT-Value inside BRAM on RedPitaya
#define REARM_TRIGGER       75  // Rearm current trigger
#define HOLD_TRIGGER		81  // hold current trigger high
#define RELEASE_TRIGGER     82  // release current trigger
// Command to send and apply a new config-struct
#define NEW_CONFIG 76
#define CONFIG_DONE 78
// Store Current LUT from BRAM to .csv-file with _adj-suffix
#define STORE_LUT 77
// Start sampling data with ADC
#define START_ADC_SAMPLING 80
// Command to receive new BRAM data
#define RECV_BRAM_DATA 83
#define RECV_BRAM_DATA_DONE 84
// Terminate Client connection
#define TERMINATE_CLIENT 85
// Exit RedPitaya-Application
#define EXIT_APP 90
// Test SPI Interface
#define SPI_TEST 91
// Debug command (for testing)
#define DEBUG 99
// debug adc20 read and write
#define READ_REG_ADC20 1000
// debug command
#define ADC20_DEBUG_CMD 1001

// RAM Writer commands
#define RAM_TEST_BLOCK_MODE 100
#define RAM_TEST_CONTI_MODE 101

// commands for clock divider
#define START_CLK_DIV 105
#define STOP_CLK_DIV 106

// Multiplexer selection commands
#define SELECT_RAM_MUX_INPUT 110
#define SELECT_ADC_MUX_INPUT 111

// LIA-Mixer commands
#define LIA_DEBUG 120

// DAC-BRAM commands
#define GET_DAC_BRAM_SAMPLE_CNT 130
#define GET_DAC_BRAM_SIGNAL_CNT 131

// Acknowledge signal (used for all commands where we need an ACK-Feedback (both ways))
#define ACK 1
// for handling a client which closes the connection
#define CLIENT_DISCONNECT 0
#define SERVER_ERROR_ID -1
// config-ids for different IPs
#define DAC_CONFIG_ID 0
#define ADC_CONFIG_ID 1
#define DAC_BRAM_CONFIG_ID 2
#define LUT_CONFIG_ID 3
#define TRIGGER_CONFIG_ID 4
#define RAM_INIT_CONFIG_ID 5
#define DUMMY_DATA_GEN_CONFIG_ID 6
#define DUMMY_DATA_GEN_BRAM_ID 7
#define LIA_MIXER_CONFIG_ID 8
#define LIA_MIXER_BRAM_ID 9
#define LIA_IIR_CONFIG_ID 10
#define CLK_DIVIDER_CONFIG_ID 11
#define SPI_CONFIG_ID 20

///////////////////////////////////////////////////////////////////////////////////////
// MISC:
#define MAX_DATA_LENGTH 16384
#define MAX_CHAR_SIZE MAX_DATA_LENGTH * 10  // no. of allowed char in lut-csv-file..-> each sample has ~10 chars...

// APP-Server modes... needed?
#define APP_SERVER_MODE_STATIC 0
#define APP_SERVER_MODE_TUNING 1
#define APP_SERVER_MODE_ADC 2
#define APP_SERVER_MODE_ADJ_ADC 3
#define APP_SERVER_MODE_DEBUG 4
#define APP_SERVER_MODE_NOT_FOUND -1

// Trigger Modes for app-server (tuning-mode)
#define TRIG_MODE_AQU 0  // use multiple triggers, check which delay we need for WaveMeter
#define TRIG_MODE_ADJ 1  // use just one trigger per sample and adjust DAC-values during sweep

#endif
