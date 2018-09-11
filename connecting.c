#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 5000
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1240

int main() // (int argc, char* argv[])
{
    struct sockaddr_in server_addr;
    int server_socket;
    int opt = 1;

    bzero(&server_addr, sizeof(server_addr)); // 置字节字符串前n个字节为0，包括'\0'
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY); // 转小端,INADDR_ANY就是指定地址为0.0.0.0的地址
    server_addr.sin_port = htons(SERVER_PORT);  

    // 创建一个Socket
    server_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }


    // bind a socket
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port: %d Failed!\n", SERVER_PORT);
        exit(1);
    }

    // 监听Socket
    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }      

    while(1)
    {

        struct sockaddr_in client_addr;
        int client_socket;
	    socklen_t length;
		int ret = 0;
		int ret1 = 0；
		int read_fd;
        char Buffer[BUFFER_SIZE];
        char sendbuffer[BUFFER_SIZE];
		char data1[BUFFER_SIZE]={",0,0,0,0"};
		char data[BUFFER_SIZE];
		char stat[8]={"#"};
		memset(data,0,sizeof(data));
        // 连接客户端Socket
        length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if (client_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }

        // 从客户端接收数据
        while(1)
        {
            bzero(Buffer, BUFFER_SIZE);
            length = recv(client_socket, Buffer, BUFFER_SIZE, 0);
            system("lxterminal -e ./I2C");
            if (strcmp(Buffer,"all")!=0)
            {
                printf("Unknown Instruct!");
                break;
            }           

           else 
            {
                read_fd = ("./fifo",O_RDONLY);
                ret=read(read_fd,sendbuffer,BUFFER_SIZE);	
                close(reead_fd);	
                strcat(data,stat);
                strcat(data,sendbuffer);
                strcat(data,data1);
				data[strlen(data)]=0x0a;
                ret1=send(client_socket,data,(int)strlen(data),0);
                memset(data,0,sizeof(data));				
            }

          
	    }
         close(client_socket);
    }

    close(server_socket);
    return 0;
}
