#ifndef ADC20CLICK_H
#define ADC20CLICK_H

#include <stdint.h>

// command to write register
#define ADC20_CMD_REG_WRITE  0x08

// command to read register
#define ADC20_CMD_REG_READ   0x10

// dummy data to send when reading register
#define DUMMY 0x00

// registers list of ADC20Click
#define ADC20_REG_SYSTEM_STATUS             0x00
#define ADC20_REG_GENERAL_CFG               0x01
#define ADC20_REG_DATA_CFG                  0x02
#define ADC20_REG_OSR_CFG                   0x03
#define ADC20_REG_OPMODE_CFG                0x04
#define ADC20_REG_PIN_CFG                   0x05
#define ADC20_REG_GPIO_CFG                  0x07
#define ADC20_REG_GPO_DRIVE_CFG             0x09
#define ADC20_REG_GPO_VALUE                 0x0B
#define ADC20_REG_GPI_VALUE                 0x0D
#define ADC20_REG_SEQUENCE_CFG              0x10
#define ADC20_REG_CHANNEL_SEL               0x11
#define ADC20_REG_AUTO_SEQ_CH_SEL           0x12

// config to set sampling rate to 1MHz
#define ADC20_1MHZ_SR_CFG                   0x00

// config to set all channels as analog inputs
#define ADC20_ALL_CHANNELS_AIN              0x00

// debug command
#define ADC20_SEND_DEBUG_CMD                0x80

// config to select manual mode
#define ADC20_MANUAL_MODE                   0x00

// config for not appending channel identifier in output_format
#define ADC20_CH_ID_NOT_APPEND              0x00

//config for appending channel identifier in output format
#define ADC20_CH_ID_APPEND                  0x10

#define ADC20_MAX_VALUE                     0x0FFF

#define ADC20_VOLTAGE_RANGE                 3.3f

// config to start sequence mode
#define ADC20_SEQUENCE_MODE                 0x11

// config to stop sequence mode
#define ADC20_STOP_SEQUENCE_MODE            0x00

void write_register(int spi_fd, uint8_t regAddr, uint8_t regCfg);
void read_register (int spi_fd, uint8_t regAddr);
void init_adc20(int spi_fd);
uint16_t read_adc20_data(int spi_fd);
int convert_adc20raw16_to_mV(uint16_t raw_data, int average);
int get_voltage_adc20(int spi_fd, int ch, int average);
int *sample_sequence_mode_adc20(int spi_fd, int stop_ch, int average, int size, bool verbose);
void adc20_debug_communication(int spi_fd);

#endif
