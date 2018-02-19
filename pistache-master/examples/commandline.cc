#include <iostream>
#include <stdio.h>
#include <limits>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
using namespace std;
#define MMSB_32_8(x)    (x&0xFF000000)>>24
#define MLSB_32_8(x)    0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)    0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)    0xFF&((x&0x000000FF))
#define I2C_CS 5
#define addr_I2C 24
unsigned short port = 62000;
int write32bCPU(unsigned int cs,unsigned int address,unsigned long int data){
    //cout<<"---------------write32bCPU"<<endl;
    struct sockaddr_in serverAddress,
        clientAddress;
    unsigned short len=0;
    unsigned char msgBuf[20],RxBuffer[105]={0};
    struct timeval tv;
    tv.tv_sec = 0;  /* 0.5 Secs Timeout */
    tv.tv_usec = 300000;
    int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    if (socketHandle<0)
    {
      printf("could not make socket\n");
      return 0;
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.1.20");
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddress.sin_port = htons(port);

    if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress))<0){
        printf("bind failed\n");
    }
    len = 0;
    msgBuf[0] = 0x4D;
    msgBuf[1] = 0x56;
    msgBuf[2] = 0x44;
    msgBuf[3] = 0x01;
    msgBuf[4] = (0xFF00&cs)>>8;
    msgBuf[5] = (0x00FF&cs)>>0;
    msgBuf[6] = 0xF0;
    msgBuf[7] = address;
    msgBuf[8] = MMSB_32_8(data);//dont care
    msgBuf[9] = MLSB_32_8(data);//dont care
    msgBuf[10]= LMSB_32_8(data);//dont care
    msgBuf[11]= LLSB_32_8(data);//dont care
    len += 12;
    int x=0;
    unsigned int count;
    while (x == 0)
    {
        int n = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));

        int serv_addr_size = sizeof(clientAddress);
        count=recvfrom(socketHandle,RxBuffer,32, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);
        x=1;
    }
    close(socketHandle);
    return count;
}
unsigned long int read32bCPU(unsigned int cs,unsigned int address){
    unsigned long int status;
    struct sockaddr_in serverAddress,
            clientAddress;
    unsigned short len=0;
    unsigned char msgBuf[20],RxBuffer[105]={0};
    struct timeval tv;
    tv.tv_sec = 0;  /* 0.5 Secs Timeout */
    tv.tv_usec = 300000;
    int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    if (socketHandle<0)
    {
      printf("could not make socket\n");
      return 0;
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.1.20");
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddress.sin_port = htons(port);

    if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress))<0){
        printf("bind failed\n");
    }

    len = 0;
    msgBuf[0] = 0x4D;
    msgBuf[1] = 0x56;
    msgBuf[2] = 0x44;
    msgBuf[3] = 0x01;
    msgBuf[4] = (0xFF00&cs)>>8;
    msgBuf[5] = (0x00FF&cs)>>0;
    msgBuf[6] = 0x01;
    msgBuf[7] =address;
    msgBuf[8] = 0x00;//dont care
    msgBuf[9] = 0x00;//dont care
    msgBuf[10]= 0x00;//dont care
    msgBuf[11]= 0x00;//dont care
    len += 12;
    int x=0,count;
    while (x == 0)
    {
      int n = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
      int serv_addr_size = sizeof(clientAddress);
      count=recvfrom(socketHandle,RxBuffer,32, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);
      int i=0;
      /*for(i=0;i<sizeof(RxBuffer) / sizeof(RxBuffer[0]);i++)
      {
        printf("%d",RxBuffer[i]);
      }*/
      status = RxBuffer[8]<<24 | RxBuffer[9]<<16 | RxBuffer[10]<<8 | RxBuffer[11];
      // printf("Read data %ld\n",status);
      x=1;
    }
    close(socketHandle);
    return status;
}
int write32(unsigned char cCs, unsigned char addr, unsigned int *sData,int sDataLen){
    unsigned int value = read32bCPU(I2C_CS,8);
    unsigned int size =sDataLen + 2;
    unsigned int word0 = (value&0x00010000) | size;  // passage en mode write
    write32bCPU (I2C_CS,8,word0);
    usleep(1000);
    int index_out = 2;
    int i=0;
    unsigned long long data_out =((addr_I2C << 8) | addr);
    int len = sDataLen+1;
    for (i=0;i<len;i++) {
        int byte ;
        if(i == len-1)
            byte = cCs;
        else
            byte = sData[i];
        data_out =  ((data_out << 8) + byte);
          if ( index_out == 3 ) {
             write32bCPU (I2C_CS,4,data_out);
             usleep(1000);
             index_out = 0;
          } else {
             index_out = index_out+1;
          }

       }
        if (index_out == 1 ) {
         data_out = data_out<<24;
          write32bCPU(I2C_CS,4,data_out);
          usleep(1000);
        } else if (index_out == 2) {
          data_out = data_out<<16;
          write32bCPU(I2C_CS,4,data_out);
          usleep(1000);
        } else if (index_out == 3 ) {
          data_out = data_out<<8;
          write32bCPU(I2C_CS,4,data_out);
          usleep(1000);
        }
        word0 = (1<<18)|word0;
        write32bCPU(I2C_CS,8,word0);
        usleep(1000);
}

