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

int AVERAGE = 0;

void write_register(int spi_fd, uint8_t regAddr, uint8_t regCfg) {

    // proper initialisation of the write buffer
    uint8_t wr_reg_buf[3] = {0};
    wr_reg_buf[0] = ADC20_CMD_REG_WRITE;
    wr_reg_buf[1] = regAddr;
    wr_reg_buf[2] = regCfg;

    // declare the spi write transfer struct and initalise all the members to 0
    struct spi_ioc_transfer wr_reg;
    memset(&wr_reg, 0, sizeof(wr_reg));

    // initialise the members 
    wr_reg.tx_buf = (unsigned long)wr_reg_buf;
    wr_reg.len = sizeof(wr_reg_buf);

    // execute the transfer 
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &wr_reg) < 0) {
        printf("Writing to register 0x%02X failed: %s\n", regAddr, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void read_register (int spi_fd, uint8_t regAddr) {

    //prepare buffer to send read command with register address and dummy data
    uint8_t rd_reg_buf[3] = {0};
    rd_reg_buf[0] = ADC20_CMD_REG_READ;
    rd_reg_buf[1] = regAddr;
    rd_reg_buf[2] = DUMMY;

    // buffe to receive register data 
    uint8_t reg_data = 0;

    // declare the spi read transfer and initialise all the members to 0
    struct spi_ioc_transfer rd_reg[2];
    memset(&rd_reg, 0, sizeof(rd_reg));

    // first transfer (send read register command)
    rd_reg[0].tx_buf = (unsigned long)rd_reg_buf;
    rd_reg[0].len = sizeof(rd_reg_buf);
    rd_reg[0].cs_change = 1;

    // second transfer (read back register value) 
    rd_reg[1].tx_buf = (unsigned long)NULL;
    rd_reg[1].rx_buf = (unsigned long)&reg_data;
    rd_reg[1].len = sizeof(reg_data);

    // execute both transfers one after another
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), &rd_reg) < 0) {
        printf("Reading value from 0x%02X register failed: %s\n", regAddr, strerror(errno));
        exit(EXIT_FAILURE);
    }    

    printf("Value stored in 0x%02X register is 0x%02X\n", rd_reg_buf[1], reg_data);

}

void init_adc20(int spi_fd, int average, int mode) {

    // append channel id at the end of output
    write_register(spi_fd, ADC20_REG_DATA_CFG, ADC20_CH_ID_APPEND);

    //initialise all channels as analog input pins
    write_register(spi_fd, ADC20_REG_PIN_CFG, ADC20_ALL_CHANNELS_AIN);

    //set sampling rate to 1MHz
    write_register(spi_fd, ADC20_REG_OPMODE_CFG, ADC20_1MHZ_SR_CFG);

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        exit(EXIT_FAILURE);
    } else {
        AVERAGE = average;
    }

    // configure the oversampling ratio to average the data
    write_register(spi_fd, ADC20_REG_OSR_CFG, (uint8_t)average);

    //config mode
    // set the mode to manual mode if the mode is MANUAL_MODE
    if (mode == MANUAL_MODE){
        write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, MANUAL_MODE);

    } else {
        //select the channels to sample data in AUTO_SEQ_CH_SEL register
        write_register(spi_fd, ADC20_REG_AUTO_SEQ_CH_SEL, mode);
        //start sequence mode
        write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_SEQUENCE_MODE);
        //read a dummy data to start the sequence mode
        read(spi_fd, NULL, 3);
    }
}

