#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <sstream>
#include "callcommand.h"

	
	//int rmx_ports[]={4660,4660,4660,4660,4660,4660};
	Callcommand:: Callcommand(){    
		
	}

	int Callcommand :: callCommand(int cmd,unsigned char* RxBuffer,int rx_len,int msgBLen,Json::Value json,int readWriteMode)
	{
		Json::Reader reader;
		unsigned char msgBuf[msgBLen];
		unsigned short len=0;
		unsigned short uProg;
		std::string strProg;
		int up;
		port = 4660;
		struct timeval tv;
		tv.tv_sec = 0;  /* 0.3 Secs Timeout */
		tv.tv_usec = 500000;
		int reuse_sockt=1;
		SOCKET_TYPE socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
		setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
		if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &reuse_sockt,sizeof(reuse_sockt)) == -1){
			std::cerr << "Set socket option failed\n";
	        return 0;	
		}
	    if (SOCKET_INVALID(socketHandle))
	    {
	        std::cerr << "could not make socket\n";
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
		if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress)) == -1){
			std ::cout << "bind failed\n";
			return 0;
		}

		// std::cout << sizeof(RxBuffer);
		len = 0;
	    msgBuf[0] = STX;
	    msgBuf[1] = (unsigned char) (len>>8);
	    msgBuf[2] = (unsigned char) len;
	    
	    switch(cmd){
	    	case 00:
			    msgBuf[3] = CMD_VER_FIMWARE;
			    msgBuf[4] = ETX;
	    		break;
	    	case 01:
				msgBuf[3]= CMD_VER_HARDWARE;
				msgBuf[4] = ETX;
	    		break;
	    	case 02:
	    		if(readWriteMode == 0){
	    			msgBuf[3] = CMD_CHANNEL;
				    msgBuf[4] = ETX;
	    		}else{
	    			std::string tmp1=json["input"].asString();
					int i=std::stoi(tmp1);
					char input=i ;
					std::string tmp=json["output"].asString();
					int i1=std::stoi(tmp);
					char output=i1 ;
	    			len = 2;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_CHANNEL;
					msgBuf[4] = input;//(unsigned char)std::stoi(json["input"].asString());
					msgBuf[5] = output;//(unsigned char)std::stoi(json["output"].asString());
					msgBuf[6] = ETX;
	    		}
	    		break;
	    	case 04:
	    		if(readWriteMode==0){
	    			msgBuf[3] = CMD_SET_INPUT_MODE;	
	    			msgBuf[4] = ETX;	
	    		}else{
	    			unsigned int SPTS=std::stoi(json["SPTS"].asString());	    			
	    			unsigned int PMT = std::stoi(json["PMT"].asString());
	    			unsigned int SID = std::stoi(json["SID"].asString());
	    			unsigned int BITRATE = std::stoi(json["BITRATE"].asString());
	    			unsigned int RISE=std::stoi(json["RISE"].asString());
	    			unsigned int MASTER =std::stoi(json["MASTER"].asString());
	    			unsigned int INSELECT =std::stoi(json["INSELECT"].asString());
	    			unsigned int mode=0;
	    			mode = BITRATE|(RISE << 31);
	    			mode =mode|(MASTER << 30);
   					mode =mode|(INSELECT << 29);
	    			len = 9;
				    msgBuf[0] = STX;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_SET_INPUT_MODE;
				    msgBuf[4] = (unsigned char)SPTS;
				    msgBuf[5] = (unsigned char)(PMT>>8);
				    msgBuf[6] = (unsigned char)PMT;
				    msgBuf[7] = (unsigned char)(SID>>8);
				    msgBuf[8] = (unsigned char)(SID);
				    msgBuf[9] = MMSB_32_8(mode);  
					msgBuf[10] = MLSB_32_8(mode);  
					msgBuf[11] = LMSB_32_8(mode);  
					msgBuf[12] = LLSB_32_8(mode);
				    msgBuf[13] = ETX;
	    		}
	    		break;
	    	case 05:
	    		
	    		if(readWriteMode==0){
	    			unsigned int Address=(unsigned int)std::stoi(json["address"].asString());
	    			unsigned char CMP_ID=(unsigned char)std::stoi(json["cs"].asString());
		    		len = 5;
		    		msgBuf[1] = (unsigned char) (len>>8);
		    		msgBuf[2] = (unsigned char) len;
		    		msgBuf[3] = CMD_CORE;
				    msgBuf[4] = CMP_ID;  //
				    msgBuf[5] = MMSB_32_8(Address);  
					msgBuf[6] = MLSB_32_8(Address);  
					msgBuf[7] = LMSB_32_8(Address);  
					msgBuf[8] = LLSB_32_8(Address);
				    msgBuf[9] = ETX;
				}else{
					unsigned int Address=(unsigned int)std::stoi(json["address"].asString());
	    			unsigned char CMP_ID=(unsigned char)std::stoi(json["cs"].asString());
					long unsigned int Data=(long unsigned int)std::stoul(json["data"].asString());				
					len = 9;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_CORE;
					msgBuf[4] = CMP_ID;  
					msgBuf[5] = MMSB_32_8(Address);  
					msgBuf[6] = MLSB_32_8(Address);  
					msgBuf[7] = LMSB_32_8(Address);  
					msgBuf[8] = LLSB_32_8(Address);  
					msgBuf[9] = MMSB_32_8(Data);  
					msgBuf[10] = MLSB_32_8(Data); 
					msgBuf[11] = LMSB_32_8(Data); 
					msgBuf[12] = LLSB_32_8(Data); 
					msgBuf[13] = ETX;
				}
	    		break;
	    	case 06:
		    		msgBuf[3] = CMD_GPIO;
		    		msgBuf[4] = ETX;
	    		break;
	    	case 07:
		    		len = 1;
		    		msgBuf[1] = (unsigned char) (len>>8);
		    		msgBuf[2] = (unsigned char) len;
		    		msgBuf[3] = CMD_I2C;
		    		msgBuf[4] = 0xC0A80114;
		    		msgBuf[5] = ETX;
	    		break;
	    	case 8:
		    		msgBuf[3] = CMD_SFN;
	    			msgBuf[4] = ETX;
	    		break;
	    	case 10:
	    		if(readWriteMode==0){
		    		msgBuf[3] = CMD_INIT_NIT_MODE;
	    			msgBuf[4] = ETX;
				}else{
					unsigned char uMode=(unsigned char)std::stoi(json["Mode"].asString());
					len = 1;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_INIT_NIT_MODE;
					msgBuf[4]= uMode;
					msgBuf[5] = ETX;
				}
	    		break;
	    	case 11:
		    	if(readWriteMode==0){
		    		len = 1;
		    		msgBuf[1] = (unsigned char) (len>>8);
		    		msgBuf[2] = (unsigned char) len;
		    		msgBuf[3] = CMD_INIT_NIT_CHAN;
		    		msgBuf[4] = 0;
		    		msgBuf[5] = ETX;
		    	}else{
		    		unsigned int uNum= json["uProg"].size();
		    		len = 4 * uNum;;
		    		msgBuf[1] = (unsigned char) (len>>8);
		    		msgBuf[2] = (unsigned char) len;
		    		msgBuf[3] = CMD_INIT_NIT_CHAN;
		    		std::cout<<json["uProg"]<<std::endl;
		    		unsigned short  progList[1024],chanList[1024];
					for(int i=0;i<uNum;i++){
		    			unsigned short pno 	= 	(unsigned short)std::stoi(json["uProg"][i].asString());
		    			msgBuf[4 + (i * 4)] = 	(unsigned char)(pno >> 8);
		    			msgBuf[4 + (i * 4) + 1] = (unsigned char)(pno);
		    			unsigned int cno1 	= (unsigned short)std::stoi( json["uChan"][i].asString());
		    			unsigned int chno = leftShifting(cno1,7,0);     //lats para "0" for SD for HD channel give "1" 
		    			msgBuf[4 + (i * 4) + 2] = (unsigned char)(chno >> 8);
		    			msgBuf[4 + (i * 4) + 3] = (unsigned char)(chno);
					}
					msgBuf[4 + len] = ETX;
		    	}
	    		
	    		break;
	    	case 12:
		    		msgBuf[3] = CMD_INIT_NIT_HOST;
	    			msgBuf[4] = ETX;
    			break;
    		case 13:
				if(readWriteMode==1){
					int table=(unsigned char)std::stoi(json["table"].asString());
					int timeout=(unsigned char)std::stoi(json["timeout"].asString());
					len = 3;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_INIT_START_OUTPUT;
					msgBuf[4] = (table & 0xFF);
					msgBuf[5] = (timeout >> 8) & 0xFF;
					msgBuf[6] = (timeout) & 0xFF;
					msgBuf[7] = ETX;
				}
				break;
    		case 14:
    			if(readWriteMode==0){
	    			msgBuf[3] = CMD_INIT_TS_ID;
	    			msgBuf[4] = ETX;
				}else{
					unsigned short uTS_Id=(unsigned short)std::stoi(json["transportid"].asString());
					unsigned short uNet_Id=(unsigned short)std::stoi(json["networkid"].asString());
					unsigned short uOrigNet_Id=(unsigned short)std::stoi(json["originalnwid"].asString());
					len = 6;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_INIT_TS_ID;
					msgBuf[4] = (unsigned char)(uTS_Id>>8);
					msgBuf[5] = (unsigned char)(uTS_Id);
					msgBuf[6] = (unsigned char)(uNet_Id>>8);
					msgBuf[7] = (unsigned char)(uNet_Id);
					msgBuf[8] = (unsigned char)(uOrigNet_Id>>8);
					msgBuf[9] = (unsigned char)(uOrigNet_Id);
					msgBuf[10] = ETX;
				}
    			break;
    		case 15:{
	    			unsigned char FIFO_Threshold0= (unsigned char)std::stoi(json["FIFO_Threshold0"].asString());
	    			unsigned char FIFO_Threshold1= (unsigned char)std::stoi(json["FIFO_Threshold1"].asString());
	    			unsigned char FIFO_Threshold2= (unsigned char)std::stoi(json["FIFO_Threshold2"].asString());
	    			unsigned char FIFO_Threshold3= (unsigned char)std::stoi(json["FIFO_Threshold3"].asString());
	    			len = 9;
					msgBuf[1] = (unsigned char)(len >> 8);
					msgBuf[2] = (unsigned char)len;
					msgBuf[3] = CMD_INIT_OV_FLAGS;
					msgBuf[4] = (unsigned char)std::stoi(json["mode"].asString());
					msgBuf[5] = (unsigned char)(FIFO_Threshold0 >> 8);
					msgBuf[6] = (unsigned char)FIFO_Threshold0;
					msgBuf[7] = (unsigned char)(FIFO_Threshold1 >> 8);
					msgBuf[8] = (unsigned char)FIFO_Threshold1;
					msgBuf[9] = (unsigned char)(FIFO_Threshold2 >> 8);
					msgBuf[10] = (unsigned char)FIFO_Threshold2;
					msgBuf[11] = (unsigned char)(FIFO_Threshold3 >> 8);
					msgBuf[12] = (unsigned char)FIFO_Threshold3;
					msgBuf[13] = ETX;
    			break;
    			}
    		case 16:
    			if(readWriteMode==0){
	    			msgBuf[3] = CMD_INIT_OUTPUT_MODE;
	    			msgBuf[4] = ETX;
				}else{
					unsigned long uRate=(unsigned short)std::stoi(json["rate"].asString());
					unsigned char Falling=(unsigned short)std::stoi(json["falling"].asString());
					unsigned char Mode=(unsigned short)std::stoi(json["mode"].asString());
					int payload = leftShifting(uRate,2,Falling);
					if(Mode)
						payload = payload | 2;
					len = 4;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_INIT_OUTPUT_MODE;
					// msgBuf[4] = (unsigned char)((!Falling<<7) |(Mode<<6) | ((uRate&0x3FFFFFFF)>>24));
					// msgBuf[5] = (unsigned char)(uRate>>16);
					// msgBuf[6] = (unsigned char)(uRate>>8);
					// msgBuf[7] = (unsigned char) uRate;
					msgBuf[4] = (unsigned char)(payload>>24);
					msgBuf[5] = (unsigned char)(payload>>16);
					msgBuf[6] = (unsigned char)(payload>>8);
					msgBuf[7] = (unsigned char) payload;
					msgBuf[8] = ETX;
				}
    			break;
    		case 17:
    			if(readWriteMode==0){
	    			msgBuf[3] = CMD_SET_SCHEDULER_TIME;
	    			msgBuf[4] = ETX;
				}else{
					unsigned short pat_int=(unsigned short)std::stoi(json["patint"].asString());
					unsigned short sdt_int=(unsigned short)std::stoi(json["sdtint"].asString());
					unsigned short nit_int=(unsigned short)std::stoi(json["nitint"].asString());
					len = 6;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_SET_SCHEDULER_TIME;
					msgBuf[4] = (unsigned char)(pat_int>>8);
					msgBuf[5] = (unsigned char) pat_int;
					msgBuf[6] = (unsigned char)(sdt_int>>8);
					msgBuf[7] = (unsigned char) sdt_int;
					msgBuf[8] = (unsigned char)(nit_int>>8);
					msgBuf[9] = (unsigned char) nit_int;
					msgBuf[10] = ETX;
				}
    			break;
    		case 18:
    			if(readWriteMode==0){
	    			msgBuf[3] = CMD_SET_NETWORK_NAME;
					msgBuf[4] = ETX;
				}
				else{
					std::string tmp=json["NewName"].asString();
					char y[1024];
					strcpy(y, tmp.c_str());
					char *Name=y;
					len=0;
					len = strlen(Name);
					msgBuf[0] = STX;
	   				msgBuf[1] = (unsigned char) (len>>8);
	    			msgBuf[2] = (unsigned char) len;
	    			msgBuf[3] = CMD_SET_NETWORK_NAME;
	    			memcpy(&msgBuf[4], Name, len);
	    			msgBuf[4+len] = ETX;
				}
    			break;
    		case 19:
    			if(readWriteMode==0){
					uProg = std::stoi(json.get("uProg","0").asString());
	    			len = 3;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_CHANGE_NAME;
				    msgBuf[4] = uProg >> 8;
				    msgBuf[5] = (unsigned char)uProg;
				    msgBuf[6] = 0;
				    msgBuf[7] = ETX;
			    }else{
					std::string service_name=json["NewName"].asString();
					uProg=(unsigned short)std::stoi(json["progNumber"].asString());
					int addFlag = json["addFlag"].asInt();
					len=0;
					if(addFlag != 0)
					{
						char y[1024];
						strcpy(y, service_name.c_str());
						char *NewName=y;
						std::cout<<"-----------------------------------";
						len = 2+strlen(NewName);
						for (int i=0; i<strlen(NewName); i++) {
							msgBuf[6+i] = NewName[i];	
						}
					}else{
						len = 2;
					}
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_CHANGE_NAME;
				    msgBuf[4] = uProg>>8;
					msgBuf[5] = (unsigned char)uProg;	
					msgBuf[4+len] = ETX;
				}
			    break;
			case 21:
					msgBuf[3] = CMD_DYN_STAT_WIN_SIZE;
	    			msgBuf[4] = ETX;
				break;
			case 23:
					msgBuf[3] = CMD_FILTER_CA;
	    			msgBuf[4] = ETX;
				break;
			case 24:
				if(readWriteMode==0){
					len = 1;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_CHANGE_SID;
				    msgBuf[4] = 0;
				    msgBuf[5] = ETX;
				}else{
					
					unsigned short length=json["oldpronum"].size();
					len = 4*length;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_CHANGE_SID;
					for(int i=0;i<json["oldpronum"].size();i++){
		    			unsigned short oPno = (unsigned short)std::stoi(json["oldpronum"][i].asString());
		    			msgBuf[4 + i*4] = (oPno&0xFF00)>>8;
						msgBuf[5 + i*4] = (oPno&0xFF);
						unsigned short nPno = (unsigned short)std::stoi(json["newprognum"][i].asString());
		    			msgBuf[6 + i*4] = (nPno&0xFF00)>>8;
						msgBuf[7 + i*4] = (nPno&0xFF);
					}
					msgBuf[4+length*4] = ETX;
				}
			    break;
			case 241:
				if(readWriteMode==0){
					len = 1;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_CHANGE_SID;
				    msgBuf[4] = 0;
				    msgBuf[5] = ETX;
				}else{
					
					len = 0;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_CHANGE_SID;
					msgBuf[4] = ETX;
				}
			    break;
			case 25:
				if(readWriteMode == 0 ){
					msgBuf[3] = CMD_CHANGE_VER;
				    msgBuf[4] = ETX;
				}else{
					
					len = 3;
					// int pat = leftShifting(std::stoi(json["pat_ver"].asString()),3,std::stoi(json["pat_isenable"].asString()));
					// int sdt = leftShifting(std::stoi(json["sdt_ver"].asString()),3,std::stoi(json["sdt_isenable"].asString()));
					// int nit = leftShifting(std::stoi(json["nit_ver"].asString()),3,std::stoi(json["nit_isenable"].asString()));
					int pat = (std::stoi(json["pat_ver"].asString()) | (std::stoi(json["pat_isenable"].asString()) << 7));
					int sdt = (std::stoi(json["sdt_ver"].asString()) | (std::stoi(json["sdt_isenable"].asString()) << 7));
					int nit = (std::stoi(json["nit_ver"].asString()) | (std::stoi(json["nit_isenable"].asString()) << 7));
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_CHANGE_VER;
				    msgBuf[4] = (unsigned char)pat;
				    msgBuf[5] = (unsigned char)sdt;
				    msgBuf[6] = (unsigned char)nit;
				    msgBuf[7] = ETX;
				}
					
				break;
			case 26:
				if(readWriteMode==0){
	    			uProg = std::stoi(json.get("uProg","0").asString());
	    			len = 3;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_CHANGE_PROV;
				    msgBuf[4] = uProg >> 8;
				    msgBuf[5] = (unsigned char)uProg;
				    msgBuf[6] = 0;
				    msgBuf[7] = ETX;
				}else{
					int addFlag  =json["addFlag"].asInt();
					uProg=(unsigned short)std::stoi(json["serviceNumber"].asString());
					
					msgBuf[3] = CMD_CHANGE_PROV;
				    msgBuf[4] = uProg>>8;
					msgBuf[5] = (unsigned char)uProg;
					if(addFlag){
						std::string providerName=json["providerName"].asString();
						char tempbuff[1024];
						strcpy(tempbuff, providerName.c_str());
						char *NewName=tempbuff;
						len = 2+strlen(NewName);
						for (int i=0; i<strlen(NewName); i++) {
							msgBuf[6+i] = NewName[i];	
						}
					}else{
						len = 2;
					}
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[4+len] = ETX;						
				}
				break;
			case 27:
				if(readWriteMode == 0)
				{
					msgBuf[3] = CMD_LCN_PROV;
			    	msgBuf[4] = ETX;
				}else{
					len = 1;
					msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_LCN_PROV;
					msgBuf[4] = std::stoi(json["provId"].asString());;
				    msgBuf[5] = ETX;	
				}
				break;
			case 29:
				if(readWriteMode==0){
					unsigned short length=json["service_list"].size();
					len = 2*length;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = 0x29;
				    for(int i=0;i<json["service_list"].size();i++){
		    			unsigned short oPno = (unsigned short)std::stoi(json["service_list"][i].asString());
		    			msgBuf[4 + i*2] = (oPno&0xFF00)>>8;
						msgBuf[5 + i*2] = (oPno&0xFF);
					}
					msgBuf[4+length*2] = ETX;
				}
				break;
			case 30:
				msgBuf[3] = CMD_STAT_FILTER_INFO;
			    msgBuf[4] = ETX;
				break;
			case 31:
				msgBuf[3] = CMD_STAT_FLOW_RATES;
			    msgBuf[4] = ETX;
				break;
			case 32:
				msgBuf[3] = CMD_STAT_LIST_PROG;
			    msgBuf[4] = ETX;
				break;
			case 33:
				msgBuf[3] = CMD_STAT_CRYPTED_PROG;
			    msgBuf[4] = ETX;
				break;
			case 34:{
				uProg = std::stoi(json.get("uProg","0").asString());
	    		len = 2;
				msgBuf[1] = (unsigned char) (len>>8);
				msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_GET_STAT_PROG;
				msgBuf[4] = uProg >> 8;
				msgBuf[5] = (unsigned char)uProg;
				msgBuf[6] = ETX;
				}
		    	break;
			case 35:
				msgBuf[3] = CMD_GET_STAT_ALL;
			    msgBuf[4] = ETX;
				break;
			case 36:
				if(readWriteMode==0){
				msgBuf[3] = CMD_PROG_PMT_ALARM;
			    msgBuf[4] = ETX;
				}else{
					unsigned short length=json["programs"].size();
					len = length * 3;
					msgBuf[1] = (unsigned char)(len >> 8);
					msgBuf[2] = (unsigned char)len;
					msgBuf[3] = CMD_PROG_PMT_ALARM;
					for(int i=0;i<json["programs"].size();i++){
		    			unsigned short pno = (unsigned short)std::stoi(json["programs"][i].asString());
		    			msgBuf[4 + i * 3] = (pno & 0xFF00) >> 8;
						msgBuf[5 + i * 3] = (pno & 0xFF);
						msgBuf[6 + i * 3] = (unsigned char)std::stoi(json["alarm"][i].asString());
					}
					msgBuf[4 + length * 3] = ETX;
				}
				break;

			case 37:
				if(readWriteMode==0){
					len = 1;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_PROG_ERASE_CA;
				    msgBuf[4] = 0;
				    msgBuf[5] = ETX;
				}else if(readWriteMode==2){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_PROG_ERASE_CA;
				    msgBuf[4] = ETX;
				}else{
					int num_prog=json["programNumbers"].size();
					len = num_prog*2;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_PROG_ERASE_CA;
					for(int i=0;i<num_prog;i++){
	    				unsigned short pno = (unsigned short)std::stoi(json["programNumbers"][i].asString());
	    				msgBuf[4+i*2] = (unsigned char)(pno >> 8);
						msgBuf[5+i*2] = (unsigned char) pno;
					}
					msgBuf[4+len] = ETX;
				}
				break;
			case 39:
				len = 1;
			    msgBuf[1] = (unsigned char) (len>>8);
			    msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_GET_STAT_PID;
			    msgBuf[4] = 0;
			    msgBuf[5] = ETX;
				break;
			case 40:
				if(readWriteMode==0){
					len = 1;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_FILT_PROG_STATE;
				    msgBuf[4] = 0;
				    msgBuf[5] = ETX;
			    }else{
					// int num_prog=(unsigned short)std::stoi(json["programs"].asString());
					int num_prog=json["programNumbers"].size();
					std::cout<<num_prog<<std::endl;
					len = num_prog*2;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_FILT_PROG_STATE;
					for(int i=0;i<num_prog;i++){
	    				unsigned short pno = (unsigned short)std::stoi(json["programNumbers"][i].asString());
	    				msgBuf[4+i*2] = (unsigned char)(pno >> 8);
						msgBuf[5+i*2] = (unsigned char) pno;
					}
					msgBuf[4+len] = ETX;
				}
				break;
			case 41:
				if(readWriteMode==0){
					msgBuf[3] = CMD_SET_FILT;
				    msgBuf[4] = ETX;
				}else if(readWriteMode==2){
					len=1;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_SET_FILT;
					msgBuf[4] = 0;
				    msgBuf[5] = ETX;
				}else{
					int num_prog=json["programNumbers"].size();
					len = num_prog*2;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_SET_FILT;
					for(int i=0;i<num_prog;i++){
	    				unsigned short pno = (unsigned short)std::stoi(json["programNumbers"][i].asString());
	    				msgBuf[4+i*2] = (unsigned char)(pno >> 8);
						msgBuf[5+i*2] = (unsigned char) pno;
					}
					msgBuf[4+len] = ETX;
				}
				break;
			case 42:
				if(readWriteMode==0){
				msgBuf[3] = CMD_STATUS_FILT;
			    msgBuf[4] = ETX;
			    }else if(readWriteMode==2){
					len=1;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_STATUS_FILT;
					msgBuf[4] = 0;
				    msgBuf[5] = ETX;
				}else{
					int num_prog=json["programNumbers"].size();
					len = num_prog*2;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_STATUS_FILT;
					for(int i=0;i<num_prog;i++){
	    				unsigned short pno = (unsigned short)std::stoi(json["programNumbers"][i].asString());
	    				msgBuf[4+i*2] = (unsigned char)(pno >> 8);
						msgBuf[5+i*2] = (unsigned char) pno;
					}
					msgBuf[4+len] = ETX;
				}
				break;
			case 45:
				if(readWriteMode==0){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_ENCRYPT_PROG_STATE;
				    msgBuf[4] = ETX;
				    std::cout<<"------------------------READ ENCRY -------------"<<std::endl;
				}else{
					
					unsigned short length=json["programNumbers"].size();
					if(length>0)
						len = 2*length+length;
					else
						len = 1;
					std::cout<<length<<std::endl;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_ENCRYPT_PROG_STATE;
					for(int i=0;i<json["programNumbers"].size();i++){
		    			unsigned short oPno = (unsigned short)std::stoi(json["programNumbers"][i].asString());
		    			msgBuf[4 + i*3] = (oPno&0xFF00)>>8;
						msgBuf[5 + i*3] = (oPno&0xFF);
						unsigned short nPno = (unsigned short)std::stoi(json["keyids"][i].asString());
		    			msgBuf[6 + i*3] = nPno;
					}
					if(length>0)
						msgBuf[4+length*3] = ETX;
					else{
						msgBuf[4] = 0x00;
						msgBuf[5] = ETX;
					}
				}
				break;
			case 46:
				if(readWriteMode==0){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_INIT_CSA;
				    msgBuf[4] = ETX;
				}else{
					
					len = 2;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_INIT_CSA;
				    msgBuf[4] = 1;
				    msgBuf[5] = 2;
				    msgBuf[6] = ETX;
				}
			    break;
			case 47:
				if(readWriteMode==0){
					std::cout<<"auth_bit"<<json["auth_bit"]<<json["parity"]<<std::endl;
					len = 2;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_SET_CSA;
				    msgBuf[4] = (unsigned char)std::stoi(json["auth_bit"].asString());
				    msgBuf[5] = (unsigned char)std::stoi(json["parity"].asString());
				    msgBuf[6] = ETX;
				}else{
					std::cout<<"auth_bit"<<json["auth_bit"]<<json["parity"]<<std::endl;
					int auth=std::stoi(json["auth_bit"].asString());
					int parity=std::stoi(json["parity"].asString());
					len = 2;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_SET_CSA;
				    msgBuf[4] = auth;
				    msgBuf[5] = parity;
				    msgBuf[6] = ETX;
				}
			    break;
			case 48:
				if(readWriteMode==0){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = CMD_SET_PID;
				    msgBuf[4] = ETX;
				}else{
					
					unsigned short length=json["Pids"].size();
					len = 2*length+length;
					std::cout<<length<<std::endl;
					msgBuf[0] = STX;
					msgBuf[1] = (unsigned char) (len>>8);
					msgBuf[2] = (unsigned char) len;
					msgBuf[3] = CMD_SET_PID;
					// std::cout << json["Pids"] <<std::endl;
					// std::cout<<json["AuthOutputs"]<<std::endl;
					for(int i=0;i<json["Pids"].size();i++){
		    			unsigned short oPno = (unsigned short)std::stoi(json["Pids"][i].asString());
		    			msgBuf[4 + i*3] = (oPno&0xFF00)>>8;
						msgBuf[5 + i*3] = (oPno&0xFF);
						unsigned short nPno = (unsigned short)std::stoi(json["AuthOutputs"][i].asString());
		    			msgBuf[6 + i*3] = nPno;
					}
					msgBuf[4+length*3] = ETX;
				}
			    break;
			case 49:
				if(readWriteMode==0){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = 0x49;
				    msgBuf[4] = ETX;
				}else{
					len = 0;
					
				    msgBuf[3] = 0x49;
					Json::Value CA_System_ids = json["CA_System_ids"];
					Json::Value emm_pids = json["emm_pids"];
					Json::Value private_data_list = json["private_data_list"];
					int private_data_count =0;
					int noof_CAS = CA_System_ids.size();
					if(noof_CAS>0)
					{
						for(int i=0;i<noof_CAS;i++)
						{

							int ca_system_id =std::stoi(CA_System_ids[i].asString());
							int ca_pid =std::stoi(emm_pids[i].asString());
							// std::cout<<4+i*(private_data_list[1].size()+6)<<std::endl;
							// std::cout<<ca_system_id<<std::endl;
							msgBuf[4+i*(private_data_count+6)] = 0x09;
							msgBuf[5+i*(private_data_count+6)] = 4+private_data_list[i].size();
							msgBuf[6+i*(private_data_count+6)] = ca_system_id>>8;
							msgBuf[7+i*(private_data_count+6)] = ca_system_id&0x00FF;
							msgBuf[8+i*(private_data_count+6)] = (ca_pid>>8)|0xE0;
							msgBuf[9+i*(private_data_count+6)] = ca_pid&0x00FF;
							private_data_count = private_data_list[i].size();
							for (int j = 0; j <private_data_list[i].size() ; ++j)
							{
								msgBuf[(10+i*(private_data_count+6))+j] = private_data_list[i][j].asInt();
								/* code */
							}
							len+=6+private_data_count;
						}
					}else{
						len = 1;
						msgBuf[4] = 0x00;
					}
					msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[len+4] = ETX;
				}
			    break;
			case 50:
				if(readWriteMode==0){
					len = 0;
				    msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
				    msgBuf[3] = 0x4A;
				    msgBuf[4] = ETX;
				}else{
					len = 0;
					
				    msgBuf[3] = 0x4A;
				    Json::Value elementry_pids = json["elementry_pids"];
					Json::Value CA_System_ids = json["CA_System_ids"];
					Json::Value ecm_pids = json["ecm_pids"];
					Json::Value private_data_list = json["private_data_list"];
					int private_data_count =0;
					int noof_CAS = CA_System_ids.size();
					if(noof_CAS>0)
					{
						for(int i=0;i<noof_CAS;i++)
						{

							int ca_system_id =std::stoi(CA_System_ids[i].asString());
							int ca_pid =std::stoi(ecm_pids[i].asString());
							int elementry_pid = std::stoi(elementry_pids[i].asString());
							// std::cout<<4+i*(private_data_list[1].size()+6)<<std::endl;
							// std::cout<<ca_system_id<<std::endl;
							msgBuf[4+i*(private_data_count+8)] = elementry_pid>>8|0xE0;
							msgBuf[5+i*(private_data_count+8)] = elementry_pid&0x00FF;
							msgBuf[6+i*(private_data_count+8)] = 0x09;
							msgBuf[7+i*(private_data_count+8)] = 4+private_data_list[i].size();
							msgBuf[8+i*(private_data_count+8)] = ca_system_id>>8;
							msgBuf[9+i*(private_data_count+8)] = ca_system_id&0x00FF;
							msgBuf[10+i*(private_data_count+8)] = (ca_pid>>8)|0xE0;
							msgBuf[11+i*(private_data_count+8)] = ca_pid&0x00FF;
							private_data_count = private_data_list[i].size();
							for (int j = 0; j <private_data_list[i].size() ; ++j)
							{
								msgBuf[(12+i*(private_data_count+6))+j] = private_data_list[i][j].asInt();
								/* code */
							}
							len+=8+private_data_count;
						}
					}else{
						len = 1;
						msgBuf[4] = 0x00;
					}
					msgBuf[1] = (unsigned char) (len>>8);
				    msgBuf[2] = (unsigned char) len;
					msgBuf[len+4] = ETX;
				}
			    break;
			case 53:
				msgBuf[3] = CMD_STATUS_RD_STATUS;
			    msgBuf[4] = ETX;
				break;
			case 54:
				msgBuf[3] = CMD_GET_FEATURES;
			    msgBuf[4] = ETX;
				break;
			case 58:{
				unsigned char Enable=(unsigned char)std::stoi(json["enable"].asString());
				unsigned char Ratio=(unsigned char)std::stoi(json["ratio"].asString());
				unsigned short PID	=(unsigned short)std::stoi(json["pid"].asString());
				len = 3;
				msgBuf[0] = STX;
				msgBuf[1] = (unsigned char) (len>>8);
				msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_SET_ASPECT_RATIO;
			  	msgBuf[4] = (unsigned char)(Enable<<7|Ratio<<6);
				msgBuf[5] = (unsigned char) (PID>>8);
				msgBuf[6] = (unsigned char) (PID);
				msgBuf[7] = ETX;
				}
				break;
			case 60:{
				uProg = std::stoi(json.get("uProg","0").asString());
	    		len = 2;
				msgBuf[1] = (unsigned char) (len>>8);
				msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_GET_PROG_NAME;
				msgBuf[4] = uProg >> 8;
				msgBuf[5] = (unsigned char)uProg;
				msgBuf[6] = ETX;
				}
		    	break;
		    case 61:{
				uProg = std::stoi(json.get("uProg","0").asString());
	    		len = 2;
				msgBuf[1] = (unsigned char) (len>>8);
				msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_GET_PROV_NAME;
				msgBuf[4] = uProg >> 8;
				msgBuf[5] = (unsigned char)uProg;
				msgBuf[6] = ETX;
				}
		    	break;
		    case 62:{
				uProg = std::stoi(json.get("uProg","0").asString());
	    		len = 2;
				msgBuf[1] = (unsigned char) (len>>8);
				msgBuf[2] = (unsigned char) len;
				msgBuf[3] = CMD_GET_NETWORK_NAME;
				msgBuf[4] = uProg >> 8;
				msgBuf[5] = (unsigned char)uProg;
				msgBuf[6] = ETX;
				}
		    	break;
		    case 85:{
		    	char name[100];
		    	int dir_err=1;
		    	int type = std::stoi(json["type"].asString());
		    	int value = std::stoi(json["table"].asString());
		    	std::string currtime=getCurrentTime();
		    	struct 	stat st;
		    	if(!stat("tables",&st)==0 && S_ISDIR(st.st_mode)){
			    	dir_err = mkdir("tables", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			    }
				if (-1 == dir_err)
				{
				    std::cout<<"Please create folder name 'tables' manually to work this cmd!"<<std::endl;
				}else{
					if (type == 0) {
						switch(value){
							case 00:
								fin = fopen(("tables/acq_"+currtime+".pat").c_str(), "wb");	
								break;
							case 01:
								fin = fopen(("tables/acq_"+currtime+".cat").c_str(), "wb");
								break;
							case 64:
								fin = fopen(("tables/acq_"+currtime+".nit").c_str(), "wb");
								break;
							case 66:
								fin = fopen(("tables/acq_"+currtime+".sdt").c_str(), "wb");
								break;
						}
					}
					else if (type == 5) {
						sprintf(name, ("tables/acq%d_"+currtime+".eit").c_str(), value);
						fin = fopen(name, "wb");
					}
					else {
						if (!value) {
							if (type == 2) fin = fopen(("tables/acq_"+currtime+".tdt").c_str(), "wb");
							else fin = fopen(("tables/acq_"+currtime+".tot").c_str(), "wb");
						}
						else {
							sprintf(name, ("tables/acq_%d_"+currtime+".pmt").c_str(), value);
							fin = fopen(name, "wb");
						}
					}
				}
				char ctype=type;
				//char cvalue = value;
			    len = 3;
			    msgBuf[0] = STX;
			    msgBuf[1] = (unsigned char) (len>>8);
			    msgBuf[2] = (unsigned char) len;
			    msgBuf[3] = CMD_GET_TABLE;
			    msgBuf[4] = ctype;
				msgBuf[5] = (value >> 8);
				msgBuf[6] = (value & 0xFF);  // PAT
				msgBuf[7] = ETX;	
		    }
			break;
	    }
		len += 5; 
		int count;
		int send_resp = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
		if(send_resp!=0){
			std::cout << "sent "<<send_resp<<" bytes of the msg\n";
			int serv_addr_size = sizeof(clientAddress);
			count=recvfrom(socketHandle,RxBuffer,rx_len, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);
			if(count<0)
				std ::cout <<"FAILED"<<"\n";
			if(cmd==85){
				len = ((RxBuffer[1] << 8) | RxBuffer[2]);
				if (len != 0) {
					fwrite(RxBuffer, sizeof(char), count, fin);
				}
				fclose(fin);
			}
			std::cout << "RMX Command "+std::to_string(msgBuf[3])+" ";
			std::cout << "done\n\n";
		}else{
			count =-1;
		}
	    close(socketHandle);
		shutdown(socketHandle,2);
	    return count;
	}
