#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#define SOCKET_INVALID(s) (s<0)
#define SOCKET_TYPE int
#define STX 0x02
#define ETX 0x03
#define BUFFER_LENGTH  1024
#define CMD_VER_FIMWARE			0x00
#define CMD_VER_HARDWARE 		0x01
#define CMD_CHANNEL				0x02
#define CMD_SET_INPUT_MODE 		0x04
#define CMD_CORE 				0x05
#define CMD_GPIO 				0x06
#define CMD_I2C 				0x07
#define CMD_SFN 				0x08
#define CMD_INIT_NIT_MODE       0x10
#define CMD_INIT_NIT_CHAN       0x11
#define CMD_INIT_NIT_HOST       0x12
#define	CMD_INIT_START_OUTPUT   0x13
#define CMD_INIT_TS_ID			0x14
#define CMD_INIT_OV_FLAGS		0x15
#define CMD_INIT_OUTPUT_MODE	0x16
#define CMD_SET_SCHEDULER_TIME	0x17
#define CMD_SET_NETWORK_NAME	0x18
#define CMD_CHANGE_NAME			0x19
#define CMD_CHANGE_PROV			0x1A
#define CMD_LCN_PROV			0x1B
#define CMD_DYN_RST_FIFO_CNT	0x20
#define CMD_DYN_STAT_WIN_SIZE	0x21
#define CMD_LOOPBACK_MODE		0x22
#define CMD_FILTER_CA			0x23
#define CMD_CHANGE_SID			0x24
#define CMD_CHANGE_VER			0x25
#define CMD_STAT_FILTER_INFO	0x30
#define CMD_STAT_FLOW_RATES		0x31
#define CMD_STAT_LIST_PROG		0x32
#define CMD_STAT_CRYPTED_PROG	0x33
#define CMD_GET_STAT_PROG		0x34
#define CMD_GET_STAT_ALL		0x35
#define CMD_PROG_PMT_ALARM		0x36
#define CMD_PROG_ERASE_CA		0x37
#define CMD_PROG_LIST_PID		0x38
#define CMD_GET_STAT_PID		0x39
#define CMD_SET_ASPECT_RATIO	0x3A
#define CMD_FILT_PROG_STATE		0x40
#define CMD_FILT_PROG_STATE		0x40
#define CMD_ENCRYPT_PROG_STATE		0x45
#define CMD_INIT_CSA		        0x46
#define CMD_SET_CSA		        0x47
#define CMD_SET_PID		        0x48
#define CMD_SET_FILT			0x41
#define CMD_STATUS_FILT			0x42
#define CMD_GET_FILTER			0x43
#define CMD_STATUS_RD_STATUS	0x53
#define CMD_GET_FEATURES		0x54
#define CMD_GET_TABLE			0x55
#define CMD_GET_PROG_NAME		0x60
#define CMD_GET_PROV_NAME		0x61
#define CMD_GET_NETWORK_NAME	0x62

#define MMSB_32_8(x)		(x&0xFF000000)>>24
#define MLSB_32_8(x)		0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)		0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)		0xFF&((x&0x000000FF))
class Callcommand{

   	public:
   	struct sockaddr_in serverAddress,
        clientAddress;
	unsigned short Port = 4660;
	unsigned short port = 4660;
    unsigned char msgBuf[20];
    unsigned short len=0;
	unsigned char MAJOR;
	unsigned char MINOR;
	unsigned char INPUT;
	unsigned char OUTPUT;
	unsigned short FIFOSIZE;
	unsigned char PRESENCE_SFN;
	double clk;
	
	char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    FILE *fin;
    Callcommand();
	Callcommand(std::string path);
	int callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode =0);
	int leftShifting(int val,int noofbit,int isenable);
	std::string hexStr(unsigned char *data, int len);
	long string2long(const std::string &str, int base = 10);
	std::string getCurrentTime();
};