int read32bI2C(unsigned char cCs, unsigned char addr){
    unsigned int sData[0];
    //readI2C 0 addr_I2C I2C_CS cCs addr 4
    write32(cCs, addr,sData,sizeof(sData) / sizeof(sData[0]));
    usleep(1000);
    write32bCPU(I2C_CS,8,0<<16|4);
    usleep(1000);
    unsigned int word0 = (addr_I2C|1)<<24 | addr<<16;
    write32bCPU(I2C_CS,4,word0);
    usleep(1000);
    write32bCPU(I2C_CS,8,(1<<18|1<<17|0<<16|4));
    usleep(1000);
    return read32bCPU(I2C_CS,4);
}

int write32bI2C(unsigned int cs,unsigned int address,unsigned long int data){
    unsigned int sData[6];
    sData[0] = cs;
    sData[1] = (data&0xFF000000)>>24;
    sData[2] = (data&0x00FF0000)>>16;
    sData[3] = (data&0x0000FF00)>>8;
    sData[4] = (data&0x000000FF)>>0;
    write32(cs,address,sData,sizeof(sData) / sizeof(sData[0]));
}

unsigned long int read32bI2C(unsigned int cs,unsigned int address){
     unsigned int sData[0];
    //readI2C 0 addr_I2C I2C_CS cCs addr 4
    write32(cs, address,sData,sizeof(sData) / sizeof(sData[0]));
    write32bCPU(I2C_CS,8,0<<16|4);
    unsigned int word0 = (addr_I2C|1)<<24 | address<<16;
    write32bCPU(I2C_CS,4,word0);
    write32bCPU(I2C_CS,8,(1<<18|1<<17|0<<16|4));
    usleep(10000);
    return read32bCPU(I2C_CS,4);
}

int selectInterface(unsigned int uart,unsigned int i2c,unsigned int spi){
    int target =((spi&0x3)<<8) | ((uart&0x7)<<5) | ((i2c&0xF)<<1) | (0&0x1);
    if(write32bCPU(0,0,target) != -1)
        cout<<1<<endl;
    else
        cout<<0<<endl;
}
int main(int argc, char* argv[])
{
    unsigned int cs,address;
    unsigned long int data;
    string function_name;
    string function_list[] = {"writeCPU","writeI2C","readCPU","readI2C","selectInterface"};
    int function_id =-1;
    while(1){
        int valid_in = 1;
        do{
            cin >>function_name>>cs>>address>>data;
            if (cin.good())
            {
                valid_in = 1;
            }
            else
            {
                valid_in = -1;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(),'\n');
                cout << "Invalid input; please re-enter." << endl;
                cout<<"function_name cs address data"<<endl;
            }
        }while(valid_in == -1);


        for(int i = 0; i<sizeof(function_list)/sizeof(function_list[0]);i++){
            if(function_name == function_list[i]){
                function_id = i;
            }
        }
        int resp=0;
        switch(function_id){
            case 0:resp = write32bCPU(cs,address,data);
                if(resp != -1)
                    cout<<1<<endl;
                else
                    cout<<0<<endl;
            break;
            case 1:resp =write32bI2C(cs,address,data);
                if(resp != -1)
                    cout<<1<<endl;
                else
                    cout<<0<<endl;
            break;
            case 2:resp = read32bCPU(cs,address); 
                if(resp != -1)
                    cout<<resp<<endl;
                else
                    cout<<0<<endl;
            break;
            case 3:resp = read32bI2C(cs,address); 
                if(resp != -1)
                    cout<<resp<<endl;
                else
                    cout<<0<<endl;
            break;
            case 4:
                unsigned int uart,spi,i2c;
                uart = cs;i2c = address;spi = (unsigned int)data;
                selectInterface(uart,i2c,spi);
            break;
            
            default:cout << "Invalid function!"<< endl;
        }
        //cout << "Hello world!"<<function_name<< endl;
    }
    return 0;
}
