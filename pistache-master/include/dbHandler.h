// This is start of the header guard.  ADD_H can be any unique name.  By convention, we use the name of the header file.
#ifndef DBHANDLER_H
#define DBHANDLER_H
#include <iostream>
#include <jsoncpp/json/json.h>
#include </usr/include/mysql/mysql.h>
#include <stdio.h>
#include <string> 
#include "config.h"
using namespace std;

class dbHandler
{
public:
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	Config cnf;
	MYSQL *connect;
	Json::Value jsonArray;
	dbHandler();
	dbHandler(std::string config_path);
	~dbHandler();
	Json::Value getRecords() ;
	int addHWversion(int major_ver,int minor_ver,int no_of_input,int no_of_output,int fifo_size,int presence_sfn,double clk) ;
	int addFirmwareDetails(int major_ver,int minor_ver,int standard,int cust_opt_id,int cust_opt_ver);

	int addChannelList(int input,int channel_number,int rmx_no);
	int addActivatedPrograms(std::string input,std::string output,Json::Value program_number,int rmx_no,int incFlag,std::string prog_list_str);
	Json::Value getActivePrograms(int input,int output);
	int addLcnNumbers(std::string program_number,std::string channel_number,std::string input,int rmx_no);
	int addPmtAlarm(std::string program_number,std::string alarm,std::string input,int rmx_no);
	int addFrequency(std::string center_frequency,std::string str_rmx_no);
	int addChannelname(int channel_number,std::string channel_name,int rmx_no,int addOrDel);
	int flushServiceNames();
	int addServiceId(int channel_number,int service_id,int rmx_no,int addFlag);
	int flushServiceId();
	int addNetworkname(std::string network_name,std::string output,int rmx_no);
	int addTablesVersion(std::string pat_ver,std::string pat_isenable,std::string std_ver,std::string sdt_isenable,std::string nit_ver,std::string nit_isenable,std::string output,int rmx_no);
	int addTableTimeout(std::string table,std::string timeout,int rmx_no);
	int addNetworkDetails(std::string output,std::string transportid,std::string networkid,std::string originalnwid,int rmx_no);
	Json::Value getNetworkDetails();
	int addNitMode(std::string mode,std::string output,int rmx_no);
	int addfreeCAModePrograms(std::string programNumber,std::string input,std::string output,int rmx_no); 
	int addHighPriorityServices(std::string program_number,std::string input,int rmx_no);
	int addLockedPrograms(std::string program_number,std::string input,int rmx_no);
	int addNewProviderName(std::string program_number,std::string NewName,int rmx_no);
	int addLcnProviderid(int provider_id,int rmx_no);
	int deletefreeCAModePrograms();
	int deleteLockedPrograms();
	int deleteHighPriorityServices();
	
	int addECMChannelSetup(int channel_id,std::string supercas_id ,std::string ip,int port);
	int updateECMChannelSetup(int channel_id,std::string supercas_id ,std::string ip,int port,int old_channel_id);
	int addECMStreamSetup(int stream_id,int ecm_id,int channel_id,std::string access_criteria,int cp_number,std::string timestamp);
	int updateECMStreamSetup(int stream_id,int ecm_id,int channel_id,std::string access_criteria,int cp_number,std::string timestamp);
	int deleteECM(int channel_id);
	int deleteECMStream(int channel_id,int stream_id);

	int addEMMchannelSetup(int channel_id, std::string client_id ,int data_id,int bw,int port,int stream_id);
	int updateEMMchannelSetup(int channel_id, std::string client_id, int data_id, int bw,int port,int stream_id);
	int deleteEMM(int channel_id);

	int scrambleService(int channel_id,int stream_id,int service_id);
	int deScrambleService(int channel_id,int stream_id,int service_id);
	
	int addInputMode(std::string SPTS,std::string PMT,std::string SID,std::string RISE,std::string MASTER,std::string INSELECT,std::string BITRATE,std::string input,int rmx_no);
	int createAlarmFlags(std::string mode,std::string level1,std::string level2,std::string level3,std::string level4,int rmx_no);
	int addDVBspiOutputMode(std::string output,std::string bit_rate,std::string falling,std::string mode,int rmx_no);  
	int addPsiSiInterval(std::string patint,std::string sdtint,std::string nitint,std::string output,int rmx_no);
	int addRmxRegisterData(std::string cs,std::string address ,std::string data,int rmx_no);
	Json::Value getCenterFrequency();
	int getServiceInputChannel(std::string service_no);
	Json::Value getServiceNewnames();
	Json::Value getServiceIds(int rmx_no);
	Json::Value getServiceIds(int rmx_no,std::string old_service_id);
	Json::Value getStreams();
	Json::Value getChannels();
	Json::Value getEMMChannels();
	Json::Value getScrambledServices();
	Json::Value getLcnNumbers();
	Json::Value getLcnNumbers(std::string input,std::string program_number );
	Json::Value getHighPriorityServices();
	Json::Value getHighPriorityServices(std::string input,std::string program_number );
	Json::Value getPmtalarm(std::string input,std::string program_number );
	Json::Value getPmtalarm();
	Json::Value getActivePrograms(std::string program_number,std::string input,std::string output ,std::string rmx_no);
	Json::Value getActivePrograms();
	Json::Value getLockedPids(std::string input,std::string program_number);
	Json::Value getLockedPids();
	Json::Value getFreeCAModePrograms(std::string program_number,std::string input,std::string output);
	Json::Value getFreeCAModePrograms();
	Json::Value getInputMode();
	Json::Value getLcnProviderId();
	Json::Value getAlarmFlags();
	Json::Value getTablesVersions();
	Json::Value getNITmode();
	Json::Value getNITtableTimeout();
	Json::Value getDVBspiOutputMode();
	Json::Value getPsiSiInterval();
	Json::Value getNewProviderName();

	int isStreamAlreadUsed(int channel_id,int stream_id);
	int isECMStreamExists(int channel_id,int stream_id);
	int isECMExists(int channel_id);
	int isEMMExists(int channel_id);

	int addTunerDetails(int mxl_id,int rmx_no,int demod_id,int lnb_id,int dvb_standard,int frequency,int symbol_rate,int mod,int fec,int rolloff,int pilots,int spectrum,int lo_frequency);
	Json::Value getRFTunerDetails();
	int addConfAllegro(int mxl_id, int address,int enable1,int volt1,int enable2,int volt2);
	Json::Value getConfAllegro();
	Json::Value getConfAllegro(int mxl_id, int address);
	int addRFauthorization(int rmx_no, int enable);
	Json::Value getRFauthorizedRmx();
	int addIPOutputChannels(int rmx_no, int out_channel, std::string ip_address,int port);
	int removeIPOutputChannels(int rmx_no, int out_channel);
	Json::Value getIPOutputChannels();
	int addIPInputChannels(int rmx_no, int input_channel, std::string ip_address,int port,int type);
	int getTunerChannelType(int contr_fpga,int rmx_no,int channel_no,int type);
	int getTunerChannelType(int contr_fpga);
	int addTunerChannelType(int rmx_no,int channel_no,int type);
	Json::Value getIPTunerDetails();
	int removeIPInputChannels(int rmx_no, int input_channel);
};
// This is the content of the .h file, which is where the declarations go

// This is the end of the header guard
#endif
