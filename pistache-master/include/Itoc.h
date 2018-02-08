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
#define UNUSED 0x00
#define CHIPS 0x04
#define READMODE 0x01
#define WRITEMODE 0xF0
#define EIGHT 0X08
#define FOUR 0X04
#define MMSB_32_8(x)          (x&0xFF000000)>>24
#define MLSB_32_8(x)          0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)          0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)          0xFF&((x&0x000000FF))
#define BUFFER_LENGTH  1024

#define MMSB_32_8(x)    (x&0xFF000000)>>24
#define MLSB_32_8(x)    0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)    0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)    0xFF&((x&0x000000FF))
class Itoc{


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
        
        char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
          FILE *fin;
        
        int callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode);
        int leftShifting(int val,int noofbit,int isenable);
        std::string hexStr(unsigned char *data, int len);
        long string2long(const std::string &str, int base);
        std::string getCurrentTime();

};