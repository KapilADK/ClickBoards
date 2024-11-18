#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc20click.h"
#include <time.h>
#include <unistd.h>

#define ADC20_MAX_VALUE_16BIT 65535
#define ADC20_MAX_VALUE_12BIT 4095


void write_register(int spi_fd, uint8_t regAddr, uint8_t regCfg) {

    // proper initialisation of the write buffer
    uint8_t tx_buf[3] = {0};
    tx_buf[0] = ADC20_CMD_REG_WRITE;
    tx_buf[1] =  regAddr;
    tx_buf[2] = regCfg;

    // declare the spi write transfer struct and initalise all the members to 0
    struct spi_ioc_transfer wr_reg;
    memset(&wr_reg, 0, sizeof(wr_reg));

    // initialise the members 
    wr_reg.tx_buf = (unsigned long)tx_buf;
    wr_reg.len = sizeof(tx_buf);

    // execute the transfer 
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &wr_reg) < 0) {
        printf("Writing to register 0x%02X failed: %s\n", regAddr, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void read_register (int spi_fd, uint8_t regAddr) {

    //prepare buffer to send read command with register address and dummy data
    uint8_t tx_buf[3] = {0};
    tx_buf[0] = ADC20_CMD_REG_READ;
    tx_buf[1] = regAddr;
    tx_buf[2] = DUMMY;

    // buffe to receive register data 
    uint8_t rx_buf = 0;

    // declare the spi read transfer and initialise all the members to 0
    struct spi_ioc_transfer rd_reg[2];
    memset(&rd_reg, 0, sizeof(rd_reg));

    // first transfer (send read register command)
    rd_reg[0].tx_buf = (unsigned long)tx_buf;
    rd_reg[0].len = sizeof(tx_buf);
    rd_reg[0].cs_change = 1;

    // second transfer (read back register value) 
    rd_reg[1].rx_buf = (unsigned long)&rx_buf;
    rd_reg[1].len = sizeof(rx_buf);

    // execute both transfers one after another
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), &rd_reg) < 0) {
        printf("Reading value from 0x%02X register failed: %s\n", regAddr, strerror(errno));
        exit(EXIT_FAILURE);
    }    

    printf("Value stored in 0x%02X register is 0x%02X\n", tx_buf[1], rx_buf);

}

void init_adc20(int spi_fd, int mode) {

    // donot append channel id at the end of output
    write_register(spi_fd, ADC20_REG_DATA_CFG, ADC20_CH_ID_APPEND);

    //initialise all channels as analog input pins
    write_register(spi_fd, ADC20_REG_PIN_CFG, ADC20_ALL_CHANNELS_AIN);

    //set sampling rate to 1MHz
    write_register(spi_fd, ADC20_REG_OPMODE_CFG, ADC20_1MHZ_SR_CFG);

    //config mode
    if (mode == MANUAL_MODE) {
        write_register( spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_MANUAL_MODE);
    }
}

uint16_t read_adc20_data(int spi_fd, int average) {

    uint8_t data_buf[ 3 ] = { 0 };
    uint16_t raw_data;
    uint8_t ch_id;

    struct spi_ioc_transfer rd_dummy;
    memset(&rd_dummy, 0, sizeof(rd_dummy));

    // first transfer
    rd_dummy.tx_buf = (unsigned long)data_buf;
    rd_dummy.rx_buf = (unsigned long)NULL;
    rd_dummy.len = sizeof(data_buf);

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &rd_dummy) < 0) {
        printf("Reading/writing dummy data failed. %s\n", strerror(errno));
    }

    usleep(1);

    //second transfer 
    struct spi_ioc_transfer rd_data;
    memset(&rd_data, 0, sizeof(rd_data));

    rd_data.tx_buf = (unsigned long)data_buf;
    rd_data.rx_buf = (unsigned long)data_buf;
    rd_data.len = sizeof(data_buf);

    // execute the transfers
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &rd_data) < 0) {
        printf("Reading data from ADC failed. %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (average == 0) {
        raw_data = (uint16_t)(data_buf[0] << 4) | (data_buf[1] >> 4);
        ch_id = data_buf[1] & 0x0F; 
    } else {
       raw_data = (uint16_t)data_buf[0] << 8 | data_buf[1];
        ch_id = data_buf[2] >> 4;
    }

    printf("Raw data read from channel %d of ADC 0x%04X\n",ch_id, raw_data);

    return raw_data;
}

int convert_adc20raw16_to_mV(uint16_t raw_data, int average) {
    int voltage_mV;
    double scaling_factor;

    // Use 12-bit or 16-bit scaling factor based on average parameter
    if (average == 0) {
        scaling_factor = ADC20_MAX_VALUE_12BIT;
    } else {
        scaling_factor = ADC20_MAX_VALUE_16BIT;
    }

    // Calculate voltage in mV
    voltage_mV = (int)(((double)raw_data / scaling_factor) * ADC20_VOLTAGE_RANGE * 1000);
    return voltage_mV;
}

int get_voltage_adc20(int spi_fd, int ch, int average) {

    int voltage_mV = 0;

    // check if valid channel is given
    if (ch < 0 || ch > 7) {
        printf("**Invalid channel number: %d**\n", ch);
        exit(EXIT_FAILURE);
    }

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        exit(EXIT_FAILURE);
    }

    // configure the oversampling ratio to average the data
    write_register(spi_fd, ADC20_REG_OSR_CFG, (uint8_t)average);

    // select channel to sample data in CHANNEL_SEL register
    write_register(spi_fd, ADC20_REG_CHANNEL_SEL, (uint8_t)ch);


    uint16_t raw_data = read_adc20_data(spi_fd, average);

    voltage_mV = convert_adc20raw16_to_mV(raw_data, average);
    return voltage_mV;
}

int *sample_sequence_mode_adc20(int spi_fd, int stop_ch, int average, int size, bool verbose) {

    int *voltages_mV =(int*) malloc(size * sizeof(int));

    // check if the stop channel is valid
    if (stop_ch < 1 || stop_ch > 7) {
        printf("**Invalid stop channel: %d**\n", stop_ch);
        exit(EXIT_FAILURE);
    }

    uint8_t regCfg =  (1 << (stop_ch + 1)) - 1;

    // configure the channels upto stop_ch for auto sequencing mode
    write_register(spi_fd, ADC20_REG_AUTO_SEQ_CH_SEL, regCfg);

    // start auto sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_SEQUENCE_MODE);

    //read voltages in a loop
    uint16_t raw_data;
    for (int i = 0; i <= stop_ch; i++) {
        raw_data = read_adc20_data(spi_fd, average);

        voltages_mV[i] = convert_adc20raw16_to_mV(raw_data, average);
        if (verbose) {
            printf("Voltage measured at channel %d: %dmV\n", i, voltages_mV[i]);
        }

    }
    // stop sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_STOP_SEQUENCE_MODE);
    return voltages_mV;
}
