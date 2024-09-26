#ifndef ADC24CLICK_H
#define ADC24CLICK_H

#include <stdint.h>

#define DUMMY_HIGH 0xFFFF   // required to pull the DIN line high when powering up AD7490
#define DUMMY_LOW  0x0000    // required when reading the data out the DOUT line

// default config in manual mode without setting channel identifier bits
/*
*This config is used to insert the channel bits in the right order to read input from a selected channel. This config has:
    -11th bit set to 1: loads the remaining bits to the control register
    -10th bit set to 0:  sequence function not used
    -9th to 6th bit set to 0: address bits
    -5th and 4th bit set to 1: normal power mode
    -3rd bit set to 0: sequence function not used
    -2nd bit set to 1: the DOUT line is weakly driven to the ADD3 channel address bit of the ensuing conversion
    -1st bit set to 0: the analog input range extends from 0 V to 2*REFIN (+2.5V to -2.5V)
    -0th bit set to 0: the output coding for the part is twos complement
*/
#define ADC24_DEF_MANUAL_MODE   0x0834

#define ADC24_MAX_VALUE         0x0800

#define ADC24_VOLTAGE_RANGE     2.5f

// config to start sequence mode
#define ADC24_DEF_SEQUENCE_MODE 0x0C3C

void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg);
void init_adc24(int spi_fd);
int16_t read_adc24_data(int spi_fd);
int convert_raw16_to_mV(int16_t raw_data);
int get_voltage_adc24(int spi_fd, int ch);
void sequence_mode_adc24(int spi_fd, int stop_ch, int *voltages_mV, bool verbose);


#endif
