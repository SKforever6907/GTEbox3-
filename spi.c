#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>


#define DIG_T_START 0x88
#define DIG_P_START 0x8E
#define DIG_H_1 0xA1
#define DIG_H_2 0xE1
#define TEMP_START 0xFA
#define PRESS_START 0xF7
#define HUM_START 0xFD
#define CTRL_MEAS 0xF4
#define TEMP_PRESS_NORMAL_MODE 0xEE // 111 111 11
#define CTRL_HUM 0xF2
#define HUM_NORMAL_MODE 0x7 //  111

char buf[10];
char buf2[10];

struct spi_ioc_transfer xfer[2];
int32_t t_fine;
int spi_init(char filename[40]) {
        int file;
        __u8  lsb = 0;
        __u8 mode = 0;
        __u8 bits = 8;
        __u32 speed = 10000000;

        if ((file = open("/dev/spidev0.0",O_RDWR)) < 0) {
                printf("Failed to open the bus.");
                exit(1);
        }

        if (ioctl(file, SPI_IOC_WR_MODE, &mode)<0) {
                perror("can't set spi mode");
                return;
        }        if (ioctl(file, SPI_IOC_RD_MODE, &mode) < 0) {
                perror("SPI rd_mode");
                return;
        }
        if (ioctl(file, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
                perror("SPI rd_lsb_fist");
                return;
        }
        if (ioctl(file, SPI_IOC_WR_BITS_PER_WORD, &bits)<0) {
                perror("can't set bits per word");
                return;
        }
        if (ioctl(file, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
                perror("SPI bits_per_word");
                return;
        }
        if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0) {
                perror("can't set max speed hz");
                return;
        }
        if (ioctl(file, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
                perror("SPI max_speed_hz");
                return;
        }
        return file;
}

char * spi_read(int addr,int nbytes,int file) {
        int status;

        memset(buf, 0, sizeof buf);
        memset(buf2, 0, sizeof buf2);
        buf[0] = addr | 128;
        xfer[0].tx_buf = (unsigned long)buf;
        xfer[0].len = 1; 
        xfer[1].rx_buf = (unsigned long) buf2;
        xfer[1].len = nbytes; 
        status = ioctl(file, SPI_IOC_MESSAGE(2), xfer);
        if (status < 0) {
                perror("SPI_IOC_MESSAGE");
                return;
        }
        return buf2;
}

void spi_write(int addr, char value, int file) {
        int status;

        memset(buf, 0, sizeof buf);
        buf[0] = addr & 127;
        buf[1] = value;
        xfer[0].tx_buf = (unsigned long)buf;
        xfer[0].len = 2;
        status = ioctl(file, SPI_IOC_MESSAGE(1), xfer);
        if (status < 0) {
                perror("SPI_IOC_MESSAGE");
        }
}

float BME280_compensate_T_double(uint32_t adc_T, unsigned short dig_T1, short dig_T2, short dig_T3)      //说明书提供的公式
{
        uint32_t var1, var2;
        float T;
        var1 = (((double)adc_T)/16384.0-((double)dig_T1)/1024.0)*((double)dig_T2);
        var2 = ((((double)adc_T)/131072.0-((double)dig_T1)/8192.0)*(((double)adc_T)/131072.0-((double)dig_T1)/8192.0))*((double)dig_T2);
		t_fine = var1 + var2;
        T = (var1+var2)/5120.0;
        return T;
}

float BME280_compensate_P_double(uint32_t adc_P,unsigned short dig_P1,short dig_P2,short dig_P3,short dig_P4,short dig_P5,short dig_P6,short dig_P7,short dig_P8,short dig_P9)
{ 
	    long var1, var2, p;
	    var1 = ((double)t_fine/2.0)-64000.0;
        var2 = var1*var1*((double)dig_P6)/32768.0;
        var2 = var2 +var1*((double)dig_P5)*2.0;
        var2 = (var2/4.0)+(((double)dig_P4)*65536.0);
        var1 = (((double)dig_P3)*var1*var1/524288.0+((double)dig_P2)*var1)/524288.0;
        var1 = (1.0+var1/32768.0)*((double)dig_P1);
        p = 1048576.0-(double)adc_P;
        p = (p-(var2/4096.0))*6250.0/var1;
        var1 = ((double)dig_P9)*p*p/2147483648.0;
        var2 = p*((double)dig_P8)/32768.0;
        p = p+(var1+var2+((double)dig_P7))/16.0;

    return p;
}
// Returns humidity in %rH as as double. Output value of “46.332” represents 46.332 %rH
float bme280_compensate_H_double(uint32_t adc_H,unsigned char dig_H1,short dig_H2,unsigned char dig_H3,short dig_H4,short dig_H5,signed char dig_H6)
{
	double var_H;
	var_H =(((double)t_fine)-76800.0);
	var_H = (adc_H-(((double)dig_H4) * 64.0 + ((double)dig_H5) / 16384.0 * var_H)) *
		(((double)dig_H2) / 65536.0 * (1.0 + ((double)dig_H6) / 67108864.0 * var_H *
		(1.0 + ((double)dig_H3) / 67108864.0 * var_H)));
	var_H = var_H * (1.0-((double)dig_H1) * var_H / 524288.0);
	
	return var_H;
}
int main() {
            int write_fd;
	    int ret;
            float temp;
            float press;
            float hum;
            char dig_t_buff[6];
	    char dig_p_buff[18];
	    char dig_h1_buff[1];
	    char dig_h7_buff[7];
	    char tmp_buff[3];
            char press_buff[3];
	    char hum_buff[2];
	    char pase[2]={","};
	    char temp1[50];
	    char press1[50];
	    char hum1[50];
	    char data[300];
	    memset(data,0,sizeof(data));		

        int file=spi_init("/dev/spidev1.0");
	spi_write(CTRL_HUM, HUM_NORMAL_MODE, file);
        spi_write(CTRL_MEAS,TEMP_PRESS_NORMAL_MODE,file);

       
        memcpy(dig_t_buff, spi_read(DIG_T_START, 6, file), 6);
        memcpy(tmp_buff,  spi_read(TEMP_START, 3, file), 3);
		
		memcpy(dig_p_buff, spi_read(DIG_P_START, 18, file), 18);
                memcpy(press_buff, spi_read(PRESS_START, 3, file), 3);
		
		memcpy(dig_h1_buff, spi_read(DIG_H_1,1, file), 1);
		memcpy(dig_h7_buff, spi_read(DIG_H_2,7, file), 7);
	        memcpy(hum_buff, spi_read(HUM_START, 2, file), 2);
		

      
                int adc_T = ((tmp_buff[0]<<16)|(tmp_buff[1]<<8)|(tmp_buff[2]))>>4;
		int adc_P = ((press_buff[0] << 16) | (press_buff[1] << 8) | (press_buff[2])) >> 4;
		int adc_H = ((hum_buff[0]<<8)|(hum_buff[1]));


                unsigned short dig_T1 = (dig_t_buff[1]<<8)|(dig_t_buff[0]);
                short dig_T2 = (dig_t_buff[3]<<8)|(dig_t_buff[2]);
                short dig_T3 = (dig_t_buff[5]<<8)|(dig_t_buff[4]);

                unsigned short dig_P1 = (dig_p_buff[1] << 8) | (dig_p_buff[0]);
		short dig_P2 = (dig_p_buff[3] << 8) | (dig_p_buff[2]);
		short dig_P3 = (dig_p_buff[5] << 8) | (dig_p_buff[4]);
		short dig_P4 = (dig_p_buff[7] << 8) | (dig_p_buff[6]);
		short dig_P5 = (dig_p_buff[9] << 8) | (dig_p_buff[8]);
		short dig_P6 = (dig_p_buff[11] << 8) | (dig_p_buff[10]);
		short dig_P7 = (dig_p_buff[13] << 8) | (dig_p_buff[12]);
		short dig_P8 = (dig_p_buff[15] << 8) | (dig_p_buff[14]);
		short dig_P9 = (dig_p_buff[17] << 8) | (dig_p_buff[16]);

		unsigned char dig_H1 = dig_h1_buff[0];
		short dig_H2 = (dig_h7_buff[1] << 8) | (dig_h7_buff[0]);
		unsigned char dig_H3 = dig_h7_buff[2];
		short dig_H4 = ((dig_h7_buff[3] <<4) | (dig_h7_buff[4] & 0xf));
		short dig_H5 = ((dig_h7_buff[5] <<4) | (dig_h7_buff[4] & 0xf0));
		signed char dig_H6 = dig_h7_buff[6];

		temp = BME280_compensate_T_double(adc_T, dig_T1, dig_T2, dig_T3);
		press =  BME280_compensate_P_double(adc_P, dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);
		hum = bme280_compensate_H_double(adc_H,dig_H1,dig_H2,dig_H3,dig_H4,dig_H5,dig_H6);
                printf("Temperature is : %6.2f \n",temp);
		printf("Pressature is : %6.2f \n",press);
		printf("Humidity is :%6.2f \n",hum);
		strcat(data,pase);
		sprintf(temp1,"%.2f",temp);
		strcat(data,temp1);
		strcat(data,pase);
		sprintf(hum1,"%.2f",hum);
		strcat(data,hum1);
		strcat(data,pase);
	        sprintf(press1,"%.2f",press);
		strcat(data,press1);
		strcat(data,pase);
		printf("%s",data);
		write_fd=open("./swap",O_WRONLY);
		ret=write(write_fd,data,100);
		close(write_fd);
		memset(data,0,sizeof(data));
                return 0;
}
