/*
 * rp_structs.h
 *
 *  Created on: 20.01.2023
 *      Author: walter
 */

#ifndef SRC_RP_STRUCTS_H
#define SRC_RP_STRUCTS_H

#include <stdbool.h>
#include <stdint.h>

#include "proj_constants.h"

// Axi-Devices:
/* Struct to combine all axi instances in one struct
to be passed to every function that uses AXI-Communication from/to FPGA */
typedef struct {
    void* reset;
    void* bram[NO_DAC_BRAM_INTERFACES_USED];
    void* dac_bram_ctrl[NO_DAC_BRAM_INTERFACES_USED];
    void* bram_sync;
    void* trigger_gen;
    void* rp_dac;
    void* ad_dac;
    void* pdm;
    void* leds;
    void* xadc;
    void* adc;
    void* adc_mux_4x1;
    void* dummy_data_gen;
    void* clk_divider;
    void* dummy_data_gen_bram;
    void* ram;
    void* ram_mux_8x1;
    void* lia_mixer;
    void* lia_mixer_bram;
    void* lia_iir;
} AxiDevs;
//////////////////////////////////////////////////////////////////////////////////

typedef struct {
    int serverMode;
    bool verbose;
} AppMode;

// DEVICES: ///////////////////////////////////////////////////////////////////////
// RAM-Init-Config
typedef struct {
    uint32_t sts_width_mask;
    uint32_t tcp_pkg_size;
    uint32_t tcp_pkg_size_bytes;
    uint32_t ram_size;
    uint32_t ram_size_bytes;
} RamInitConfig;

//  RAM-Config
typedef struct {
    int32_t* ram;
    volatile uint32_t* pos;
    uint32_t base_addr;
    RamInitConfig param;
} RamConfig;

// struct for TCP-Command
typedef struct {
    int id;
    double val;
    int ch;
} TcpCmd;

// ADC-Config:
typedef struct {
    int sample_rate_divider;  // relation between sys-clock and adc-sample-rate
    int burst_size;           // no. of ADC-samples in one burst
    int adc_mode;             // either RAM-BLOCK-MODE or RAM-CONTINOUS-MODE
    int start_delay;          // delay ADC-Sampling towards external first-sample-trigger by x triggers
    int trigger_mode;         // trigger-mode (free=0,external_trig=1)
    int under_sampling;       // enable/disable under_sampling for SR_ADC <= SR_DAC
    int offset_val_port_1;    // contains the calibration values for the ADC port 1
    int offset_val_port_2;    // contains the calibration values for the ADC port 2
    int gain_val_port_1;      // contains the calibration values for the ADC (port 1)
    int gain_val_port_2;      // contains the calibration values for the ADC (port 2)
                              // ctrl register bits:
    int debug_port_en;        // enable data output on debug port
    int gain_offset_calib_en; // enable gain and offset calibration
} AdcConfig;

// Trigger-Config:
typedef struct {
    int trigger_inc;
    int trigger_ref_max_value;
    int trigger_ref_min_value;
    int length;
    int start_delay;
} TriggerConfig;

// DAC-Config
typedef struct {
    int dev_id;
    int mode;
    float reset_voltage;
    int used_ports;
    int mirror;
} DacConfig;

// BRAM-DAC-Config
typedef struct {
    int port_id;           // which DAC-BRAM-Controller-Port is connected
    DacConfig dacConfig;   // which DAC is connected to this module?
    int dac_port_id;       // which DAC-Port is connected to this DAC-BRAM-Ctrl
    int no_steps;          // amount of samples in LUT
    int dwell_time_delay;  // delay in n-clocks between two samples
    int no_sweeps;         // amount of repetitions on output
    int rep_delay;         // delay in n-clocks between two sweeps
    int enable_handshake;  // enable handshaking for axi-stream transfer (mandatory for AD5754)
} BramDacConfig;

// Dummy Data Generator Config
typedef struct {
    uint32_t mode;          // Mode of operation
    uint32_t no_sample;     // amount of samples to be sent
    uint32_t sample_delay;  // delay between data_valid signals in clock cycles
    uint32_t max_cnt;       // maximum value of data sent
} DummyDataGenConfig;

// LIA-Mixer Config
typedef struct {
    int channel;        // which channel is used
    int debug_mode;     // enable or disable debug mode
} LiaMixerConfig;

// LIA IIR Filter Config
typedef struct {
    uint32_t num_sos;     // number of second order sections, 5 coeff per SOS
    uint32_t coeffs[30];  // coefficients for the IIR filter, max 6 SOS
} LiaIIRConfig;


// Struct to define a LUT-Value, for adjustment of single values inside LUT
typedef struct {
    int port_id;        // which DAC-BRAM-Ctrl-Port
    int dac_dev_id;     // which dac-device for
    int dac_port_id;    // which DAC-Port is used
    float voltage;      // voltage value of LUT at index
    int index;          // index for voltage value in LUT
} LutValue;

// BRAM data struct which holds a maximum of 16KB of 32 bit samples
typedef struct {
    uint32_t size;
    uint32_t data[16384];
} BramConfig;

// struct for calibration GO-Values (ADC and DAC)
typedef struct {
    float gain_ch0;
    float offset_ch0;
    float gain_ch1;
    float offset_ch1;
} GOValues;

//create Struct to define divide parameter and sync_enable_mode
typedef struct {
    int divider_param;
    int sync_mode_en;
} ClockDividerConfig;

// struct to configure SPI interface 
typedef struct {
    int mode;
    int spi_speed;
}SpiConfig;

#endif