int Callcommand :: updateNITTable(unsigned char *ucSectiobData, unsigned short usPayloadLen,unsigned short usPointer,unsigned short usSectionNo)
{
    struct sockaddr_in serverAddress,
        clientAddress;
    unsigned short port = 4660;
    unsigned char msgBuf[300];
    unsigned char RxBuffer[105]={0};

    unsigned short FIFOSIZE;
    unsigned char PRESENCE_SFN;
    double clk;


    unsigned short len=0;
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
    len = usPayloadLen+2;
    msgBuf[0] = 0x02;
	msgBuf[1] = (unsigned char) (len>>8);
	msgBuf[2] = (unsigned char) len;
	msgBuf[3] = 0x12;
	unsigned int uiSecPointer = (0xF0 | usSectionNo); 
	msgBuf[4] = (unsigned char) (usSectionNo<<4) | (usPointer>>8);;
	msgBuf[5] = (unsigned char) usPointer&0xFF;
 	int i=0;
	for(i=0;i<usPayloadLen;i++ ){
		unsigned char ucSectiobData1 = *ucSectiobData++; 
		// printf("%x:",ucSectiobData1);
        msgBuf[6+i] = ucSectiobData1;
	}
    len = usPayloadLen+6;
	msgBuf[len] = 0x03;
	
	printf("\n-------------------usPointer --- %d SECTION -------->>%d -- >> %d \n",usPointer,usSectionNo,uiSecPointer);
	len++;
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
int Callcommand :: leftShifting(int val,int noofbit,int isenable){
        val=val<<noofbit;
        return val | isenable;
    }
std::string Callcommand :: hexStr(unsigned char *data, int len)
{
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}

long Callcommand :: string2long(const std::string &str, int base)
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
std::string Callcommand :: getCurrentTime()
    {
        time_t result = time(0);
        std::stringstream ss;
        ss << result;
        return ss.str();
    }

