#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#define I2C_ADDR 0x23
int main(void)
{
    int fd;
    int write_fd;
    int ret;
    char buf[3];
    char val,value;
    char buffer[4];
    float flight;
    fd=open("/dev/i2c-1",O_RDWR);
    if(fd<0)
    {
        printf("打开文件错误:%s\r\n",strerror(errno)); return 1;
    }
    if(ioctl( fd,I2C_SLAVE,I2C_ADDR)<0 )
    {
        printf("ioctl 错误 : %s\r\n",strerror(errno));return 1;
    }
    val=0x01;
    if(write(fd,&val,1)<0)
    {
        printf("上电失败\r\n");
    }
    val=0x11;
    if(write(fd,&val,1)<0)
    {
        printf("开启高分辨率模式2\r\n");
    }
    usleep(200000);
    if(read(fd,&buf,3)){
        flight=(buf[0]*256+buf[1])*0.5/1.2;
        printf("光照度: %6.2flx\r\n",flight);
		write_fd=open("./fifo",O_WRONLY);
		sprintf(buffer,"%.3f",flight);
		ret=write(write_fd,buffer,10);
		close(write_fd);
    }
    else{
        printf("读取错误\r\n");
    }
}
