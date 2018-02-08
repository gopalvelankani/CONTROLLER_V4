#include <iostream>
#include <iomanip>
#include "Itoc.h"
using namespace std;
 
// Itoc :: Itoc() 
// {}
 
// void Itoc :: setTime(const int h, const int m, const int s) 
// {
//      hour = h;
//      minute = m;
//      second = s;     
// }		
 
// void Itoc :: print() const
// {
//      cout << setw(2) << setfill('0') << hour << ":"
// 	<< setw(2) << setfill('0') << minute << ":"
//  	<< setw(2) << setfill('0') << second << "\n";	
 
// }
 
// bool Itoc :: equals(const Itoc &otherTime)
// {
//      if(hour == otherTime.hour 
//           && minute == otherTime.minute 
//           && second == otherTime.second)
//           return true;
//      else
//           return false;
// }

int Itoc :: callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode)
     {
          std::cout << "gimme something: ";
          // RxBuffer1[9] = (char)'a';
          unsigned char msgBuf[msgBLen];
          unsigned short len=0;
          struct timeval tv;

          tv.tv_sec = 0;  /* 0.5 Secs Timeout */
          tv.tv_usec = 500000;
          
          // reader.parse(json);
          SOCKET_TYPE socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
          setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
 
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
         msgBuf[5] = 0x04;
          unsigned short uProg;
          
          

         switch(cmd){
          case 1:
                    msgBuf[6] = READMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;//dont care
                   msgBuf[9] = 0x00;//dont care
                   msgBuf[10]= 0x00;//dont care
                   msgBuf[11]= 0x00;//dont care
                    break;
               case 2:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x01; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x02;
                    break;
               case 3:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = FOUR;
                   msgBuf[8] = 0x1a;  
                    msgBuf[9] = 0x00; 
                    msgBuf[10] = 0x04; 
                    msgBuf[11] = 0x00;
                    break;
               case 4:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x05; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x02;
                    break;
               case 5:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x01; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x04;
                    break;
               case 6:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = FOUR;
                   msgBuf[8] = 0x1b;  
                    msgBuf[9] = 0x00; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x00;
                    break;

               case 7:
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x07; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x04;
                    break;
                    
               case 8:
                    msgBuf[6] = READMODE;
                   msgBuf[7] = FOUR;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x00; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x00;
                    break;    
               case 9:
                    msgBuf[6] = READMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x00; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x00;
                    break;
               case 10:
                    if(readWriteMode==1){
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x01; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x06;
                    }
                    break;
               case 11:
                    if(readWriteMode==1){
                    string tmp=json["frequency"].asString();
                    int i=std::stoi(tmp);
                    int x = (int)((4294967296*i)/2141.772152303);
                    msgBuf[6] = WRITEMODE; 
                    msgBuf[7] = FOUR;
                    msgBuf[8] = 0x1a;  
                    msgBuf[9] = 0x00; 
                    msgBuf[10] = 0x04; 
                    msgBuf[11] = MMSB_32_8(x); 
                    }
                    break;
                    
               case 12:
                    if(readWriteMode==1){
                    string tmp1=json["frequency"].asString();
                    int i1=std::stoi(tmp1);
                    int x1 = (int)((4294967296*i1)/2141.772152303);
                    msgBuf[6] = WRITEMODE;
                    msgBuf[7] = FOUR;
                    msgBuf[8] = MLSB_32_8(x1);   
                    msgBuf[9] = LMSB_32_8(x1);  
                    msgBuf[10] = LLSB_32_8(x1)+1;  
                    msgBuf[11] = 0x00;
                    }
                    break;
                    
                    
               case 13:
                    if(readWriteMode==1){
                    msgBuf[6] = WRITEMODE;
                   msgBuf[7] = EIGHT;
                   msgBuf[8] = 0x00;  
                    msgBuf[9] = 0x05; 
                    msgBuf[10] = 0x00; 
                    msgBuf[11] = 0x06;
                    }
                    break;   
              case 14:
                 msgBuf[6] = WRITEMODE;
                 msgBuf[7] = 0x14;
                 msgBuf[8] = 0x00;  
                  msgBuf[9] = 0x00; 
                  msgBuf[10] = 0x00; 
                  msgBuf[11] = 0x00;
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
                         if(count<0)
                            std ::cout <<"FAILED"<<"\n";

                         // std ::cout << std::to_string(RxBuffer[0])+" -- "+std::to_string(RxBuffer[3]);
                         // for(int i=0;i<rx_len;i++){
                         //      std ::cout << hexStr(&RxBuffer[i],1)<<"\n" ;
                         //      std::cout << string2long(hexStr(&RxBuffer[i],1), 16) << std::endl;
                         // }
                         std::cout << "done\n";
                         x=1;
          }
          
          close(socketHandle);
         
          
         return count;
     }
std::string Itoc ::  hexStr(unsigned char *data, int len)
{
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}

long Itoc ::  string2long(const std::string &str, int base = 10)
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