uint16_t manual_read_adc20_data(int spi_fd, int average) {

    uint8_t data_buf[ 3 ] = { 0 };
    uint16_t raw_data;
    uint8_t ch_id;

    struct spi_ioc_transfer rd_dummy;
    memset(&rd_dummy, 0, sizeof(rd_dummy));

    // first transfer
    rd_dummy.tx_buf = (unsigned long)data_buf;
    rd_dummy.rx_buf = (unsigned long)data_buf;
    rd_dummy.len = sizeof(data_buf);

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &rd_dummy) < 0) {
        printf("Reading/writing dummy data failed. %s\n", strerror(errno));
    }

    //second transfer 
    struct spi_ioc_transfer rd_data;
    memset(&rd_data, 0, sizeof(rd_data));

    rd_data.tx_buf = (unsigned long)NULL;
    rd_data.rx_buf = (unsigned long)data_buf;
    rd_data.len = sizeof(data_buf);

    // execute the transfers
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &rd_data) < 0) {
        printf("Reading data from ADC failed. %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!average) {
        raw_data = (((uint16_t)(data_buf[0])) << 4)| (data_buf[1] >> 4);
        ch_id = data_buf[1] & 0x0F; 
    } else {
       raw_data = ((uint16_t)data_buf[0] << 8) | data_buf[1];
        ch_id = data_buf[2] >> 4;
    }
    printf("Raw data read from channel %d of ADC 0x%04X\n",ch_id, raw_data);

    return raw_data;
}

int convert_adc20raw16_to_mV(uint16_t raw_data, int average) {
    int voltage_mV;
    double scaling_factor;

    // Use 12-bit or 16-bit scaling factor based on average parameter
    if (!average) {
        scaling_factor = ADC20_MAX_VALUE_12BIT;
    } else {
        scaling_factor = ADC20_MAX_VALUE_16BIT;
    }

    // Calculate voltage in mV
    voltage_mV = (int)(((double)raw_data / scaling_factor) * ADC20_VOLTAGE_RANGE * 1000);
    return voltage_mV;
}

int get_voltage_adc20(int spi_fd, int ch) {

    int voltage_mV = 0;

    // check if valid channel is given
    if (ch < 0 || ch > 7) {
        printf("**Invalid channel number: %d**\n", ch);
        exit(EXIT_FAILURE);
    }

    // select channel to sample data in CHANNEL_SEL register
    write_register(spi_fd, ADC20_REG_CHANNEL_SEL, (uint8_t)ch);

    uint16_t raw_data = manual_read_adc20_data(spi_fd, AVERAGE);

    voltage_mV = convert_adc20raw16_to_mV(raw_data, AVERAGE);
    return voltage_mV;
}

uint16_t auto_read_adc20_data(int spi_fd, int average) {

    uint8_t data_buf[ 3 ] = { 0 };
    uint16_t raw_data;
    uint8_t ch_id;

    // read data 
    struct spi_ioc_transfer rd_data;
    memset(&rd_data, 0, sizeof(rd_data));

    rd_data.tx_buf = (unsigned long)NULL;
    rd_data.rx_buf = (unsigned long)data_buf;
    rd_data.len = sizeof(data_buf);

    // execute the transfer
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &rd_data) < 0) {
        printf("Reading data from ADC failed. %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!average) {
        raw_data = (((uint16_t)(data_buf[0])) << 4)| (data_buf[1] >> 4);
        ch_id = data_buf[1] & 0x0F; 
    } else {
       raw_data = ((uint16_t)data_buf[0] << 8) | data_buf[1];
        ch_id = data_buf[2] >> 4;
    }
    printf("Raw data read from channel %d of ADC 0x%04X\n",ch_id, raw_data);

    return raw_data;
}

void sample_sequence_mode_adc20(int spi_fd, int size, int *ptr) {

    uint16_t raw_data = 0;
    int voltage_mV = 0;
    // read data from all the channels in sequence mode until the given samples are read
    for (int i = 0; i < size; i++) {
        raw_data = auto_read_adc20_data(spi_fd, AVERAGE);
        voltage_mV = convert_adc20raw16_to_mV(raw_data, AVERAGE);
        ptr[i] = voltage_mV;
    }
    // stop sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_STOP_SEQUENCE_MODE);

}
