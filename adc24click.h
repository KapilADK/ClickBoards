#ifndef ADC24CLICK_H
#define ADC24CLICK_H

#define DUMMY_HIGH 0xFFFF
#define DUMMY_LOW  0x0000


/*
*This config is used to insert the channel bits in the right order to read input from a selected channel. This config has:
    -11th bit set to 1: loads the remaining bits to the control register
    -10th bit set to 0:  sequence function not used
    -9th to 6th bit set to 0: address bits
    -5th and 4th bit set to 1: normal power mode
    -3rd bit set to 0: sequence function not used
    -2nd bit set to 1: the DOUT line is weakly driven to the ADD3 channel address bit of the ensuing conversion
    -1st bit set to 1: the analog input range extends from 0 V to REFIN
    -0th bit set to 0: the output coding for the part is twos complement
*/
#define ADC24_MANUAL_CONFG 0x0836

/**
 * This config is used to insert the channel bits upto which the sequence mode should sample the data. 
 * All the bits except 10th and 3rd are same as in ADC_24_MANUAL_CONFG. The 10th and 3rd bits are set to 1 to start the ADC in sequence mode.
 */
#define ADC24_SEQUENCE_CONFG 0x0C3E

#define ADC24_ADC_RESOLUTION 0x0FFF
#define ADC24_CTRL_REG_RESOLUTION 0x0FFF
#define ADC24_VREF_2V5 2.5f
#define ADC24_HIGHEST_VAL_TWOS_COMP 2048.0f

#define MAX_CHANNELS 15



void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg);
void init_adc24(int spi_fd);
void read_adc24_data(int spi_fd, uint16_t *data);
int convert_data_to_mV(uint16_t data);
int get_voltage_adc24(int spi_fd, int ch);
void sample_sequence_adc24(int spi_fd,  int stop_ch, int channels, int *voltages);


#endif