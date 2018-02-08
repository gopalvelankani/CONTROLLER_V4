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
#define CHIP 0x05
#define READMODE 0x01

#define CMD_GET_VERSION_OFCORE 0x00

#define ETX 0x03
#define BUFFER_LENGTH  1024

#define MMSB_32_8(x)		(x&0xFF000000)>>24
#define MLSB_32_8(x)		0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)		0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)		0xFF&((x&0x000000FF))

class Upsampler{


   	public: void echo(char c){
   	 	std::cout<<c<<std::endl;
   	}
   	struct sockaddr_in serverAddress,
        clientAddress;
	unsigned short Port = 4660;
	unsigned short port = 4660;
    unsigned char msgBuf[20];
	//unsigned char RxBuffer[15]={0};
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
	
	int callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode)
	{
		std::cout << "gimme something: ";
		// RxBuffer1[9] = (char)'a';
		unsigned char msgBuf[msgBLen];
		unsigned short len=0;
		SOCKET_TYPE socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
	    if (SOCKET_INVALID(socketHandle))
	    {
	        std::cerr << "could not make socket\n";
	        return 0;
	    }
	    memset(&serverAddress, 0, sizeof(serverAddress));
	    memset(&clientAddress, 0, sizeof(clientAddress));
	    serverAddress.sin_family = AF_INET;
	    serverAddress.sin_port = htons(Port);
	    serverAddress.sin_addr.s_addr = inet_addr("192.168.1.20");

	    clientAddress.sin_family = AF_INET;
		clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	    clientAddress.sin_port = htons(port);
		if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress))<0){
			std ::cout << "bind failed";
		}
		std::cout << "gimme something: ";
		std::cout << sizeof(RxBuffer);
		len = 0;
	    msgBuf[0] = STX1;
	    msgBuf[1] = STX2;
		msgBuf[2] = STX3;
	    msgBuf[3] = UDPNUMBER;
	    msgBuf[4] = UNUSED;
	    msgBuf[5] = CHIP;
		unsigned short uProg;
		
		

	    switch(cmd){
	    	case 0:
				msgBuf[6] = READMODE;
			    msgBuf[7] = CMD_GET_VERSION_OFCORE;
			    msgBuf[8] = 0x00;//dont care
			    msgBuf[9] = 0x00;//dont care
			    msgBuf[10]= 0x00;//dont care
			    msgBuf[11]= 0x00;//dont care	
				break;
			

	    }
		len += 12;
		int x=0;
		int count;		
		while (x == 0)
		{ 	
					int n = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
					std::cout << "sent "<<n<<" bytes of the msg\n";
			
				
					int serv_addr_size = sizeof(clientAddress);
					count=recvfrom(socketHandle,RxBuffer,rx_len, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);

					std::cout << "sent "<<n<<" bytes of the msg\n";
					std ::cout << count<<"\n";

					// std ::cout << std::to_string(RxBuffer[0])+" -- "+std::to_string(RxBuffer[3]);
					for(int i=0;i<rx_len;i++){
						std ::cout << hexStr(&RxBuffer[i],1)<<"\n" ;
						std::cout << string2long(hexStr(&RxBuffer[i],1), 16) << std::endl;
					}
					std::cout << "done\n";
					x=1;
		}
		
		close(socketHandle);
	    
		
	    return count;
	}
	
std::string hexStr(unsigned char *data, int len)
{
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}

long string2long(const std::string &str, int base = 10)
{
	long result = 0;

	if (base < 0 || base > 36)
		return result;

	for (size_t i = 0, mx = str.size(); i < mx; ++i)
	{
		char ch = tolower(str[i]);
		int num = 0;

		if ('0' <= ch && ch <= '9')
			num = (ch - '0');
		else if ('a' <= ch && ch <= 'z')
			num = (ch - 'a') + 10;
		else
			break;

		if (num > base)
			break;

		result = result*base + num;
	}

	return result;
}

};
