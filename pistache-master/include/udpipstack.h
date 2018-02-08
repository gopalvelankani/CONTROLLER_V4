#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#define SOCKET_INVALID(s) (s<0)
#define SOCKET_TYPE int
#define STX1 0x4D
#define STX2 0x56
#define STX3 0x44
#define UDPNUMBER 0x01
#define UNUSED 0x01
#define CHIPSELECT 0x02
#define READMODE 0x01
#define WRITEMODE 0xF0
#define CMD_GET_UDPCHANNELS 0x34
#define UDP_PORT_SOURCE 0x28
#define UDP_CHANNEL_NUMBER 0x30
#define IGMP_CHANNEL_NUMBER 0x20
#define CMD_GET_VERSION_OF_CORE 0x08




#define CMD_SET_DESTINATION_IP 0x24
#define CMD_SET_SUBNET_MASK 0x3C
#define CMD_SET_IP_GATEWAY 0x38
#define CMD_GET_DHCP_IP_GATEWAY 0x40
#define CMD_GET_DHCP_IP_MASK 0x44
#define CMD_SET_IP_MULTICAST 0x1C
#define CMD_GET_FPGA_IP 0x14
#define CMD_SET_IP_DHCP 0x18
#define CMD_SET_FPGA_MAC_LSB 0x0C
#define CMD_SET_FPGA_MAC_MSB 0x10
#define ETX 0x03
#define BUFFER_LENGTH  1024
#define UDP_PORT_SOURCE 0x28
#define UDP_PORT_DESTINATION 0x2C


#define ETX 0x03
#define BUFFER_LENGTH  1024

#define MMSB_32_8(x)		(x&0xFF000000)>>24
#define MLSB_32_8(x)		0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)		0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)		0xFF&((x&0x000000FF))

class Udpipstack{

	public:
   	struct sockaddr_in serverAddress,
        clientAddress;
	unsigned short Port = 62000;
	unsigned short port = 62000;
    unsigned char msgBuf[20];
    unsigned short len=0;
	unsigned char MAJOR;
	unsigned char MINOR;
	unsigned char INPUT;
	unsigned char OUTPUT;
	unsigned short FIFOSIZE;
	unsigned char PRESENCE_SFN;
	double clk;
	
	char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7','8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	
	int callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode);
	
	std::string hexStr(unsigned char *data, int len);

	long string2long(const std::string &str, int base = 10);

};
