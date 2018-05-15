// This is start of the header guard.  ADD_H can be any unique name.  By convention, we use the name of the header file.

#include <iostream>
#include <jsoncpp/json/json.h>
#include </usr/include/mysql/mysql.h>
#include <stdio.h>
#include <string> 
#include "dbHandler.h"
using namespace std;

	dbHandler :: dbHandler(){}
	dbHandler :: dbHandler(std::string config_path){
		cnf.readConfig(config_path);
		// std::cout<<cnf.DB_NAME<<"****"<<cnf.DB_HOST<<"****"<<cnf.DB_USER<<"****"<<cnf.DB_PASS<<std::endl;
		connect=mysql_init(NULL);
		if (!connect)
		{
			cout<<"MySQL Initialization failed";
			exit(1);
		}
		connect=mysql_real_connect(connect, (cnf.DB_HOST).c_str(), (cnf.DB_USER).c_str(), (cnf.DB_PASS).c_str() , 
		(cnf.DB_NAME).c_str() ,0,NULL,0);
		if (connect)
		{
			cout<<"MYSQL connection Succeeded\n";
		}
		else
		{
			cout<<"MYSQL connection failed\n";
			exit(1);
		}

	}
	dbHandler :: ~dbHandler(){
		mysql_close (connect);
	} 
	Json::Value dbHandler :: getRecords()
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		mysql_query (connect,"select * from data;");
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		unsigned int numrows = mysql_num_rows(res_set);
		if(numrows>0)
		{
			jsonArray["error"] = false;
			jsonArray["message"] = "Records found!";
			Json::Value jsonList;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				Json::Value jsonObj;
				jsonObj["id"]=row[i];
				jsonObj["name"]=row[i+1];
				jsonList.append(jsonObj);
			}
			jsonArray["list"]=jsonList;
		}else{
			jsonArray["error"] = true;
			jsonArray["message"] = "No records found!";
		}
		
	 	return jsonArray;
	}
	int dbHandler :: addHWversion(int major_ver,int minor_ver,int no_of_input,int no_of_output,int fifo_size,int presence_sfn,double clk) 
	{
		string query = "Insert into HWversion (major_ver,min_ver,no_of_input,no_of_output,fifo_size,options_implemented,core_clk) VALUES ('"+std::to_string(major_ver)+"','"+std::to_string(minor_ver)+"','"+std::to_string(no_of_input)+"','"+std::to_string(no_of_output)+"','"+std::to_string(fifo_size)+"','"+std::to_string(presence_sfn)+"','"+std::to_string(clk)+"');";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addFirmwareDetails(int major_ver,int minor_ver,int standard,int cust_opt_id,int cust_opt_ver) 
	{
		string query = "Insert into Firmware (major_ver,min_ver,standard,cust_option_id,cust_option_ver) VALUES ('"+std::to_string(major_ver)+"','"+std::to_string(minor_ver)+"','"+std::to_string(standard)+"','"+std::to_string(cust_opt_id)+"','"+std::to_string(cust_opt_ver)+"');";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}

	int dbHandler :: addChannelList(int input,int channel_number,int rmx_no) 
	{
		string query = "Insert into channel_list (input_channel,channel_number,rmx_id) VALUES ('"+std::to_string(input)+"','"+std::to_string(channel_number)+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE channel_number = '"+std::to_string(channel_number)+"' ;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addActivatedPrograms(std::string input,std::string output,Json::Value program_numbers,int rmx_no,int includeflag,std::string prog_list_str) 
	{
		std::string query="";
		if(includeflag){
			query = "Insert into active_service_list (in_channel,out_channel,channel_num,rmx_id) VALUES ";
			for (int i = 0; i < program_numbers.size(); ++i)
				query = query+"('"+input+"','"+output+"','"+program_numbers[i].asString()+"','"+std::to_string(rmx_no)+"'),";  	
			query = query.substr(0,query.length()-1);
			query =query+" ON DUPLICATE KEY UPDATE channel_num = channel_num;";
		}
		else{
			query = "DELETE FROM active_service_list WHERE in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num IN("+ prog_list_str+") AND rmx_id = '"+std::to_string(rmx_no)+"';";  
		}
		// std::cout<<"addActivatedPrograms--------------------------\n"<<query;
		// std::cout<<"\n"<<query;
		mysql_query (connect,query.c_str());
		
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addEncryptedPrograms(std::string input,std::string output,Json::Value program_numbers,int rmx_no,int includeflag,std::string prog_list_str,Json::Value keyIndex) 
	{
		std::string query="";
		if(includeflag){
			query = "Insert into encrypted_service_list (in_channel,out_channel,channel_num,rmx_id,key_index) VALUES ";
			for (int i = 0; i < program_numbers.size(); ++i)
				query = query+"('"+input+"','"+output+"','"+program_numbers[i].asString()+"','"+std::to_string(rmx_no)+"','"+keyIndex[i].asString()+"'),";  	
			query = query.substr(0,query.length()-1);
			query =query+" ON DUPLICATE KEY UPDATE channel_num = channel_num;";
		}
		else{
			query = "DELETE FROM encrypted_service_list WHERE in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num IN("+ prog_list_str+") AND rmx_id = '"+std::to_string(rmx_no)+"';";  
		}
		// std::cout<<"addActivatedPrograms--------------------------\n"<<query;
		// std::cout<<"\n"<<query;
		mysql_query (connect,query.c_str());
		
		return mysql_affected_rows(connect);
	}
	
	int dbHandler :: addFrequency(std::string center_frequency,std::string str_rmx_no,unsigned int* uiFrequencies) 
	{
		
		string query = "Insert into Ifrequency (rmx_id,ifrequency) VALUES ('"+str_rmx_no+"','"+center_frequency+"') ON DUPLICATE KEY UPDATE ifrequency = '"+center_frequency+"';";  
		mysql_query (connect,query.c_str());
		int affctd= mysql_affected_rows(connect);
		
		for (int output = 0; output < 8; ++output)
		{
			unsigned int uiFerq = *(uiFrequencies++);
			// std::cout<<uiFerq<<endl;
			query = "UPDATE output SET frequency = '"+std::to_string(uiFerq)+"' WHERE rmx_id = '"+str_rmx_no+"' AND output_channel = '"+std::to_string(output)+"'";
			mysql_query (connect,query.c_str());	
		}
		return affctd;
	}
	int dbHandler :: addSymbolRate(std::string symbol_rate,std::string str_rmx_no,std::string output,std::string rolloff){
		std::string query = "UPDATE output SET symbol_rate = '"+symbol_rate+"',roll_off = '"+rolloff+"' WHERE rmx_id = '"+str_rmx_no+"' AND output_channel = '"+output+"'";
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
		}
	int dbHandler :: addChannelname(int channel_number,std::string channel_name,int rmx_no,int addOrDel)
	{
		string query="";
		if(addOrDel==1)
		{
			query = "Insert into new_service_namelist (channel_number,channel_name,rmx_id) VALUES ('"+std::to_string(channel_number)+"','"+channel_name+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE channel_name = '"+channel_name+"'  ;";  
		}else
			query = "Insert into new_service_namelist (channel_number,channel_name,rmx_id) VALUES ('"+std::to_string(channel_number)+"','-1','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE channel_name = '-1'  ;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: flushServiceNames()
	{
		string query = "UPDATE new_service_namelist SET channel_name = '-1' WHERE channel_name <> '-1';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addServiceId(int channel_number,int service_id,int rmx_no,int addFlag)
	{
		string query="";
		if(addFlag){
			query = "Insert into new_service_namelist (channel_number,service_id,rmx_id) VALUES ('"+std::to_string(channel_number)+"','"+std::to_string(service_id)+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE service_id = '"+std::to_string(service_id)+"';";  
		}else{
			query = "Insert into new_service_namelist (channel_number,service_id,rmx_id) VALUES ('"+std::to_string(channel_number)+"','-1','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE service_id = '-1';";  
		}
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: flushServiceId()
	{
		string query = "UPDATE new_service_namelist SET service_id = '-1' WHERE service_id <> '-1';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	
	int dbHandler :: addNetworkname(std::string network_name,std::string output,int rmx_no) 
	{
		string query = "Insert into network_details(output,network_name,rmx_id) VALUES ('"+output+"','"+network_name+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE network_name = '"+network_name+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addTablesVersion(std::string pat_ver,std::string pat_isenable,std::string sdt_ver,std::string sdt_isenable,std::string nit_ver,std::string nit_isenable,std::string output,int rmx_no)
	{
		string query = "";

		if(rmx_no != -1)
			query = "Insert into table_versions(output,pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,rmx_id) VALUES ('"+output+"','"+pat_ver+"','"+pat_isenable+"','"+sdt_ver+"','"+sdt_isenable+"','"+nit_ver+"','"+nit_isenable+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE pat_ver = '"+pat_ver+"',pat_isenable = '"+pat_isenable+"',sdt_ver = '"+sdt_ver+"',sdt_isenable = '"+sdt_isenable+"',nit_ver = '"+nit_ver+"',nit_isenable = '"+nit_isenable+"';";
		else
			query = "UPDATE table_versions SET pat_ver = '"+pat_ver+"',pat_isenable = '"+pat_isenable+"',sdt_ver = '"+sdt_ver+"',sdt_isenable = '"+sdt_isenable+"',nit_ver = '"+nit_ver+"',nit_isenable = '"+nit_isenable+"';";
		mysql_query (connect,query.c_str());
		std::cout<<"------------------------------------->"<<query<<endl;
		return mysql_affected_rows(connect);
	}
	
	int dbHandler :: addNitMode(std::string mode,std::string output,int rmx_no)
	{
		string query = "UPDATE output SET nit_mode = '"+mode+"' WHERE output_channel = '"+output+"' AND rmx_id = '"+std::to_string(rmx_no)+"';";  
		mysql_query (connect,query.c_str());
		// std::cout<<query<<std::endl;
		return mysql_affected_rows(connect);
	}

	int dbHandler :: addLcnNumbers(std::string program_number,std::string channel_number,std::string input,int rmx_no,int addFlag)
	{
			string query ="";
			if(addFlag)
				query = "UPDATE channel_list SET lcn_id = '"+channel_number+"' WHERE input_channel = '"+input+"' AND channel_number = '"+program_number+"' AND rmx_id = '"+std::to_string(rmx_no)+"'";
			else  
				query = "UPDATE channel_list SET lcn_id = '-1' WHERE input_channel = '"+input+"' AND channel_number = '"+program_number+"' AND rmx_id = '"+std::to_string(rmx_no)+"'";  
			std::cout<<query<<std::endl;
			mysql_query (connect,query.c_str());
			return mysql_affected_rows(connect);
	}
	int dbHandler :: addPmtAlarm(std::string program_number,std::string alarm,std::string input,int rmx_no)
	{
		//for(int i=0;i<root.size();i++){
			string query = "Insert into pmt_alarms (program_number,input,alarm_enable,rmx_id) VALUES ('"+program_number+"','"+input+"','"+alarm+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE alarm_enable = '"+alarm+"';";  
			mysql_query (connect,query.c_str());
		//}
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: addNetworkDetails(std::string output,std::string transportid,std::string networkid,std::string originalnwid,int rmx_no) 
	{
		string query = "Insert into network_details(output,ts_id,network_id,original_netw_id,rmx_id) VALUES ('"+output+"','"+transportid+"','"+networkid+"','"+originalnwid+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE ts_id = '"+transportid+"',network_id = '"+networkid+"',original_netw_id = '"+originalnwid+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}

	int dbHandler :: addfreeCAModePrograms(std::string programNumber,std::string input,std::string output,int rmx_no) 
	{
		// for(int i=0;i<root.size();i++){
			string query = "Insert into freeCA_mode_programs (program_number,input_channel,output_channel,rmx_id) VALUES ('"+programNumber+"','"+input+"','"+output+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE program_number = '"+programNumber+"'  ;";  
			mysql_query (connect,query.c_str());
		// }
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addHighPriorityServices(std::string program_number,std::string input,int rmx_no) 
	{
		string query = "Insert into highPriorityServices (program_number,input,rmx_id) VALUES ('"+program_number+"','"+input+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE input = '"+input+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addLockedPrograms(std::string program_number,std::string input,int rmx_no) 
	{
		string query = "Insert into locked_programs (program_number,input,rmx_id) VALUES ('"+program_number+"','"+input+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE program_number = '"+program_number+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addLcnProviderid(int provider_id, int rmx_no) 
	{
		string query = "Insert into lcn_provider_id (rmx_id,provider_id) VALUES ('"+std::to_string(rmx_no)+"','"+std::to_string(provider_id)+"') ON DUPLICATE KEY UPDATE provider_id = '"+std::to_string(provider_id)+"'  ;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addNewProviderName(std::string service_number,std::string serviceName,std::string rmx_no,std::string addFlag) 
	{
		string query = "";
		if(std::stoi(addFlag))
			query = "Insert into service_providers (service_number,provider_name,rmx_id) VALUES ('"+service_number+"','"+serviceName+"','"+rmx_no+"') ON DUPLICATE KEY UPDATE provider_name = '"+serviceName+"';";  
		else
			query = "DELETE FROM service_providers WHERE service_number = '"+service_number+"' AND rmx_id = '"+rmx_no+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addInputMode(std::string SPTS,std::string PMT,std::string SID,std::string RISE,std::string MASTER,std::string INSELECT,std::string BITRATE,std::string input,int rmx_no){

		string query = "Insert into input_mode (input,is_spts,pmt,sid,rise,master,in_select,bitrate,rmx_id) VALUES ('"+input+"','"+SPTS+"','"+PMT+"','"+SID+"','"+RISE+"','"+MASTER+"','"+INSELECT+"','"+BITRATE+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE is_spts='"+SPTS+"',pmt='"+PMT+"',sid='"+SID+"',rise='"+RISE+"',master='"+MASTER+"',in_select='"+INSELECT+"',bitrate='"+BITRATE+"',rmx_id = '"+std::to_string(rmx_no)+"';";  
		// std::cout<<"-->"<<query<<std::endl;
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: createAlarmFlags(std::string mode,std::string level1,std::string level2,std::string level3,std::string level4,int rmx_no){
		string query = "Insert into create_alarm_flags (rmx_id,level1,level2,level3,level4,mode) VALUES ('"+std::to_string(rmx_no)+"','"+level1+"','"+level2+"','"+level3+"','"+level4+"','"+mode+"') ON DUPLICATE KEY UPDATE level1='"+level1+"',level2='"+level2+"',level3='"+level3+"',level4='"+level4+"',mode='"+mode+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addTableTimeout(std::string table,std::string timeout,int rmx_no)
	{
		string query = "Insert into NIT_table_timeout (timeout,table_id,rmx_id) VALUES ('"+timeout+"','"+table+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE timeout = '"+timeout+"'  ;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addDVBspiOutputMode(std::string output,std::string bit_rate,std::string falling,std::string mode,int rmx_no){
		string query = "Insert into dvb_spi_output (output,bit_rate,falling,mode,rmx_id) VALUES ('"+output+"','"+bit_rate+"','"+falling+"','"+mode+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE bit_rate='"+bit_rate+"',falling='"+falling+"',mode='"+mode+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addPsiSiInterval(std::string patint,std::string sdtint,std::string nitint,std::string output,int rmx_no){
		string query ="";
		if(rmx_no != -1){
		 query = "Insert into psi_si_interval (output,pat_int,sdt_int,nit_int,rmx_id) VALUES ('"+output+"','"+patint+"','"+sdtint+"','"+nitint+"','"+std::to_string(rmx_no)+"') ON DUPLICATE KEY UPDATE pat_int='"+patint+"',sdt_int='"+sdtint+"',nit_int='"+nitint+"';";  
		}
		else{
			query = "UPDATE psi_si_interval SET pat_int='"+patint+"',sdt_int='"+sdtint+"',nit_int='"+nitint+"';";  
		}
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: addRmxRegisterData(std::string cs,std::string address ,std::string data,int rmx_no){
		string query = "Insert into rmx_registers (rmx_id,cs,address,data) VALUES ('"+std::to_string(rmx_no)+"','"+cs+"','"+address+"','"+data+"') ON DUPLICATE KEY UPDATE data='"+data+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}

	int dbHandler :: addTunerDetails(int mxl_id,int rmx_no,int demod_id,int lnb_id,int dvb_standard,int frequency,int symbol_rate,int mod,int fec,int rolloff,int pilots,int spectrum,int lo_frequency){
		string query = "UPDATE ip_input_channels SET is_enable = 0 WHERE rmx_no = '"+std::to_string(rmx_no)+"' AND input_channel = '"+std::to_string(demod_id+1)+"';";  
		mysql_query (connect,query.c_str());

		query = "Insert into tuner_details (mxl_id,rmx_no,demod_id,lnb_id,dvb_standard,frequency,symbol_rate,modulation,fec,rolloff,pilots,spectrum,lo_frequency,is_enable) VALUES ('"+std::to_string(mxl_id)+"','"+std::to_string(rmx_no)+"','"+std::to_string(demod_id)+"','"+std::to_string(lnb_id)+"','"+std::to_string(dvb_standard)+"','"+std::to_string(frequency)+"','"+std::to_string(symbol_rate)+"','"+std::to_string(mod)+"','"+std::to_string(fec)+"','"+std::to_string(rolloff)+"','"+std::to_string(pilots)+"','"+std::to_string(spectrum)+"','"+std::to_string(lo_frequency)+"','1') ON DUPLICATE KEY UPDATE lnb_id = '"+std::to_string(lnb_id)+"', dvb_standard = '"+std::to_string(dvb_standard)+"',frequency = '"+std::to_string(frequency)+"', symbol_rate = '"+std::to_string(symbol_rate)+"', modulation = '"+std::to_string(mod)+"', fec = '"+std::to_string(fec)+"', rolloff = '"+std::to_string(rolloff)+"', pilots = '"+std::to_string(pilots)+"', spectrum = '"+std::to_string(spectrum)+"', lo_frequency = '"+std::to_string(lo_frequency)+"', is_enable = '1';";
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);	
	}
	int dbHandler :: addConfAllegro(int mxl_id, int address,int enable1,int volt1,int enable2,int volt2){
		string query = "Insert into config_allegro (mxl_id,address,enable1,volt1,enable2,volt2) VALUES ('"+std::to_string(mxl_id)+"','"+std::to_string(address)+"','"+std::to_string(enable1)+"','"+std::to_string(volt1)+"','"+std::to_string(enable2)+"','"+std::to_string(volt2)+"') ON DUPLICATE KEY UPDATE enable1 = '"+std::to_string(enable1)+"', volt1 = '"+std::to_string(volt1)+"', enable2 = '"+std::to_string(enable2)+"', volt2 = '"+std::to_string(volt2)+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);	
	}
	int dbHandler :: addRFauthorization(int rmx_no, int enable){
		string query = "Insert into rf_authorization (rmx_no,enable) VALUES ('"+std::to_string(rmx_no)+"','"+std::to_string(enable)+"') ON DUPLICATE KEY UPDATE enable = '"+std::to_string(enable)+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);	
	}

	int dbHandler :: addIPOutputChannels(int rmx_no, int out_channel, std::string ip_address,int port){
			string  query = "Insert into ip_output_channels (rmx_no,output_channel,ip_address,port,is_enable) VALUES ('"+std::to_string(rmx_no)+"','"+std::to_string(out_channel)+"','"+ip_address+"','"+std::to_string(port)+"',1) ON DUPLICATE KEY UPDATE ip_address = '"+ip_address+"',port = '"+std::to_string(port)+"' , is_enable = 1;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler ::addIPInputChannels(int rmx_no, int input_channel, std::string ip_address,int port,int type){
		string query = "UPDATE tuner_details SET is_enable = 0 WHERE rmx_no = '"+std::to_string(rmx_no)+"' AND demod_id = '"+std::to_string(input_channel-1)+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());

		query = "Insert into ip_input_channels (rmx_no,input_channel,ip_address,port,type) VALUES ('"+std::to_string(rmx_no)+"','"+std::to_string(input_channel)+"','"+ip_address+"','"+std::to_string(port)+"','"+std::to_string(type)+"') ON DUPLICATE KEY UPDATE ip_address = '"+ip_address+"',port = '"+std::to_string(port)+"' ,type = '"+std::to_string(type)+"',is_enable = 1;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	 int dbHandler ::addSPTSIPInputChannels(std::string mux_id, std::string input_channel, std::string ip_address,std::string port,std::string type){
		string query = "UPDATE mux_15_1 SET is_enable = 1 WHERE mux_id = '"+mux_id+"';";  
		std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		query = "Insert into spts_ip_inputs (mux_id,channel_no,ip_address,port,type) VALUES ('"+mux_id+"','"+input_channel+"','"+ip_address+"','"+port+"','"+type+"') ON DUPLICATE KEY UPDATE ip_address = '"+ip_address+"',port = '"+port+"' ,type = '"+type+"',is_enable = 1;";  
		mysql_query (connect,query.c_str());
		// std::cout<<query<<std::endl;
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: addTunerChannelType(int rmx_no,int channel_no,int type){
		string query = "UPDATE input SET tuner_type = '"+std::to_string(type)+"' WHERE rmx_id = '"+std::to_string(rmx_no)+"' AND input_channel = '"+std::to_string(channel_no)+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: removeIPOutputChannels(int rmx_no, int out_channel){
		string query = "UPDATE ip_output_channels SET is_enable = 0 WHERE rmx_no = '"+std::to_string(rmx_no)+"' AND output_channel = '"+std::to_string(out_channel)+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: removeSPTSIPInputChannels(std::string rmx_no,std::string channel_no){
		string query = "UPDATE spts_ip_inputs SET is_enable = 0 WHERE mux_id = '"+rmx_no+"' AND channel_no = '"+channel_no+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: removeIPInputChannels(int rmx_no, int input_channel){
		string query = "UPDATE ip_input_channels SET is_enable = 0 WHERE rmx_no = '"+std::to_string(rmx_no)+"' AND input_channel = '"+std::to_string(input_channel)+"';";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: addQAM(std::string rmx_no, std::string output_channel,std::string qam_id){
		string query = "UPDATE output SET qam_id = '"+qam_id+"' WHERE rmx_id = '"+rmx_no+"' AND output_channel = '"+output_channel+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}
	int dbHandler :: addCustomPid(std::string rmx_no, std::string output,std::string pid, int addFlag){
		string query = "";
		if(addFlag){
			query = "INSERT INTO `custom_pids` ( `rmx_no`, `output_channel`, `pid`) VALUES ('"+rmx_no+"', '"+output+"', '"+pid+"');";
		}else{
			query = "DELETE FROM custom_pids WHERE pid = '"+pid+"'AND rmx_no = '"+rmx_no+"' AND output_channel = '"+output+"';";  
		}
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);		
	}

	int dbHandler :: deleteLockedPrograms() 
	{
		string query = "delete from locked_programs;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: deleteHighPriorityServices() 
	{
		string query = "delete from highPriorityServices;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: deletefreeCAModePrograms() 
	{
		string query = "delete from freeCA_mode_programs;";  
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	
	int dbHandler :: addECMChannelSetup(int channel_id,std::string supercas_id ,std::string ip,int port) 
	{
		string query = "Insert into ecmg (channel_id,supercas_id,ip,port) VALUES ('"+std::to_string(channel_id)+"','"+supercas_id+"','"+ip+"','"+std::to_string(port)+"') ON DUPLICATE KEY UPDATE supercas_id = '"+supercas_id+"', ip = '"+ip+"', port = '"+std::to_string(port)+"';";  
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: updateECMChannelSetup(int channel_id,std::string supercas_id ,std::string ip,int port,int old_channel_id) 
	{
		if(channel_id != old_channel_id){
			string query = "UPDATE ecm_stream SET channel_id = '"+std::to_string(channel_id)+"' WHERE channel_id = '"+std::to_string(old_channel_id)+"';";  
			mysql_query (connect,query.c_str());
		}
		string query = "UPDATE ecmg SET channel_id = '"+std::to_string(channel_id)+"', supercas_id = '"+supercas_id+"', ip = '"+ip+"', port = '"+std::to_string(port)+"' WHERE channel_id = '"+std::to_string(old_channel_id)+"';";
		// std::cout<<mysql_affected_rows(connect)<<std::endl;  
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: addECMStreamSetup(int stream_id,int ecm_id,int channel_id,std::string access_criteria,int cp_number,std::string ecm_pid,std::string timestamp) 
	{
		string query = "Insert into ecm_stream (stream_id,ecm_id,channel_id,access_criteria,cp_number,ecm_pid,timestamp) VALUES ('"+std::to_string(stream_id)+"','"+std::to_string(ecm_id)+"','"+std::to_string(channel_id)+"','"+access_criteria+"','"+std::to_string(cp_number)+"','"+ecm_pid+"','"+timestamp+"') ON DUPLICATE KEY UPDATE stream_id = '"+std::to_string(stream_id)+"',ecm_id = '"+std::to_string(ecm_id)+"',channel_id = '"+std::to_string(channel_id)+"', access_criteria = '"+access_criteria+"', cp_number = '"+std::to_string(cp_number)+"', ecm_pid = '"+ecm_pid+"', timestamp = '"+timestamp+"';";  
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: updateECMStreamSetup(int stream_id,int ecm_id,int channel_id,std::string access_criteria,int cp_number,std::string ecm_pid,std::string timestamp) 
	{
		string query = "UPDATE ecm_stream SET ecm_id = '"+std::to_string(ecm_id)+"', access_criteria = '"+access_criteria+"', cp_number = '"+std::to_string(cp_number)+"', timestamp = '"+timestamp+"' WHERE channel_id = '"+std::to_string(channel_id)+"' AND stream_id = '"+std::to_string(stream_id)+"' AND ecm_pid = '"+ecm_pid+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: addEMMchannelSetup(int channel_id, std::string client_id ,int data_id,int bw,int port,int stream_id,int emm_pid) 
	{
		string query = "Insert into emmg(channel_id,client_id,data_id,bw,port,stream_id,emm_pid) VALUES ('"+std::to_string(channel_id)+"','"+client_id+"','"+std::to_string(data_id)+"','"+std::to_string(bw)+"','"+std::to_string(port)+"','"+std::to_string(stream_id)+"','"+std::to_string(emm_pid)+"') ON DUPLICATE KEY UPDATE client_id = '"+client_id+"', data_id = '"+std::to_string(data_id)+"', port = '"+std::to_string(port)+"', bw = '"+std::to_string(bw)+"', stream_id = '"+std::to_string(stream_id)+"', emm_pid = '"+std::to_string(emm_pid)+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: enableEMM(std::string channel_id,std::string rmx_no,std::string output,int addFlag){
		string query = "";
		if(addFlag){
			query = "Insert into emmg_descriptor(channel_id,rmx_no,output) VALUES ('"+channel_id+"','"+rmx_no+"','"+output+"') ON DUPLICATE KEY UPDATE is_enable = 1;";  
		}
		else{
			query = "UPDATE emmg_descriptor SET is_enable = 0 WHERE channel_id = '"+channel_id+"';";  
		}
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: enableECM(std::string channel_id,std::string stream_id,std::string service_pid,std::string programNumber, std::string rmx_no,std::string output,std::string input,int addFlag){
		string query = "";
		if(addFlag){
			query = "Insert into ecm_descriptor(channel_id,stream_id,service_pid,service_no,rmx_id,output,input) VALUES ('"+channel_id+"','"+stream_id+"','"+service_pid+"','"+programNumber+"','"+rmx_no+"','"+output+"','"+input+"') ON DUPLICATE KEY UPDATE service_pid = '"+service_pid+"';";  
		}
		else{
			query = "DELETE FROM ecm_descriptor WHERE channel_id = '"+channel_id+"' AND stream_id = '"+stream_id+"' AND service_no = '"+programNumber+"' AND rmx_id = '"+rmx_no+"' AND output = '"+output+"' AND output = '"+input+"';";  
		}
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);

	}
	int dbHandler :: updateEMMchannelSetup(int channel_id, std::string client_id ,int data_id,int bw,int port,int stream_id,std::string emm_pid) 
	{
		string query = "UPDATE emmg SET client_id = '"+client_id+"', data_id = '"+std::to_string(data_id)+"', port = '"+std::to_string(port)+"', bw = '"+std::to_string(bw)+"', stream_id = '"+std::to_string(stream_id)+"', emm_pid = '"+emm_pid+"' WHERE channel_id = '"+std::to_string(channel_id)+"';";  
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	
	int dbHandler :: scrambleService(int channel_id,int stream_id,int service_id){
		string query = "Insert into stream_service_map (channel_id,stream_id,service_id) VALUES ('"+std::to_string(channel_id)+"','"+std::to_string(stream_id)+"','"+std::to_string(service_id)+"');";  
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	
	int dbHandler :: deScrambleService(int channel_id,int stream_id,int service_id){
		string query = "DELETE FROM stream_service_map WHERE channel_id ='"+std::to_string(channel_id)+"' AND stream_id ='"+std::to_string(stream_id)+"' AND service_id ='"+std::to_string(service_id)+"';";  
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: deleteECM(int channel_id) 
	{
		//string query = "UPDATE ecm_stream set is_enable = 0 where channel_id = '"+std::to_string(channel_id)+"';";
		string query = "DELETE FROM ecm_stream where channel_id = '"+std::to_string(channel_id)+"';";
		mysql_query (connect,query.c_str());
		query = "DELETE FROM ecmg where channel_id = '"+std::to_string(channel_id)+"';";
		mysql_query (connect,query.c_str());
	 	return mysql_affected_rows(connect);
	}
	int dbHandler :: isECMExists(int channel_id)
	{
		string query = "select count(*) from ecmg where channel_id = '"+std::to_string(channel_id)+"' AND is_enable = 1;";
		mysql_query (connect,query.c_str());
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}
	int dbHandler :: deleteEMM(int channel_id) 
	{
		string query = "DELETE FROM emmg where channel_id = '"+std::to_string(channel_id)+"';";
		mysql_query (connect,query.c_str());
		return mysql_affected_rows(connect);
	}
	int dbHandler :: isEMMExists(int channel_id)
	{
		string query = "select count(*) from emmg where channel_id = '"+std::to_string(channel_id)+"' AND is_enable = 1;";
		mysql_query (connect,query.c_str());
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}
	int dbHandler :: deleteECMStream(int channel_id,int stream_id)
	{
		// string query = "UPDATE ecm_stream set is_enable = 0 where channel_id = '"+std::to_string(channel_id)+"' AND stream_id = '"+std::to_string(stream_id)+"';";
		string query = "Delete From ecm_stream where channel_id = '"+std::to_string(channel_id)+"' AND stream_id = '"+std::to_string(stream_id)+"';";
		// std::cout<<query<<std::endl;
		int resp = mysql_query (connect,query.c_str());
 		return mysql_affected_rows(connect);
	}
	int dbHandler :: isECMStreamExists(int channel_id,int stream_id)
	{
		string query = "select count(*) from ecm_stream where channel_id = '"+std::to_string(channel_id)+"' AND stream_id = '"+std::to_string(stream_id)+"';";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}
	int dbHandler :: isStreamAlreadUsed(int channel_id,int stream_id)
	{
		string query = "select count(*) from stream_service_map where channel_id = '"+std::to_string(channel_id)+"' AND stream_id = '"+std::to_string(stream_id)+"';";
		mysql_query (connect,query.c_str());
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}
	Json::Value dbHandler :: getNewProviderName(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT DISTINCT s.service_number, s.provider_name,c.input_channel,s.rmx_id FROM service_providers s,channel_list c WHERE c.channel_number = s.service_number AND s.rmx_id = c.rmx_id group by s.service_number,s.rmx_id;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["service_number"]=row[i];
					jsonObj["service_name"]=row[i+1];
					jsonObj["input_channel"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getPsiSiInterval(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT output,pat_int,sdt_int,nit_int,rmx_id FROM psi_si_interval;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["pat_int"]=row[i+1];
					jsonObj["sdt_int"]=row[i+2];
					jsonObj["nit_int"]=row[i+3];
					jsonObj["rmx_no"]=row[i+4];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getDVBspiOutputMode(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT output,bit_rate,falling,mode,rmx_id FROM dvb_spi_output;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["bit_rate"]=row[i+1];
					jsonObj["falling"]=row[i+2];
					jsonObj["mode"]=row[i+3];
					jsonObj["rmx_no"]=row[i+4];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getNITtableTimeout(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT(table_id),timeout,rmx_id FROM NIT_table_timeout;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["table_id"]=row[i];
					jsonObj["timeout"]=row[i+1];
					jsonObj["rmx_no"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	int dbHandler :: getServiceInputChannel(std::string service_no)
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		int input_channel=-1;
		std::string query="select input_channel from channel_list where channel_number='"+service_no+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					input_channel=std::stoi(row[i]);
				}
			}
		}
	 	return input_channel;
	}
	Json::Value  dbHandler :: getNITmode() 
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		int nit_mode=-1;
		Json::Value jsonList;
		std::string query="select output_channel,nit_mode,rmx_id from output;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					// nit_mode=std::stoi(row[i]);
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["mode"]=row[i+1];
					jsonObj["rmx_no"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler ::  getTablesVersions(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,output,rmx_id FROM table_versions;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["pat_ver"]=row[i];
					jsonObj["pat_isenable"]=row[i+1];
					jsonObj["sdt_ver"]=row[i+2];
					jsonObj["sdt_isenable"]=row[i+3];
					jsonObj["nit_ver"]=row[i+4];
					jsonObj["nit_isenable"]=row[i+5];
					jsonObj["output"]=row[i+6];
					jsonObj["rmx_no"]=row[i+7];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getAlarmFlags(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT level1,level2,level3,level4,mode,rmx_id FROM create_alarm_flags;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["level1"]=row[i];
					jsonObj["level2"]=row[i+1];
					jsonObj["level3"]=row[i+2];
					jsonObj["level4"]=row[i+3];
					jsonObj["mode"]=row[i+4];
					jsonObj["rmx_no"]=row[i+5];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value  dbHandler :: getLcnProviderId() 
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		int provider_id=-1;
		Json::Value jsonObj;
		std::string query="select provider_id,rmx_id from lcn_provider_id;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		jsonObj["error"] = true;
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonObj["provider_id"] = row[i];
					jsonObj["rmx_no"] = row[i+1];
					jsonObj["error"] = false;
					// provider_id=std::stoi();
				}
			}
		}
	 	return jsonObj;
	}
	Json::Value dbHandler :: getActivePrograms(std::string program_numbers,std::string input,std::string output,std::string rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT a.channel_num FROM active_service_list a,channel_list c WHERE c.channel_number = a.channel_num AND a.in_channel = '"+input+"' AND a.out_channel = '"+output+"' AND a.rmx_id = '"+rmx_no+"' AND a.channel_num NOT IN ("+program_numbers+");";

		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonList.append(row[i]);
					// jsonList.append(jsonObj);
				}
				// jsonList["program_number"]=jsonList;
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
		
	 	return jsonArray;
	}
	Json::Value dbHandler :: getEncryptedPrograms(std::string program_numbers,std::string input,std::string output,std::string rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		// query="SELECT DISTINCT a.channel_num,a.key_index FROM encrypted_service_list a,channel_list c WHERE c.channel_number = a.channel_num AND a.in_channel = '"+input+"' AND a.out_channel = '"+output+"' AND a.rmx_id = '"+rmx_no+"' AND a.channel_num NOT IN ("+program_numbers+");";
		query="SELECT DISTINCT a.channel_num,a.key_index FROM encrypted_service_list a,channel_list c WHERE c.channel_number = a.channel_num AND a.in_channel = '"+input+"' AND a.rmx_id = '"+rmx_no+"' AND a.channel_num NOT IN ("+program_numbers+");";
		std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["service_no"] = row[i];
					jsonObj["key_index"] = row[i+1];
					jsonList.append(jsonObj);
					std::cout<<jsonObj<<endl;
				}
				// jsonList["program_number"]=jsonList;
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
		// std::cout<<query;
	 	return jsonArray;
	}
	Json::Value dbHandler :: getActivePrograms(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT a.channel_num,a.in_channel,a.out_channel,a.rmx_id FROM active_service_list a,channel_list c WHERE c.channel_number = a.channel_num AND a.rmx_id = c.rmx_id group by a.in_channel,a.out_channel;";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["input_channel"]=row[i+1];
					jsonObj["output_channel"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getActivePrograms(std::string output ,std::string rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT a.channel_num,in_channel FROM active_service_list a WHERE a.rmx_id = '"+rmx_no+"' AND a.out_channel = '"+output+"';";
		// std::cout<<query<<endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["service_id"]=row[i];
					jsonObj["input"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	int dbHandler :: getLCNNumber(std::string input ,std::string rmx_no ,std::string service_id){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		int lcn_id = 0;
		query="SELECT lcn_id FROM channel_list WHERE rmx_id = '"+rmx_no+"' AND input_channel = '"+input+"' AND channel_number = '"+service_id+"';";
		// std::cout<<query<<endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					std::string lcn = row[i]; 
					lcn_id = std::stoi(lcn);
				}
			}
		}
	 	return lcn_id;
	}
	Json::Value dbHandler :: getCenterFrequency()
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList,jsonArray;
		string query = "select ifrequency,rmx_id from Ifrequency;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			jsonArray["error"] = false;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				Json::Value jsonObj;
				jsonObj["center_frequency"]=row[i];
				jsonObj["rmx_no"]=row[i+1];
				jsonList.append(jsonObj);
			}
			jsonArray["list"] = jsonList;
	 	}else{
	 		jsonArray["true"] = true;
	 		jsonArray["message"] = "No Records";
	 	}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getInputMode(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT input,is_spts,pmt,sid,rise,master,in_select,bitrate,input_mode.rmx_id FROM input_mode,input where input_channel = input AND input_mode.rmx_id = input.rmx_id;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["input"]=row[i];
					jsonObj["is_spts"]=row[i+1];
					jsonObj["pmt"]=row[i+2];
					jsonObj["sid"]=row[i+3];
					jsonObj["rise"]=row[i+4];
					jsonObj["master"]=row[i+5];
					jsonObj["in_select"]=row[i+6];
					jsonObj["bitrate"]=row[i+7];
					jsonObj["rmx_no"]=row[i+8];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getNetworkDetails(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT output,ts_id,network_id,original_netw_id,network_name,rmx_id FROM network_details;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["ts_id"]=row[i+1];
					jsonObj["network_id"]=row[i+2];
					jsonObj["original_netw_id"]=row[i+3];
					jsonObj["network_name"]=row[i+4];
					jsonObj["rmx_no"]=row[i+5];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	
	Json::Value dbHandler :: getNetworkDetailsForNIT(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		
		std::string query="SELECT n.output,n.ts_id,n.network_id,n.original_netw_id,n.network_name,n.rmx_id,o.qam_id,o.frequency,o.symbol_rate FROM network_details n,output o,(SELECT DISTINCT `out_channel`,`rmx_id` FROM `active_service_list`) alist WHERE n.output = output_channel AND n.rmx_id = o.rmx_id AND alist.out_channel = o.output_channel AND alist.rmx_id = o.rmx_id ORDER BY o.frequency";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["ts_id"]=row[i+1];
					jsonObj["network_id"]=row[i+2];
					jsonObj["original_netw_id"]=row[i+3];
					jsonObj["network_name"]=row[i+4];
					jsonObj["rmx_no"]=row[i+5];
					jsonObj["modulation"]=row[i+6];
					jsonObj["frequency"]=row[i+7];
					jsonObj["symbol_rate"]=row[i+8];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
		
	}
	Json::Value dbHandler :: getFreeCAModePrograms(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT f.program_number,f.input_channel,f.output_channel,f.rmx_id FROM freeCA_mode_programs f,channel_list c WHERE c.channel_number = f.program_number AND f.rmx_id = c.rmx_id group by f.program_number;";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["input_channel"]=row[i+1];
					jsonObj["output_channel"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getFreeCAModePrograms(std::string program_number,std::string input,std::string output){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;

		query="SELECT DISTINCT f.program_number,f.input_channel,f.output_channel FROM freeCA_mode_programs f,channel_list c WHERE c.channel_number = f.program_number AND f.input_channel = '"+input+"' AND f.output_channel = '"+output+"' AND f.program_number != '"+program_number+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getHighPriorityServices(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT h.program_number,h.input,h.rmx_id FROM highPriorityServices h,channel_list c WHERE c.channel_number = h.program_number AND h.rmx_id = c.rmx_id group by h.program_number;";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["input_channel"]=row[i+1];
					jsonObj["rmx_no"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getHighPriorityServices(std::string input,std::string program_number ){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;

		query="SELECT DISTINCT h.program_number,h.input FROM highPriorityServices h,channel_list c WHERE c.channel_number = h.program_number AND h.input = '"+input+"' AND h.program_number != '"+program_number+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getLockedPids(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT l.program_number,l.input,l.rmx_id FROM locked_programs l,channel_list c WHERE c.channel_number = l.program_number AND l.rmx_id = c.rmx_id group by l.program_number;";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["input_channel"]=row[i+1];
					jsonObj["rmx_no"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getLockedPids(std::string input,std::string program_number ){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;

		query="SELECT DISTINCT l.program_number,l.input FROM locked_programs l,channel_list c WHERE c.channel_number = l.program_number AND l.input = '"+input+"' AND l.program_number != '"+program_number+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getLcnNumbers(std::string input,std::string program_number ){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;

		query="SELECT DISTINCT channel_number, lcn_id FROM channel_list WHERE input_channel = '"+input+"' AND channel_number != '"+program_number+"' AND lcn_id != '-1';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["channel_number"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	
	
	
	Json::Value dbHandler :: getLcnNumbers(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT `channel_number`,`lcn_id`,`input_channel`,`rmx_id` FROM channel_list WHERE lcn_id != '-1';";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["channel_number"]=row[i+1];
					jsonObj["input_channel"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}

	Json::Value dbHandler :: getPmtalarm(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT DISTINCT p.program_number,p.input,p.alarm_enable,p.rmx_id FROM pmt_alarms p,channel_list c WHERE c.channel_number = p.program_number AND p.rmx_id = c.rmx_id group by p.program_number;";
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["input"]=row[i+1];
					jsonObj["alarm_enable"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getPmtalarm(std::string input,std::string program_number ){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;

		query="SELECT DISTINCT p.program_number,p.input,p.alarm_enable FROM pmt_alarms p,channel_list c WHERE c.channel_number = p.program_number AND p.input = '"+input+"' AND p.program_number != '"+program_number+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["program_number"]=row[i];
					jsonObj["alarm_enable"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getServiceNewnames(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT DISTINCT n.channel_number, n.channel_name,c.input_channel,n.rmx_id FROM new_service_namelist n,channel_list c WHERE c.channel_number = n.channel_number AND channel_name !=-1 AND n.rmx_id = c.rmx_id;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["channel_number"]=row[i];
					jsonObj["channel_name"]=row[i+1];
					jsonObj["input_channel"]=row[i+2];
					jsonObj["rmx_no"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getServiceIds(int rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT DISTINCT n.channel_number, n.service_id FROM new_service_namelist n,channel_list c WHERE c.channel_number = n.channel_number AND service_id !=-1 AND n.rmx_id = c.rmx_id AND n.rmx_id = '"+std::to_string(rmx_no)+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["channel_number"]=row[i];
					jsonObj["service_id"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getServiceIds(int rmx_no,std::string old_service_id){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query="SELECT DISTINCT n.channel_number, n.service_id FROM new_service_namelist n,channel_list c WHERE c.channel_number = n.channel_number AND service_id !=-1 AND n.rmx_id = c.rmx_id AND n.rmx_id = '"+std::to_string(rmx_no)+"' AND n.channel_number !='"+old_service_id+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["channel_number"]=row[i];
					jsonObj["service_id"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getStreams() 
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		mysql_query (connect,"select stream_id,channel_id,ecm_id,access_criteria,cp_number,timestamp,ecm_pid from ecm_stream;");
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["stream_id"]=row[i];
					jsonObj["channel_id"]=row[i+1];
					jsonObj["ecm_id"]=row[i+2];
					jsonObj["access_criteria"]=row[i+3];
					jsonObj["cp_number"]=row[i+4];
					jsonObj["timestamp"]=row[i+5];
					jsonObj["ecm_pid"]=row[i+6];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getScrambledServices()
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		mysql_query (connect,"SELECT s.stream_id as stream_id,s.channel_id as channel_id, ecm_id, access_criteria, cp_number, timestamp FROM `stream_service_map` sm,ecm_stream s  WHERE sm.channel_id = s.channel_id AND sm.stream_id = s.stream_id;");
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["stream_id"]=row[i];
					jsonObj["channel_id"]=row[i+1];
					jsonObj["ecm_id"]=row[i+2];
					jsonObj["access_criteria"]=row[i+3];
					jsonObj["cp_number"]=row[i+4];
					jsonObj["timestamp"]=row[i+5];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}

	Json::Value dbHandler :: getChannels() 
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		mysql_query (connect,"select * from ecmg;");
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		unsigned int numrows = mysql_num_rows(res_set);
		if(numrows>0)
		{
			jsonArray["error"] = false;
			jsonArray["message"] = "Records found!";
			Json::Value jsonList;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				Json::Value jsonObj;
				jsonObj["channel_id"]=row[i];
				jsonObj["supercas_id"]=row[i+1];
				jsonObj["ip"]=row[i+2];
				jsonObj["port"]=row[i+3];
				jsonObj["is_enable"]=row[i+4];
				jsonList.append(jsonObj);
			}
			jsonArray["list"]=jsonList;
		}else{
			jsonArray["error"] = true;
			jsonArray["message"] = "No records found!";
		}
		//
	 	return jsonArray;
	}
	Json::Value dbHandler :: getEMMChannels()
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		mysql_query (connect,"select channel_id,client_id,data_id,bw,port,stream_id,is_enable from emmg;");
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		unsigned int numrows = mysql_num_rows(res_set);
		if(numrows>0)
		{
			jsonArray["error"] = false;
			jsonArray["message"] = "Records found!";
			Json::Value jsonList;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				Json::Value jsonObj;
				jsonObj["channel_id"]=row[i];
				jsonObj["client_id"]=row[i+1];
				jsonObj["data_id"]=row[i+2];
				jsonObj["bw"]=row[i+3];
				jsonObj["port"]=row[i+4];
				jsonObj["stream_id"]=row[i+5];
				jsonObj["is_enable"]=row[i+6];
				jsonList.append(jsonObj);
			}
			jsonArray["list"]=jsonList;
		}else{
			jsonArray["error"] = true;
			jsonArray["message"] = "No records found!";
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getEMMGChannels(std::string rmx_no,std::string output,std::string channel_id){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonArray;
		std::string query="";
		if(channel_id != "-1"){
			query = "SELECT e.client_id,e.emm_pid FROM emmg e,emmg_descriptor ed WHERE e.channel_id = ed.channel_id AND ed.rmx_no = '"+rmx_no+"' AND ed.output ='"+output+"' AND ed.is_enable =1 AND e.is_enable =1 AND ed.output IN (SELECT output FROM emmg_descriptor WHERE channel_id = '"+channel_id+"');";
		}else{
			query = "SELECT e.client_id,e.emm_pid FROM emmg e,emmg_descriptor ed WHERE e.channel_id = ed.channel_id AND ed.rmx_no = '"+rmx_no+"' AND ed.output ='"+output+"' AND ed.is_enable =1 AND e.is_enable =1;";
		}
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		unsigned int numrows = mysql_num_rows(res_set);
		if(numrows>0)
		{
			jsonArray["error"] = false;
			jsonArray["message"] = "Records found!";
			Json::Value jsonList,ca_system_id,emm_pid;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				// Json::Value jsonObj;
				// jsonObj["ca_system_id"]=row[i];
				ca_system_id.append(row[i]);
				// jsonObj["emm_pid"]=row[i+1];
				emm_pid.append(row[i+1]);
				// jsonList.append(jsonObj);
			}
			jsonArray["ca_system_ids"]=ca_system_id;
			jsonArray["emm_pids"]=emm_pid;
		}else{
			jsonArray["error"] = true;
			jsonArray["message"] = "No records found!";
		}
	 	return jsonArray;
	}
	Json::Value dbHandler :: getECMDescriptors(std::string rmx_no,std::string input){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonArray;
		std::string query = "SELECT supercas_id,service_pid,ecm_pid FROM ecm_descriptor ed,ecmg e,ecm_stream es WHERE ed.channel_id = es.channel_id AND ed.stream_id = es.stream_id AND es.channel_id = e.channel_id AND ed.rmx_id = '"+rmx_no+"' AND ed.input = '"+input+"'";	
		
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		unsigned int numrows = mysql_num_rows(res_set);
		if(numrows>0)
		{
			jsonArray["error"] = false;
			jsonArray["message"] = "Records found!";
			Json::Value jsonList,ca_system_id,service_pid,ecm_pids;
			while (((row= mysql_fetch_row(res_set)) !=NULL ))
			{ 
				// Json::Value jsonObj;
				// jsonObj["ca_system_id"]=row[i];
				ca_system_id.append(row[i]);
				// jsonObj["emm_pid"]=row[i+1];
				service_pid.append(row[i+1]);
				// jsonList.append(jsonObj);
				ecm_pids.append(row[i+2]);
			}
			jsonArray["ca_system_ids"]=ca_system_id;
			jsonArray["service_pids"]=service_pid;
			jsonArray["ecm_pids"]=ecm_pids;
		}else{
			jsonArray["error"] = true;
			jsonArray["message"] = "No records found!";
		}
	 	return jsonArray;
	}
	
	// Json::Value dbHandler :: getEMMGChannels(std::string channel_id ){
	// 	MYSQL_RES *res_set;
	// 	MYSQL_ROW row;
	// 	Json::Value jsonArray;
		
	// 	mysql_query (connect,("SELECT ed.rmx_no,ed.output, e.client_id,e.emm_pid FROM emmg e,emmg_descriptor ed WHERE e.channel_id = ed.channel_id AND ed.is_enable =1 AND e.is_enable =1 AND ed.output IN (SELECT output FROM emmg_descriptor WHERE channel_id = '"+channel_id+"');").c_str());
	// 	unsigned int i =0;
	// 	res_set = mysql_store_result(connect);
	// 	unsigned int numrows = mysql_num_rows(res_set);
	// 	if(numrows>0)
	// 	{
	// 		jsonArray["error"] = false;
	// 		jsonArray["message"] = "Records found!";
	// 		Json::Value jsonList;
	// 		while (((row= mysql_fetch_row(res_set)) !=NULL ))
	// 		{ 
	// 			Json::Value jsonObj;
	// 			jsonObj["rmx_no"]=row[i];
	// 			jsonObj["output"]=row[i+1];
	// 			jsonObj["ca_system_id"]=row[i+2];
	// 			jsonObj["emm_pid"]=row[i+3];
	// 			jsonList.append(jsonObj);
	// 		}
	// 		jsonArray["list"]=jsonList;
	// 	}else{
	// 		jsonArray["error"] = true;
	// 		jsonArray["message"] = "No records found!";
	// 	}
	//  	return jsonArray;
	// }
	
	Json::Value dbHandler :: getConfAllegro(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT mxl_id,address,enable1,volt1,enable2,volt2 FROM config_allegro;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["mxl_id"]=row[i];
					jsonObj["address"]=row[i+1];
					jsonObj["enable1"]=row[i+2];
					jsonObj["volt1"]=row[i+3];
					jsonObj["enable2"]=row[i+4];
					jsonObj["volt2"]=row[i+5];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getConfAllegro(int mxl_id, int address){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonArray;

		std::string query="SELECT enable1,volt1,enable2,volt2 FROM config_allegro WHERE mxl_id = '"+std::to_string(mxl_id)+"' AND address = '"+std::to_string(address)+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonArray["enable1"]=row[i];
					jsonArray["volt1"]=row[i+1];
					jsonArray["enable2"]=row[i+2];
					jsonArray["volt2"]=row[i+3];
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
		}else{ 		
			jsonArray["error"] = true;
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getRFTunerDetails(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT mxl_id,rmx_no,demod_id,lnb_id,dvb_standard,frequency,symbol_rate,modulation,fec,rolloff,pilots,spectrum,lo_frequency FROM tuner_details t ,input i where t.rmx_no = i.rmx_id AND i.input_channel = t.demod_id AND is_enable = 1 AND i.tuner_type = 0;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["mxl_id"]=row[i];
					jsonObj["rmx_no"]=row[i+1];
					jsonObj["demod_id"]=row[i+2];
					jsonObj["lnb_id"]=row[i+3];
					jsonObj["dvb_standard"]=row[i+4];
					jsonObj["frequency"]=row[i+5];
					jsonObj["symbol_rate"]=row[i+6];
					jsonObj["modulation"]=row[i+7];
					jsonObj["fec"]=row[i+8];
					jsonObj["rolloff"]=row[i+9];
					jsonObj["pilots"]=row[i+10];
					jsonObj["spectrum"]=row[i+11];
					jsonObj["lo_frequency"]=row[i+12];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
Json::Value dbHandler :: getRFauthorizedRmx(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT rmx_no FROM rf_authorization WHERE enable = 1;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonList.append(row[i]);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
Json::Value dbHandler :: getIPOutputChannels(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT rmx_no,output_channel,ip_address,port FROM ip_output_channels WHERE is_enable = 1;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["rmx_no"]=row[i];
					jsonObj["output_channel"]=row[i+1];
					jsonObj["ip_address"]=row[i+2];
					jsonObj["port"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
int dbHandler :: getTunerChannelType(int control_fpga,int rmx_no,int channel_no,int type){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		int value = 0;
		int rmx_no1,rmx_no2;
		addTunerChannelType(rmx_no,channel_no,type);
		(control_fpga == 1)? (rmx_no1 = 1,rmx_no2 = 2) : ((control_fpga == 2)? (rmx_no1 = 3,rmx_no2 = 4) : (rmx_no1 = 5,rmx_no2 = 6));
		std::string query="(SELECT  tuner_type FROM  `input` WHERE  `rmx_id` =  '"+std::to_string(rmx_no1)+"' ) UNION ALL (SELECT tuner_type FROM  `input` WHERE  `rmx_id` = '"+std::to_string(rmx_no2)+"');";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		// std::cout<<query<<std::endl;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					int tuner_value =atoi(row[0]);
					value = value | (tuner_value&0x1)<<i; 
					i++;
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return value;
	}
int dbHandler :: getTunerChannelType(int control_fpga){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		int value = 0;
		int rmx_no1,rmx_no2;
		(control_fpga == 1)? (rmx_no1 = 1,rmx_no2 = 2) : ((control_fpga == 2)? (rmx_no1 = 3,rmx_no2 = 4) : (rmx_no1 = 5,rmx_no2 = 6));
		std::string query="(SELECT  tuner_type FROM  `input` WHERE  `rmx_id` =  '"+std::to_string(rmx_no1)+"' ) UNION ALL (SELECT tuner_type FROM  `input` WHERE  `rmx_id` = '"+std::to_string(rmx_no2)+"');";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		// std::cout<<query<<std::endl;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					int tuner_value =atoi(row[0]);
					value = value | (tuner_value&0x1)<<i; 
					i++;
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return value;
	}
Json::Value dbHandler :: getIPTunerDetails(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT ip.rmx_no,ip.input_channel,ip.ip_address,ip.port,ip.type FROM ip_input_channels ip ,input i where ip.rmx_no = i.rmx_id AND i.input_channel = (ip.input_channel - 1) AND ip.is_enable = 1 AND i.tuner_type = 1;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["rmx_no"]=row[i];
					jsonObj["input_channel"]=row[i+1];
					jsonObj["ip_address"]=row[i+2];
					jsonObj["port"]=row[i+3];
					jsonObj["type"]=row[i+4];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getSPTSIPTuners(int mux_id){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT channel_no,ip_address,port,type FROM spts_ip_inputs where mux_id = '"+std::to_string(mux_id)+"' AND is_enable = 1;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["channel_no"]=row[i];
					jsonObj["ip_address"]=row[i+1];
					jsonObj["port"]=row[i+2];
					jsonObj["type"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getSPTSControl(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT mux_id FROM mux_15_1 where is_enable = 1;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonList.append(row[i]);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getQAM(int rmx_id){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT output_channel,qam_id FROM output where rmx_id = '"+std::to_string(rmx_id)+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output_channel"]=row[i];
					jsonObj["qam_id"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}
	Json::Value dbHandler :: getCustomPids(std::string rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT pid,output_auth FROM custom_pids where rmx_no = '"+rmx_no+"';";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				Json::Value json_pids,json_output_auth;
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					json_pids.append(row[i]);
					json_output_auth.append(row[i+1]);
				}
				jsonArray["pids"] = json_pids;
				jsonArray["output_auths"] = json_output_auth;
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}

	// Json::Value dbHandler :: getCustomPids(){
	// MYSQL_RES *res_set;
	// MYSQL_RES *res_set1;
	// MYSQL_RES *res_set2;
	// MYSQL_ROW row;
	// Json::Value jsonList;

	// std::string query="SELECT pid,output_auth FROM custom_pids where rmx_no = '"+rmx_no+"' AND output_channel = '"+output+"';";
	// mysql_query (connect,query.c_str());
	// unsigned int i =0;
	// res_set = mysql_store_result(connect);

	// query="SELECT pid,output_auth FROM custom_pids where rmx_no = '"+rmx_no+"' AND output_channel = '"+output+"';";
	// mysql_query (connect,query.c_str());
	// unsigned int i =0;
	// res_set1 = mysql_store_result(connect);

	// query="SELECT pid,output_auth FROM custom_pids where rmx_no = '"+rmx_no+"' AND output_channel = '"+output+"';";
	// mysql_query (connect,query.c_str());
	// unsigned int i =0;
	// res_set1 = mysql_store_result(connect);

	// if(res_set){
	// 	unsigned int numrows = mysql_num_rows(res_set);
	// 	if(numrows>0)
	// 	{
	// 		Json::Value json_pids,json_output_auth;
	// 		jsonArray["error"] = false;
	// 		while (((row= mysql_fetch_row(res_set)) !=NULL ))
	// 		{ 
	// 			json_pids.append(row[i]);
	// 			json_output_auth.append(row[i+1]);
	// 		}
	// 		jsonArray["pids"] = json_pids;
	// 		jsonArray["output_auths"] = json_output_auth;
	// 	}else{	
	// 		jsonArray["error"] = true;
	// 		jsonArray["message"] = "NO record found!";
	// 	}
	// }else{ 		
	// 	jsonArray["error"] = true;
	// 	jsonArray["message"] = "ERROR!";
	// }
	// return jsonArray;
	// }
	int dbHandler :: generateECMPID(std::string rmx_no,std::string output,std::string channel_id,std::string stream_id,std::string programNumber){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		std::string ecm_pid;

		query="SELECT ecm_pid FROM ecm_descriptor WHERE rmx_id = '"+rmx_no+"' AND output = '"+output+"' AND channel_id = '"+channel_id+"' AND stream_id = '"+stream_id+"' AND service_no = '"+programNumber+"';";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					ecm_pid=row[i];
				}
			}else{	
				ecm_pid = "-1";
			}
		}else{ 		
			ecm_pid = "-1";
		}
	 	
	 	if(ecm_pid != "-1"){
	 		return std::stoi(ecm_pid);
	 	}else{
	 		
	 		return 1001;
	 	}
	}
	unsigned long int dbHandler :: getScramblingIndex(std::string rmx_no,std::string output){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		std::string indexValue;

		query="SELECT indexValue FROM output WHERE rmx_id = '"+rmx_no+"' AND output_channel = '"+output+"';";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					indexValue=row[i];
				}
			}else{	
				indexValue = "-1";
			}
		}else{ 		
			indexValue = "-1";
		}
	 	
	 	if(indexValue != "-1"){
	 		return std::stoul(indexValue);
	 	}else{
	 		
	 		return 0;
	 	}
	}
	int dbHandler :: updateCWIndex(std::string rmx_no,std::string output,std::string programNumber,long int index,int indexValue,int addFlag){
		std::string query;
		unsigned int indx_value;
		if(std::stoi(rmx_no)%2 == 0)
    		indx_value = setIndexSetUnset(index-128,indexValue,addFlag);
    	else
    		indx_value = setIndexSetUnset(index,indexValue,addFlag);
		query="UPDATE output SET indexValue = '"+std::to_string(indx_value)+"' WHERE rmx_id = '"+rmx_no+"' AND output_channel = '"+output+"';";
		std::cout<<query<<endl;
		mysql_query (connect,query.c_str());
	    return mysql_affected_rows(connect);	    	
	}

	int dbHandler :: setIndexSetUnset(int index, int indexValue,int flag){
		if(flag)
			return  indexValue | 1UL << index;
		else
			return indexValue & (~(1UL << index));
	}
	long int dbHandler :: getCWKeyIndex(std::string rmx_no,std::string input,std::string output,std::string programNumber){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		long int cw_index;

		query="SELECT key_index FROM encrypted_service_list WHERE rmx_id = '"+rmx_no+"' AND in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num = '"+programNumber+"';";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					cw_index=std::stoul(row[i]);
				}
				return cw_index;
			}else{	
				return -1;
			}
		}else{ 		
			return -1;
		}
	}
	int dbHandler :: addEncryptedService(std::string rmx_no,std::string input,std::string output,std::string programNumber,long int key_index, int includeflag){
			std::string query="";
			if(includeflag){
				query = "Insert into encrypted_service_list (in_channel,out_channel,channel_num,rmx_id,key_index) VALUES ('"+input+"','"+output+"','"+programNumber+"','"+rmx_no+"','"+std::to_string(key_index)+"')  ON DUPLICATE KEY UPDATE channel_num = channel_num;";  	
			}
			else{
				query = "DELETE FROM encrypted_service_list WHERE in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num = '"+ programNumber+"' AND rmx_id = '"+rmx_no+"';";  
			}
			// std::cout<<"addActivatedPrograms--------------------------\n"<<query;
			std::cout<<"\n"<<query;
			mysql_query (connect,query.c_str());
			return mysql_affected_rows(connect);
		
	}
	int dbHandler :: isServiceEncrypted(std::string programNumber,std::string rmx_no,std::string output,std::string input){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		long int cw_index;

		query="SELECT COUNT(*) FROM encrypted_service_list WHERE rmx_id = '"+rmx_no+"' AND in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num = '"+programNumber+"';";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					cw_index=std::stoul(row[i]);
				}
				return cw_index;
			}else{	
				return -1;
			}
		}else{ 		
			return -1;
		}
	}
Json::Value dbHandler :: getEnabledEMCStreams(std::string channel_id,std::string stream_id){
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	Json::Value jsonArray;
	std::string query = "SELECT supercas_id,service_pid,ecm_pid,ed.rmx_id,ed.output,es.access_criteria,ed.output FROM ecm_descriptor ed,ecmg e,ecm_stream es WHERE ed.channel_id = es.channel_id AND ed.stream_id = es.stream_id AND es.channel_id = e.channel_id AND ed.stream_id = '"+stream_id+"' AND ed.channel_id = '"+channel_id+"'";
	// std::cout<<query<<endl;
	mysql_query (connect,query.c_str());
	unsigned int i =0;
	res_set = mysql_store_result(connect);
	unsigned int numrows = mysql_num_rows(res_set);
	if(numrows>0)
	{
		jsonArray["error"] = false;
		jsonArray["message"] = "Records found!";
		Json::Value jsonList,ca_system_id,service_pid,ecm_pids,rmx_nos,output_channel,access_criteria,input_channel;
		while (((row= mysql_fetch_row(res_set)) !=NULL ))
		{ 
			ca_system_id.append(row[i]);
			service_pid.append(row[i+1]);
			ecm_pids.append(row[i+2]);
			rmx_nos.append(row[i+3]);
			output_channel.append(row[i+4]);
			access_criteria.append(row[i+5]);
			input_channel.append(row[i+6]);
		}
		jsonArray["ca_system_ids"]=ca_system_id;
		jsonArray["service_pids"]=service_pid;
		jsonArray["ecm_pids"]=ecm_pids;
		jsonArray["rmx_nos"]=rmx_nos;
		jsonArray["output_channel"]=output_channel;
		jsonArray["input_channel"]=input_channel;
		jsonArray["access_criteria"]=access_criteria;
	}else{
		jsonArray["error"] = true;
		jsonArray["message"] = "No records found!";
	}
 	return jsonArray;
}
Json::Value dbHandler :: getECMChannelDetails(std::string ca_system_id,std::string service_pid,std::string ecm_pid,std::string rmx_no,std::string  output){
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	Json::Value jsonArray;
	std::string query = "SELECT ed.channel_id,ed.stream_id FROM ecm_descriptor ed,ecmg e,ecm_stream es WHERE e.channel_id = es.channel_id AND e.supercas_id = '"+ca_system_id+"' AND es.channel_id = ed.channel_id AND ed.stream_id = es.stream_id AND es.ecm_pid = '"+ecm_pid+"' AND ed.service_pid = '"+service_pid+"' AND ed.rmx_id = '"+rmx_no+"' AND ed.output = '"+output+"'";
	// std::cout<<query<<endl;
	mysql_query (connect,query.c_str());
	unsigned int i =0;
	res_set = mysql_store_result(connect);
	unsigned int numrows = mysql_num_rows(res_set);
	if(numrows>0)
	{
		jsonArray["error"] = false;
		jsonArray["message"] = "Records found!";
		Json::Value jsonList,ca_system_id,service_pid,ecm_pids,rmx_nos,output_channel,access_criteria;
		while (((row= mysql_fetch_row(res_set)) !=NULL ))
		{ 
			jsonArray["channel_id"] = row[i];
			jsonArray["stream_id"] = row[i+1];
		}
	}else{
		jsonArray["error"] = true;
		jsonArray["message"] = "No records found!";
	}
 	return jsonArray;
}

Json::Value dbHandler :: getSymbolRates(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		int nit_mode=-1;
		Json::Value jsonList;
		std::string query="select output_channel,rmx_id,symbol_rate,roll_off from output;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					// nit_mode=std::stoi(row[i]);
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["rmx_no"]=row[i+1];
					jsonObj["symbol_rate"]=row[i+2];
					jsonObj["roll_off"]=row[i+3];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
}

int dbHandler :: addNITDetails(int version,std::string network_name,std::string network_id){
	string query = "";
	if(isNetworkExists() == 0)
		query = "INSERT INTO `NIT_version` ( `network_id`, `network_name`, `nit_version`) VALUES ('"+network_id+"', '"+network_name+"', '"+std::to_string(version)+"');";
	else
		query="UPDATE NIT_version SET network_id='"+network_id+"',network_name = '"+network_name+"',nit_version = '"+std::to_string(version)+"';";
		
	// std::cout<<query<<std::endl;
	mysql_query (connect,query.c_str());
	return mysql_affected_rows(connect);		
}

int dbHandler :: isNetworkExists(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT COUNT(*) FROM NIT_version;";
		// std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}
Json::Value dbHandler :: getNITDetails(){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;

		std::string query="SELECT network_id,network_name,nit_version FROM NIT_version;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonArray["network_id"]=row[i];
					jsonArray["network_name"]=row[i+1];
					jsonArray["nit_version"]=row[i+2];
				}
			}else{	
				jsonArray["error"] = true;
				jsonArray["message"] = "NO record found!";
			}
		}else{ 		
			jsonArray["error"] = true;
			jsonArray["message"] = "ERROR!";
		}
		return jsonArray;
	}

Json::Value  dbHandler :: getNITHostMode() 
	{
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		int nit_mode=-1;
		Json::Value jsonList;
		std::string query="select output_channel,rmx_id from output WHERE nit_mode = 2;";
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["output"]=row[i];
					jsonObj["rmx_no"]=row[i+1];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}

Json::Value dbHandler :: getLcnIds(std::string output,std::string rmx_no){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		query="SELECT a.in_channel,a.channel_num,a.lcn_id FROM active_service_list a WHERE a.rmx_id = '"+rmx_no+"' AND a.out_channel = '"+output+"' AND a.lcn_id != -1;";
		// std::cout<<query<<endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					Json::Value jsonObj;
					jsonObj["input"]=row[i];
					jsonObj["service_id"]=row[i+1];
					jsonObj["lcn_id"]=row[i+2];
					jsonList.append(jsonObj);
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}


int dbHandler :: addLcnIds(std::string service_id,std::string lcn_id,std::string input, std::string output,std::string rmx_no){
	string query ="";
	query = "UPDATE active_service_list SET lcn_id ='"+lcn_id+"' WHERE rmx_id = '"+rmx_no+"' AND in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num = '"+service_id+"';";
		
	// std::cout<<query<<std::endl;
	mysql_query (connect,query.c_str());
	if(mysql_affected_rows(connect)){
		return 1;
	}else{
		query = "SELECT COUNT(*) FROM active_service_list WHERE lcn_id ='"+lcn_id+"' AND rmx_id = '"+rmx_no+"' AND in_channel = '"+input+"' AND out_channel = '"+output+"' AND channel_num = '"+service_id+"';";
		mysql_query (connect,query.c_str());
		res_set = mysql_store_result(connect);
		if(mysql_num_rows(res_set))
		{
			row= mysql_fetch_row(res_set);
		 	return atoi(row[0]);
	 	}else{
	 		return 0;
	 	}
	}		
}

int dbHandler :: isLcnIdExists(std::string lcn_id){
	string query = "SELECT COUNT(*) FROM active_service_list WHERE lcn_id ='"+lcn_id+"';";
	mysql_query (connect,query.c_str());
	res_set = mysql_store_result(connect);
	if(mysql_num_rows(res_set))
	{
		row= mysql_fetch_row(res_set);
	 	return atoi(row[0]);
 	}else{
 		return 0;
 	}
}


Json::Value dbHandler :: getNetworkId(std::string rmx_no,std::string output){
		MYSQL_RES *res_set;
		MYSQL_ROW row;
		Json::Value jsonList;
		std::string query;
		std::string indexValue;

		query="SELECT network_id,original_netw_id FROM network_details WHERE rmx_id = '"+rmx_no+"' AND output = '"+output+"';";
		std::cout<<query<<std::endl;
		mysql_query (connect,query.c_str());
		unsigned int i =0;
		res_set = mysql_store_result(connect);
		if(res_set){
			unsigned int numrows = mysql_num_rows(res_set);
			if(numrows>0)
			{
				jsonArray["error"] = false;
				while (((row= mysql_fetch_row(res_set)) !=NULL ))
				{ 
					jsonArray["network_id"]=row[i];
					jsonArray["original_netw_id"]=row[i+1];
				}
			}else{	
				jsonArray["error"] = true;
			}
			jsonArray["list"]=jsonList;
		}else{ 		
			jsonArray["error"] = true;
		}
	 	return jsonArray;
	}








// int rmx_no = 6;
// 		int frequency = 462;
// 		for (int i = 0; i < 8; ++i)
// 		{
// 			std::string query="UPDATE output SET frequency='"+std::to_string(frequency)+"',symbol_rate = '6400'  WHERE output_channel = '"+std::to_string(i)+"' AND rmx_id = '"+std::to_string(rmx_no)+"';";
// 			mysql_query (connect,query.c_str());
// 			frequency = frequency+8;
// 			std::cout<<query<<endl;
// 		}