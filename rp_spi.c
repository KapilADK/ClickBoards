#include "rp_spi.h"
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <stdlib.h>
#include "rp_structs.h"

int test_spi_fd = -1;

int setup_spi(SpiConfig spiCfg) {

    // opening spi interface
    int spi_fd;
	int bits_per_word = 8;
	int bit_order = 0; //0 for MSB first 
    spi_fd = open("/dev/spidev1.0", O_RDWR | O_NOCTTY);
    if (spi_fd < 0) {
        printf("Error opening /dev/spidev1.0. Error: %s\n", strerror(errno));
        return -1;
    }

    //setting mode (CPHA, CPOL)
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spiCfg.mode) < 0) {
        printf("Error setting SPI mode. Error %s\n", strerror(errno));
        release_spi(spi_fd);
        return -1;
    }

    //setting spi bus speed
	if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiCfg.spi_speed) < 0) {
		printf("Error setting SPI MAX_SPEED_HZ. Error: %s\n", strerror(errno));
        release_spi(spi_fd);
		return -1;
	}

    // setting bit order (MSB first)
    if (ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &bit_order) < 0) {
        printf("Error setting SPI bit order. Error: %s\n", strerror(errno));
        release_spi(spi_fd);
        return -1;
    }

	// setting bits per word (8)
	if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) {
		printf("Error setting SPI bits per word. Error: %s\n", strerror(errno));
		release_spi(spi_fd);
		return -1;
	}

    return spi_fd;

}

int release_spi(int spi_fd) {

    //Release spi resources
    close(spi_fd);
    return 0;
}

int spi_test(void){

	/* Sample data */
	char *data = "REDPITAYA SPI TEST";

	/* Init the spi resources */
	if(init_spi() < 0){
		printf("Initialization of SPI failed. Error: %s\n", strerror(errno));
		return -1;
	}

	/* Write some sample data */
	if(write_spi(data, strlen(data)) < 0){
		printf("Write to SPI failed. Error: %s\n", strerror(errno));
		return -1;
	}

	/* Read flash ID and some sample loopback data */
	if(read_flash_id(test_spi_fd) < 0){
		printf("Error reading from SPI bus : %s\n", strerror(errno));
		return -1;
	}

	/* Release resources */
	if(release_spi(test_spi_fd) < 0){
		printf("Relase of SPI resources failed, Error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int init_spi(){

	/* MODES: mode |= SPI_LOOP; 
	 *        mode |= SPI_CPHA; 
	 *        mode |= SPI_CPOL; 
	 *		  mode |= SPI_LSB_FIRST; 
	 *        mode |= SPI_CS_HIGH; 
	 *        mode |= SPI_3WIRE; 
	 *        mode |= SPI_NO_CS; 
	 *        mode |= SPI_READY;
	 *
	 * multiple possibilities possible using | */
	uint8_t mode = SPI_MODE_3;

	/* Opening file stream */

    printf("Initializing SPI interface....\n");
	test_spi_fd = open("/dev/spidev1.0", O_RDWR | O_NOCTTY);
	if(test_spi_fd < 0){
		printf("Error opening spidev0.1. Error: %s\n", strerror(errno));
		return -1;
	}

	/* Setting mode (CPHA, CPOL) */
    printf("Setting SPI Mode..\n");
	if(ioctl(test_spi_fd, SPI_IOC_WR_MODE, &mode) < 0){
		printf("Error setting SPI MODE. Error: %s\n", strerror(errno));
		return -1;
	}

	// /* Setting SPI bus speed */
	// int spi_speed = 900000000;
    // printf("Setting SPI Bus Speed.......\n");

	// if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0){
	// 	printf("Error setting SPI MAX_SPEED_HZ. Error: %s\n", strerror(errno));
	// 	return -1;
	// }

    printf("Reading SPI Mode, Speed and Bit Order....\n");
    // Now you can read the settings back to confirm
    uint8_t read_mode;
    if (ioctl(test_spi_fd, SPI_IOC_RD_MODE, &read_mode) < 0) {
        perror("Failed to read SPI mode");
        close(test_spi_fd);
        return EXIT_FAILURE;
    }
    printf("SPI Mode: %d\n", read_mode);

    uint32_t read_speed;
    if (ioctl(test_spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &read_speed) < 0) {
        perror("Failed to read SPI speed");
        close(test_spi_fd);
        return EXIT_FAILURE;
    }
    printf("SPI Speed: %d Hz\n", read_speed);

    uint8_t read_bits;
    if (ioctl(test_spi_fd, SPI_IOC_RD_BITS_PER_WORD, &read_bits) < 0) {
        perror("Failed to read bits per word");
        close(test_spi_fd);
        return EXIT_FAILURE;
    }
    printf("SPI Bits per Word: %d\n", read_bits);

	return 0;
}


/* Read data from the SPI bus */
static int read_flash_id(int fd){

	int size = 2;

	/*struct spi_ioc_transfer {
          __u64           tx_buf;
          __u64           rx_buf;
  
          __u32           len;
          __u32           speed_hz;
  
          __u16           delay_usecs;
          __u8            bits_per_word;
          __u8            cs_change;
          __u32           pad;  
    }*/
    /* If the contents of 'struct spi_ioc_transfer' ever change
	 * incompatibly, then the ioctl number (currently 0) must change;
	 * ioctls with constant size fields get a bit more in the way of
	 * error checking than ones (like this) where that field varies.
	 *
	 * NOTE: struct layout is the same in 64bit and 32bit userspace.*/  
	struct spi_ioc_transfer xfer[size];
    
    unsigned char           buf0[1];
    unsigned char           buf1[3];
	int                     status;
  	
 	memset(xfer, 0, sizeof xfer);
 	
 	/* RDID command */
	buf0[0] = 0x9f;
	/* Some sample data */
	buf1[0] = 0x01;
	buf1[1] = 0x23;
	buf1[2] = 0x45;

	/* RDID buffer */
	xfer[0].tx_buf = (unsigned long)buf0;
	xfer[0].rx_buf = (unsigned long)buf0;
	xfer[0].len = 1;

	/* Sample loopback buffer */
	xfer[1].tx_buf = (unsigned long)buf1;
	xfer[1].rx_buf = (unsigned long)buf1;
	xfer[1].len = 3;

	/* ioctl function arguments
	 * arg[0] - file descriptor
	 * arg[1] - message number
	 * arg[2] - spi_ioc_transfer structure
	 */

     printf("Reading from SPI Receive Buffer..\n");
	status = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	if (status < 0) {
		perror("SPI_IOC_MESSAGE");
		return -1;
	}
	
	/* Print read buffer */
	for(int i = 0; i < 3; i++){
		printf("Buffer: %d\n", buf1[i]);
	}
	
	return 0;
}

/* Write data to the SPI bus */
static int write_spi(char *write_buffer, int size){

    printf("Data written to SPI Interface: %s\n", write_buffer);

	int write_spi = write(test_spi_fd, write_buffer, size);

	if(write_spi < 0){
		printf("Failed to write to SPI. Error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

