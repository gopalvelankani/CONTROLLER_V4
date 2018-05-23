#include "http.h"
#include "router.h"
#include "endpoint.h"
#include <algorithm>
#include "dbHandler.h"
#include <callcommand.h>
#include <udpipstack.h>
#include "Itoc.h"
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include "transport.h"
#include <fstream>
#include <unistd.h>
#include <random>
#include </usr/include/hiredis/hiredis.h>
#include <pthread.h>
#include <ctime>
#include <sys/time.h>
#include <stdlib.h>
#include <typeinfo>
#include <signal.h>
#include <stdio.h>
extern "C" {
#include "MxlApiLib.h"
}

#define STX 0x02
#define ETX 0x03
#define RMX_COUNT 6
#define INPUT_COUNT 7
#define OUTPUT_COUNT 7
#define CMD_VER_FIMWARE  0x00
using namespace std;
using namespace Net;
#define CW_MQ_TIME_DIFF 2
#define Invalid_json "Invalid json parameters"
#define QAM_CS 11
#define QAM_ADDR 12
#define DVBC_VER_ADDR 0
#define SPI_STATUS_ADDR 4
#define SPI_CONF_ADDR 8

#define FSYMBOLRATE_ADDR 12
#define FDAC_ADDR 8
#define GAIN_ADDR 16
#define UPSAMPLER_VER_ADDR 0
#define STATUS_REG_ADDR 4

#define DVBCSA_VER_ADDR 0
#define DVBCSA_CORE_CONTR_ADDR 4
#define DVBCSA_LSB_KEY_ADDR 8
#define DVBCSA_MSB_KEY_ADDR 12
#define DVBCSA_KEY_CONTR_ADDR 16
#define FboardClk 135.0
#define FDAC_VALUE 2048
#define CHECK(X) if ( !X || X->type == REDIS_REPLY_ERROR ) { printf("Redis Authentication error! \n"); exit(-1); }
void printCookies(const Net::Http::Request& req) {
    auto cookies = req.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto& c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
    std::cout << req.body();
}
class StatsEndpoint {
public:
    Config cnf;
    unsigned int j;
    redisContext *context;
    redisReply *reply,*stream_reply;
    int err;
    pthread_t tid,thid;
    static int CW_THREAD_CREATED;
    Json::Value streams_json,scrambled_services;
    Callcommand c1;
    Udpipstack c2;
    Itoc c3;
    dbHandler *db;
    int channel_ids,ecm_port,emm_port,emm_bw,stream_id,data_id;
    std::string supercas_id,ecm_ip,client_id;
    char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int DVBC_OUTPUT_CS[8] = {10,11,12,13,14,15,16,17};
    int UPSAMPLER_OUTPUT_CS[8] = {18,19,20,21,22,23,24,25};
    int DVBCSA_OUTPUT_CS[8] = {2,3,4,5,6,7,8,9};
    StatsEndpoint(Net::Address addr)
        : httpEndpoint(std::make_shared<Net::Http::Endpoint>(addr))
    {
    
    }
    void init(size_t thr = 2,std::string config_file_path="config.conf") {
        cnf.readConfig(config_file_path);
        db = new dbHandler(config_file_path);
        auto opts = Net::Http::Endpoint::options()
            .threads(thr)
            .flags(Net::Tcp::Options::ReuseAddr);
        httpEndpoint->init(opts);
        setupRoutes();
        initRedisConn();
        
        std::cout<<"------------------------------INIT()-----------------------------"<<std::endl;
        // forkEMMChannels();
        // forkChannels();
        // forkCWthread();
        runBootupscript();
    }
    void start() {
        
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serve();  
        std::cout<<"------------------------------START()-----------------------------"<<std::endl;           
        
    }

    void shutdown() {
        httpEndpoint->shutdown();
    }
    void addCWQueue(){
        Json::Value jlist;
        jlist=streams_json["list"];
        reply = (redisReply *)redisCommand(context,"SELECT 5");
        int j=1;
        for (int i = 0; i < jlist.size(); ++i)
        {
            //printf("\n Record -- %s == %s == %s == %s \n",((jlist[i]["stream_id"]).asString()).c_str(),((jlist[i]["access_criteria"]).asString()).c_str(),((jlist[i]["timestamp"]).asString()).c_str(),((jlist[i]["cp_number"]).asString()).c_str());     
            if(getReminderOfTime(std::stoi(jlist[i]["timestamp"].asString()))==0){
                std::string query= "HMSET cw_provision:ch_"+jlist[i]["channel_id"].asString()+":"+std::to_string((std::stoi(getCurrentTime()))+CW_MQ_TIME_DIFF)+":"+std::to_string(j)+" stream_id "+jlist[i]["stream_id"].asString()+" channel_id "+jlist[i]["channel_id"].asString()+" curr_CW "+generatorRandCW()+" next_CW "+generatorRandCW()+" timestamp "+getCurrentTime()+"";
                std::cout<<"----------------------- "+jlist[i]["channel_id"].asString()+":"+jlist[i]["stream_id"].asString()<<std::endl;
                reply = (redisReply *)redisCommand(context,query.c_str());

                std::string query1="EXPIREAT cw_provision:ch_"+jlist[i]["channel_id"].asString()+":"+std::to_string((std::stoi(getCurrentTime()))+CW_MQ_TIME_DIFF)+":"+std::to_string(j)+" "+std::to_string((std::stoi(getCurrentTime()))+15) +"";
                reply = (redisReply *)redisCommand(context,query1.c_str());
                j++;
            }
        }
    }

private:
    void setupRoutes() {
        using namespace Net::Rest;
        Routes::Get(router, "/getAllInputServices/:rmx_no", Routes::bind(&StatsEndpoint::getAllInputServices, this));
        Routes::Get(router, "/getHardwareVersion/:rmx_no", Routes::bind(&StatsEndpoint::getHardwareVersion, this));
        Routes::Post(router, "/getHardwareVersion", Routes::bind(&StatsEndpoint::getHardwareVersions, this));
        Routes::Get(router, "/getFirmwareVersion/:rmx_no", Routes::bind(&StatsEndpoint::getFirmwareVersion, this));
        Routes::Post(router, "/getFirmwareVersion", Routes::bind(&StatsEndpoint::getFirmwareVersions, this));
        Routes::Post(router, "/setInputOutput", Routes::bind(&StatsEndpoint::setInputOutput, this));
        Routes::Get(router, "/getInputOutput/:rmx_no", Routes::bind(&StatsEndpoint::getInputOutput, this));
        Routes::Post(router, "/setInputMode", Routes::bind(&StatsEndpoint::setInputMode, this));
        Routes::Get(router, "/getInputMode/:rmx_no", Routes::bind(&StatsEndpoint::getInputMode, this));
        Routes::Post(router, "/getInputMode", Routes::bind(&StatsEndpoint::getInputModes, this));
        Routes::Post(router, "/setCore", Routes::bind(&StatsEndpoint::setCore, this));
        Routes::Post(router, "/getCore", Routes::bind(&StatsEndpoint::getCore, this));
        Routes::Get(router, "/getGPIO/:rmx_no", Routes::bind(&StatsEndpoint::getGPIO, this));//not their in the document
        Routes::Get(router, "/readI2c", Routes::bind(&StatsEndpoint::readI2c, this));
        Routes::Get(router, "/readSFN/:rmx_no", Routes::bind(&StatsEndpoint::readSFN, this));
        Routes::Post(router, "/setLcn", Routes::bind(&StatsEndpoint::setLcn, this));
        Routes::Get(router, "/getLCN/:rmx_no", Routes::bind(&StatsEndpoint::getLCN, this));
        Routes::Post(router, "/getLCN", Routes::bind(&StatsEndpoint::getLcn, this));
        Routes::Get(router, "/getHostNIT/:rmx_no", Routes::bind(&StatsEndpoint::getHostNIT, this));//no
        Routes::Post(router, "/setDvbSpiOutputMode", Routes::bind(&StatsEndpoint::setDvbSpiOutputMode, this));
        Routes::Get(router, "/getDvbSpiOutputMode/:rmx_no", Routes::bind(&StatsEndpoint::getDvbSpiOutputMode, this));
        Routes::Post(router, "/getDvbSpiOutputMode", Routes::bind(&StatsEndpoint::getDvbSpiOutputModes, this));
        Routes::Post(router, "/setNetworkName", Routes::bind(&StatsEndpoint::setNetworkName, this));
        Routes::Get(router, "/getNetworkName/:rmx_no", Routes::bind(&StatsEndpoint::getNetworkName, this));
        Routes::Post(router, "/getNetworkName", Routes::bind(&StatsEndpoint::getNetworkname, this));
        Routes::Post(router, "/setServiceName", Routes::bind(&StatsEndpoint::setServiceName, this));
        Routes::Get(router, "/getServiceName/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getServiceName, this));
        Routes::Post(router, "/getServiceName", Routes::bind(&StatsEndpoint::getServicename, this));
        Routes::Get(router, "/getDynamicStateWinSize/:rmx_no", Routes::bind(&StatsEndpoint::getDynamicStateWinSize, this));
        Routes::Post(router, "/getDynamicStateWinSize", Routes::bind(&StatsEndpoint::getDynamicStateWinsize, this));
        Routes::Get(router, "/getFilterCA/:rmx_no", Routes::bind(&StatsEndpoint::getFilterCA, this));
        Routes::Post(router, "/getFilterCA", Routes::bind(&StatsEndpoint::getFilterCAS, this));
        Routes::Post(router, "/setServiceID", Routes::bind(&StatsEndpoint::setServiceID, this));
        Routes::Post(router, "/flushServiceIDs", Routes::bind(&StatsEndpoint::flushServiceIDs, this));
        Routes::Get(router, "/getServiceID/:rmx_no", Routes::bind(&StatsEndpoint::getServiceID, this));
        Routes::Post(router, "/getServiceID", Routes::bind(&StatsEndpoint::getServiceIDs, this));
        Routes::Get(router, "/getSmoothFilterInfo/:rmx_no", Routes::bind(&StatsEndpoint::getSmoothFilterInfo, this));
        Routes::Post(router, "/getSmoothFilterInfo", Routes::bind(&StatsEndpoint::getSmoothFilterinfo, this));
        Routes::Get(router, "/getProgramList/:rmx_no", Routes::bind(&StatsEndpoint::getProgramList, this));
        Routes::Post(router, "/getProgramList", Routes::bind(&StatsEndpoint::getProgramsList, this));
        // Routes::Post(router, "/getProgramsListNames", Routes::bind(&StatsEndpoint::getProgramsListNames, this));
        Routes::Get(router, "/getCryptedProg/:rmx_no", Routes::bind(&StatsEndpoint::getCryptedProg, this));
        Routes::Post(router, "/getCryptedProg", Routes::bind(&StatsEndpoint::getCryptedPrograms, this));
        Routes::Get(router, "/getAllReferencedPIDInfo/:rmx_no", Routes::bind(&StatsEndpoint::getAllReferencedPIDInfo, this));
        Routes::Post(router, "/getAllReferencedPIDInfo", Routes::bind(&StatsEndpoint::getAllReferencedPidInfo, this));
        Routes::Post(router, "/setEraseCAMod", Routes::bind(&StatsEndpoint::setEraseCAMod, this));
        Routes::Get(router, "/getEraseCAMod/:rmx_no", Routes::bind(&StatsEndpoint::getEraseCAMod, this));        
        Routes::Post(router, "/getEraseCAMod", Routes::bind(&StatsEndpoint::getEraseCAmod, this));
        Routes::Get(router, "/eraseCAMod/:rmx_no", Routes::bind(&StatsEndpoint::eraseCAMod, this));
        Routes::Post(router, "/eraseCAMod", Routes::bind(&StatsEndpoint::eraseCAmod, this));
        Routes::Get(router, "/getStatPID/:rmx_no", Routes::bind(&StatsEndpoint::getStatPID, this));//not there in the document
        Routes::Post(router, "/setKeepProg", Routes::bind(&StatsEndpoint::setKeepProg, this));
        Routes::Post(router, "/getKeptProgramFromOutput", Routes::bind(&StatsEndpoint::getKeptProgramFromOutput, this));
        Routes::Get(router, "/getProgActivation/:rmx_no", Routes::bind(&StatsEndpoint::getProgActivation, this));
        Routes::Post(router, "/getProgActivation", Routes::bind(&StatsEndpoint::getActivatedProgs, this));
        Routes::Post(router, "/getAllActivatedServices", Routes::bind(&StatsEndpoint::getAllActivatedServices, this));
        Routes::Post(router, "/setLockedPIDs", Routes::bind(&StatsEndpoint::setLockedPIDs, this));
        Routes::Get(router, "/getLockedPIDs/:rmx_no", Routes::bind(&StatsEndpoint::getLockedPIDs, this));        
        Routes::Post(router, "/getLockedPIDs", Routes::bind(&StatsEndpoint::getLockedPids, this));        
        Routes::Get(router, "/flushLockedPIDs/:rmx_no", Routes::bind(&StatsEndpoint::flushLockedPIDs, this));
        Routes::Post(router, "/flushLockedPIDs", Routes::bind(&StatsEndpoint::flushLockedPids, this));
        Routes::Post(router, "/setHighPriorityServices", Routes::bind(&StatsEndpoint::setHighPriorityServices, this));
        Routes::Get(router, "/getHighPriorityServices/:rmx_no", Routes::bind(&StatsEndpoint::getHighPriorityServices, this));//
        Routes::Post(router, "/getHighPriorityServices", Routes::bind(&StatsEndpoint::getHighPriorityService, this));//
        Routes::Get(router, "/flushHighPriorityServices/:rmx_no", Routes::bind(&StatsEndpoint::flushHighPriorityServices, this));
        Routes::Post(router, "/flushHighPriorityServices", Routes::bind(&StatsEndpoint::flushHighPriorityService, this));
        Routes::Get(router, "/getPsiSiDecodingStatus/:rmx_no", Routes::bind(&StatsEndpoint::getPsiSiDecodingStatus, this));
        Routes::Post(router, "/getPsiSiDecodingStatus", Routes::bind(&StatsEndpoint::getPSISIDecodingStatus, this));
        Routes::Get(router, "/getTSFeatures/:rmx_no", Routes::bind(&StatsEndpoint::getTSFeatures, this));
        Routes::Post(router, "/getTSFeatures", Routes::bind(&StatsEndpoint::getTsFeatures, this));
        Routes::Post(router, "/setLcnProvider", Routes::bind(&StatsEndpoint::setLcnProvider, this));
        Routes::Get(router, "/getLcnProvider/:rmx_no", Routes::bind(&StatsEndpoint::getLcnProvider, this));
        Routes::Post(router, "/getLcnProvider", Routes::bind(&StatsEndpoint::getLcnProviders, this));
        Routes::Post(router, "/setServiceProvider", Routes::bind(&StatsEndpoint::setServiceProvider, this));
        Routes::Get(router, "/getServiceProvider/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getServiceProviderName, this));
        Routes::Post(router, "/getServiceProvider", Routes::bind(&StatsEndpoint::getServiceProvider, this));

        Routes::Get(router, "/getProgramOriginalName/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getProgramOriginalName, this));
        Routes::Post(router, "/getProgramOriginalName", Routes::bind(&StatsEndpoint::getProgramOriginalname, this));
        Routes::Get(router, "/getProgramOriginalNetworkName/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getProgramOriginalNetworkName, this));
        Routes::Post(router, "/getProgramOriginalNetworkName", Routes::bind(&StatsEndpoint::getProgramOriginalNetworkname, this));
        Routes::Post(router, "/getAffectedOutputServices", Routes::bind(&StatsEndpoint::getAffectedOutputServices, this));
        Routes::Post(router, "/downloadTables", Routes::bind(&StatsEndpoint::downloadTables, this));
        Routes::Post(router, "/setCreateAlarmFlags", Routes::bind(&StatsEndpoint::setCreateAlarmFlags, this));
        Routes::Post(router, "/setTableTimeout", Routes::bind(&StatsEndpoint::setTableTimeout, this));//

        Routes::Get(router, "/getPrograminfo/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getPrograminfo, this));
        Routes::Post(router, "/getPrograminfo", Routes::bind(&StatsEndpoint::getProgramInfo, this));
        Routes::Get(router, "/getProgramOriginalProviderName/:uProg/:rmx_no", Routes::bind(&StatsEndpoint::getProgramOriginalProviderName, this));

        Routes::Post(router, "/getProgramOriginalProviderName", Routes::bind(&StatsEndpoint::getProgramOriginalProvidername, this));
        Routes::Post(router, "/setPmtAlarm", Routes::bind(&StatsEndpoint::setPmtAlarm, this));
        Routes::Get(router, "/getPmtAlarm/:rmx_no", Routes::bind(&StatsEndpoint::getPmtAlarm, this));
        Routes::Post(router, "/getPmtAlarm", Routes::bind(&StatsEndpoint::getPMTAlarm, this));
        Routes::Get(router, "/getDataflowRates/:rmx_no", Routes::bind(&StatsEndpoint::getDataflowRates, this));
        Routes::Post(router, "/getDataflowRates", Routes::bind(&StatsEndpoint::getDataFlowRates, this));
        Routes::Post(router, "/getChannelDetails", Routes::bind(&StatsEndpoint::getChannelDetails, this));

        Routes::Post(router, "/setTablesVersion", Routes::bind(&StatsEndpoint::setTablesVersion, this));
        Routes::Post(router, "/setTablesVersionToAllRmx", Routes::bind(&StatsEndpoint::setTablesVersionToAllRmx, this));
        Routes::Get(router, "/getTablesVersion/:rmx_no", Routes::bind(&StatsEndpoint::getTablesVersion, this));
        Routes::Post(router, "/getTablesVersion", Routes::bind(&StatsEndpoint::getTablesVersions, this));
        Routes::Post(router, "/setPsiSiInterval", Routes::bind(&StatsEndpoint::setPsiSiInterval, this));
        Routes::Post(router, "/setPsiSiIntervalToAllRmx", Routes::bind(&StatsEndpoint::setPsiSiIntervalToAllRmx, this));
        Routes::Get(router, "/getPsiSiInterval/:rmx_no", Routes::bind(&StatsEndpoint::getPsiSiInterval, this));
        Routes::Post(router, "/getPsiSiInterval", Routes::bind(&StatsEndpoint::getPsiSiIntervals, this));
        Routes::Post(router, "/getTableDetails", Routes::bind(&StatsEndpoint::getTableDetails, this));

        Routes::Post(router, "/setTsId", Routes::bind(&StatsEndpoint::setTsId, this));
        Routes::Post(router, "/setTransportStreamId", Routes::bind(&StatsEndpoint::setTransportStreamId, this));
        Routes::Post(router, "/setONId", Routes::bind(&StatsEndpoint::setONId, this));
        Routes::Get(router, "/getTsId/:rmx_no", Routes::bind(&StatsEndpoint::getTsId, this));
        Routes::Post(router, "/getTsId", Routes::bind(&StatsEndpoint::getTSID, this));
        Routes::Post(router, "/setNITmode", Routes::bind(&StatsEndpoint::setNITmode, this));
        Routes::Post(router, "/setNITmodeToAllRmx", Routes::bind(&StatsEndpoint::setNITmodeToAllRmx, this));
        Routes::Get(router, "/getNITmode/:rmx_no", Routes::bind(&StatsEndpoint::getNITmode, this));      
        Routes::Post(router, "/getNITmode", Routes::bind(&StatsEndpoint::getNITmodes, this));      
        Routes::Post(router, "/getTSDetails", Routes::bind(&StatsEndpoint::getTSDetails, this));      
        Routes::Post(router, "/setTDT_TOT", Routes::bind(&StatsEndpoint::setTDT_TOT, this));
        
        //UDP IP STACK Commands
        Routes::Post(router, "/readWrite32bUdpCpu", Routes::bind(&StatsEndpoint::readWrite32bUdpCpu, this));
        Routes::Post(router, "/readWrite32bUdpI2C", Routes::bind(&StatsEndpoint::readWrite32bUdpI2C, this));
        Routes::Get(router, "/getVersionofcore", Routes::bind(&StatsEndpoint::getVersionofcore, this));
        Routes::Post(router, "/setIpdestination", Routes::bind(&StatsEndpoint::setIpdestination, this));
        Routes::Get(router, "/getIpdestination", Routes::bind(&StatsEndpoint::getIpdestination, this));
        Routes::Post(router, "/setSubnetmask", Routes::bind(&StatsEndpoint::setSubnetmask, this));
        Routes::Get(router, "/getSubnetmask", Routes::bind(&StatsEndpoint::getSubnetmask, this));
        Routes::Post(router, "/setIpgateway", Routes::bind(&StatsEndpoint::setIpgateway, this));
        Routes::Get(router, "/getIpgateway", Routes::bind(&StatsEndpoint::getIpgateway, this));
        Routes::Get(router, "/getDhcpIpgateway", Routes::bind(&StatsEndpoint::getDhcpIpgateway, this));
        Routes::Get(router, "/getDhcpSubnetmask", Routes::bind(&StatsEndpoint::getDhcpSubnetmask, this));
        Routes::Post(router, "/setIpmulticast", Routes::bind(&StatsEndpoint::setIpmulticast, this));
        Routes::Get(router, "/getIpmulticast", Routes::bind(&StatsEndpoint::getIpmulticast, this));
        Routes::Get(router, "/getFpgaIp", Routes::bind(&StatsEndpoint::getFpgaIp, this));
        Routes::Post(router, "/setFpgaipDhcp", Routes::bind(&StatsEndpoint::setFpgaipDhcp, this));
        Routes::Get(router, "/getFpgaipDhcp", Routes::bind(&StatsEndpoint::getFpgaipDhcp, this));
        Routes::Post(router, "/setFpgaMACAddress", Routes::bind(&StatsEndpoint::setFpgaMACAddress, this));
        Routes::Get(router, "/getFpgaMACAddress", Routes::bind(&StatsEndpoint::getFpgaMACAddress, this));
        Routes::Get(router, "/getUDPportSource", Routes::bind(&StatsEndpoint::getUDPportSource, this));
        Routes::Get(router, "/getUDPportDestination", Routes::bind(&StatsEndpoint::getUDPportDestination, this));
        Routes::Get(router, "/getUDPChannelNumber", Routes::bind(&StatsEndpoint::getUDPChannelNumber, this));
        Routes::Get(router, "/getIGMPChannelNumber", Routes::bind(&StatsEndpoint::getIGMPChannelNumber, this));
        Routes::Get(router, "/getUdpchannels", Routes::bind(&StatsEndpoint::getUdpchannels, this));
        Routes::Post(router, "/setUDPportSource", Routes::bind(&StatsEndpoint::setUDPportSource, this));
        Routes::Post(router, "/setUDPportDestination", Routes::bind(&StatsEndpoint::setUDPportDestination, this));
        Routes::Post(router, "/setUDPChannelNumber", Routes::bind(&StatsEndpoint::setUDPChannelNumber, this));
        Routes::Post(router, "/setIGMPChannelNumber", Routes::bind(&StatsEndpoint::setIGMPChannelNumber, this));
        Routes::Post(router, "/setEthernetOut", Routes::bind(&StatsEndpoint::setEthernetOut, this));
        Routes::Post(router, "/getEthernetOut", Routes::bind(&StatsEndpoint::getEthernetOut, this));
        Routes::Post(router, "/setEthernetOutOff", Routes::bind(&StatsEndpoint::setEthernetOutOff, this));
        Routes::Post(router, "/setEthernetIn", Routes::bind(&StatsEndpoint::setEthernetIn, this));
        Routes::Post(router, "/setEthernetInOff", Routes::bind(&StatsEndpoint::setEthernetInOff, this));
        Routes::Post(router, "/setSPTSEthernetInOff", Routes::bind(&StatsEndpoint::setSPTSEthernetInOff, this));
        Routes::Post(router, "/getEthernetIn", Routes::bind(&StatsEndpoint::getEthernetIn, this));
        Routes::Post(router, "/setSPTSEthernetIn", Routes::bind(&StatsEndpoint::setSPTSEthernetIn, this));
        Routes::Post(router, "/setPCRBypass", Routes::bind(&StatsEndpoint::setPCRBypass, this));

        //Maxlinear api's
        Routes::Post(router, "/connectMxl", Routes::bind(&StatsEndpoint::connectMxl, this));
        Routes::Post(router, "/setDemodMxl", Routes::bind(&StatsEndpoint::setDemodMxl, this));
        Routes::Post(router, "/setMPEGOut", Routes::bind(&StatsEndpoint::setMPEGOut, this));
        Routes::Post(router, "/downloadFirmware", Routes::bind(&StatsEndpoint::downloadFirmware, this));
        Routes::Post(router, "/getMxlVersion", Routes::bind(&StatsEndpoint::getMxlVersion, this));
        Routes::Post(router, "/getDemodMxl", Routes::bind(&StatsEndpoint::getDemodMxl, this));
        Routes::Post(router, "/setConfAllegro", Routes::bind(&StatsEndpoint::setConfAllegro, this));
        Routes::Post(router, "/readAllegro", Routes::bind(&StatsEndpoint::readAllegro, this));
        Routes::Post(router, "/authorizeRFout", Routes::bind(&StatsEndpoint::authorizeRFout, this));
        Routes::Post(router, "/setIPTV", Routes::bind(&StatsEndpoint::setIPTV, this));
        Routes::Post(router, "/setMxlTuner", Routes::bind(&StatsEndpoint::setMxlTuner, this));
        Routes::Post(router, "/setMxlTunerOff", Routes::bind(&StatsEndpoint::setMxlTunerOff, this));
        /*i2c controller*/
         Routes::Get(router, "/getCenterFrequency/:rmx_no", Routes::bind(&StatsEndpoint::getCenterFrequency, this));
         Routes::Post(router, "/setCenterFrequency", Routes::bind(&StatsEndpoint::setCenterFrequency, this));
        /*i2c controller*/
        
        Routes::Post(router, "/addECMChannelSetup", Routes::bind(&StatsEndpoint::addECMChannelSetup, this));
        Routes::Post(router, "/updateECMChannelSetup", Routes::bind(&StatsEndpoint::updateECMChannelSetup, this));
        Routes::Post(router, "/deleteECMChannelSetup", Routes::bind(&StatsEndpoint::deleteECMChannelSetup, this));
        Routes::Post(router, "/addECMStreamSetup", Routes::bind(&StatsEndpoint::addECMStreamSetup, this));
        Routes::Post(router, "/updateECMStreamSetup", Routes::bind(&StatsEndpoint::updateECMStreamSetup, this));
        Routes::Post(router, "/deleteECMStreamSetup", Routes::bind(&StatsEndpoint::deleteECMStreamSetup, this));
        Routes::Post(router, "/addEmmgSetup", Routes::bind(&StatsEndpoint::addEmmgSetup, this));
        Routes::Post(router, "/updateEmmgSetup", Routes::bind(&StatsEndpoint::updateEmmgSetup, this));
        Routes::Post(router, "/deleteEMMSetup", Routes::bind(&StatsEndpoint::deleteEMMSetup, this));
        Routes::Post(router, "/scrambleService", Routes::bind(&StatsEndpoint::scrambleService, this));
        Routes::Post(router, "/deScrambleService", Routes::bind(&StatsEndpoint::deScrambleService, this));
        Routes::Post(router, "/setCATCADescriptor", Routes::bind(&StatsEndpoint::setCATCADescriptor, this));
        Routes::Post(router, "/setPMTCADescriptor", Routes::bind(&StatsEndpoint::setPMTCADescriptor, this));
        Routes::Post(router, "/getPMTCADescriptor", Routes::bind(&StatsEndpoint::getPMTCADescriptor, this));
        Routes::Post(router, "/setCSAParity", Routes::bind(&StatsEndpoint::setCSAParity, this));

        //DVBC CORE
        Routes::Post(router, "/getDVBCCoreVersion", Routes::bind(&StatsEndpoint::getDVBCCoreVersion, this));  
        Routes::Post(router, "/getSPIStatusReg", Routes::bind(&StatsEndpoint::getSPIStatusReg, this));   
        Routes::Post(router, "/getSPIConfigReg", Routes::bind(&StatsEndpoint::getSPIConfigReg, this));  
        Routes::Post(router, "/setSPIConfigReg", Routes::bind(&StatsEndpoint::setSPIConfigReg, this));   
        Routes::Post(router, "/setQAM", Routes::bind(&StatsEndpoint::setQAM, this));
        Routes::Post(router, "/getQAM", Routes::bind(&StatsEndpoint::getQAM, this));   

        //Upsampler
        Routes::Post(router, "/setFSymbolRate", Routes::bind(&StatsEndpoint::setFSymbolRate, this));
        Routes::Post(router, "/getFSymbolRate", Routes::bind(&StatsEndpoint::getFSymbolRate, this));
        Routes::Post(router, "/setFDAC", Routes::bind(&StatsEndpoint::setFDAC, this));
        Routes::Post(router, "/getFDAC", Routes::bind(&StatsEndpoint::getFDAC, this));
        Routes::Post(router, "/confGain", Routes::bind(&StatsEndpoint::confGain, this));
        Routes::Post(router, "/getGain", Routes::bind(&StatsEndpoint::getGain, this));
        Routes::Post(router, "/getUpsamplerCoreVersion", Routes::bind(&StatsEndpoint::getUpsamplerCoreVersion, this));
        Routes::Post(router, "/setStatusReg", Routes::bind(&StatsEndpoint::setStatusReg, this));
        Routes::Post(router, "/getStatusReg", Routes::bind(&StatsEndpoint::getStatusReg, this));

        //DVB-CSA 
        Routes::Post(router, "/getDVBCSAVersion", Routes::bind(&StatsEndpoint::getDVBCSAVersion, this));        
        Routes::Post(router, "/getDVBCSACoreControlReg", Routes::bind(&StatsEndpoint::getDVBCSACoreControlReg, this));
        Routes::Post(router, "/setDVBCSACoreControlReg", Routes::bind(&StatsEndpoint::setDVBCSACoreControlReg, this));
        Routes::Post(router, "/getDVBCSAKey", Routes::bind(&StatsEndpoint::getDVBCSAKey, this));
        Routes::Post(router, "/setDVBCSAKey", Routes::bind(&StatsEndpoint::setDVBCSAKey, this));
        Routes::Post(router, "/upconverterRW", Routes::bind(&StatsEndpoint::upconverterRW, this));

        // Encrypted programs
        Routes::Post(router, "/setEncryptedServices", Routes::bind(&StatsEndpoint::setEncryptedServices, this));
        Routes::Post(router, "/setEncrypteService", Routes::bind(&StatsEndpoint::setEncrypteService, this));
        Routes::Post(router, "/getEncryptedServices", Routes::bind(&StatsEndpoint::getEncryptedServices, this));

        //CSA
        Routes::Get(router, "/initCsa/:rmx_no", Routes::bind(&StatsEndpoint::initCsa, this));
        Routes::Get(router, "/getinitCsa/:rmx_no", Routes::bind(&StatsEndpoint::getinitCsa, this));

        // Encrypted programs
        Routes::Post(router, "/setCustomPids", Routes::bind(&StatsEndpoint::setCustomPids, this));
        Routes::Get(router, "/getCustomPids/:rmx_no", Routes::bind(&StatsEndpoint::getCustomPids, this));
        Routes::Post(router, "/setCsa", Routes::bind(&StatsEndpoint::setCsa, this));

        Routes::Post(router, "/selectInterface", Routes::bind(&StatsEndpoint::selectInterface, this));

        Routes::Post(router, "/insertNITable", Routes::bind(&StatsEndpoint::insertNITable, this));
        Routes::Post(router, "/disableNITable", Routes::bind(&StatsEndpoint::disableNITable, this));
        Routes::Post(router, "/unsetHostNITLcn", Routes::bind(&StatsEndpoint::unsetHostNITLcn, this));
        Routes::Post(router, "/setHostNITLcn", Routes::bind(&StatsEndpoint::setHostNITLcn, this));
        Routes::Post(router, "/getHostNITLcn", Routes::bind(&StatsEndpoint::getHostNITLcn, this));
        Routes::Post(router, "/reBootScript", Routes::bind(&StatsEndpoint::reBootScript, this));
        Routes::Post(router, "/getInputChannelTunerType", Routes::bind(&StatsEndpoint::getInputChannelTunerType, this));
        Routes::Post(router, "/setPrivateData", Routes::bind(&StatsEndpoint::setPrivateData, this));
        Routes::Post(router, "/getPrivateData", Routes::bind(&StatsEndpoint::getPrivateData, this));


    }
    /*****************************************************************************/
    /*  function setMxlTuner                           
        confAllegro, tuneMxl, setMpegMode, RF authorization
    */
    /*****************************************************************************/
    void  setMxlTuner(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;        
        Json::Value json,injson;
        Json::FastWriter fastWriter;   
        std::string rangeMsg[] = {"Required integer between 1-6!","Required integer between 1-6!","Required integer between 0-7!","Required integer between 0-3!","Required integer between 0-2!","Required integer!","Required integer!","Required integer between 0-2!","Required integer between 0-10!","Required integer between 0-3!","Required integer between 0-2!","Required integer between 0-2!","Required integer between 3650-11475!","Required integer !","Required integer between 0-1!","Required integer between 0-7!"};
        std::string para[] = {"mxl_id","rmx_no","demod_id","lnb_id","dvb_standard","frequency","symbol_rate","mod","fec","rolloff","pilots","spectrum","lo_frequency","polarization","auth_bit","voltage"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;   
        addToLog("setMxlTuner",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string str_mxl_id,str_rmx_no,str_demod_id,str_lnb_id,str_dvb_standard,str_frequency,str_symbol_rate,str_mod,str_fec,str_rolloff,str_pilots,str_spectrum,str_lo_frequency,str_polarization,auth_bit,str_voltage;        
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            str_demod_id = getParameter(request.body(),"demod_id");
            str_lnb_id = getParameter(request.body(),"lnb_id");
            str_dvb_standard = getParameter(request.body(),"dvb_standard");
            str_frequency = getParameter(request.body(),"frequency");
            str_symbol_rate = getParameter(request.body(),"symbol_rate");
            str_mod = getParameter(request.body(),"mod");
            str_fec = getParameter(request.body(),"fec");
            str_rolloff = getParameter(request.body(),"rolloff");
            str_pilots = getParameter(request.body(),"pilots");
            str_spectrum = getParameter(request.body(),"spectrum");
            str_lo_frequency = getParameter(request.body(),"lo_frequency");
            str_polarization = getParameter(request.body(),"polarization");
            auth_bit =getParameter(request.body(),"auth_bit");
            str_voltage =getParameter(request.body(),"voltage");
            
            error[0] = verifyInteger(str_mxl_id,1,1,6,1);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            error[2] = verifyInteger(str_demod_id,1,1,7);
            error[3] = verifyInteger(str_lnb_id,1,1,3);
            error[4] = verifyInteger(str_dvb_standard,1,1,2);
            error[5] = verifyInteger(str_frequency);
            error[6] = verifyInteger(str_symbol_rate);
            error[7] = verifyInteger(str_mod,1,1,2);
            error[8] = verifyInteger(str_fec,0,0,10);
            error[9] = verifyInteger(str_rolloff,1,1,3);
            error[10] = verifyInteger(str_pilots,1,1,2);
            error[11] = verifyInteger(str_spectrum,1,1,2);
            error[12] = verifyInteger(str_lo_frequency);
            error[13] = verifyInteger(str_polarization);
            error[14] = verifyInteger(auth_bit);
            error[15] = verifyInteger(str_voltage,1,1,7);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= rangeMsg[i];
            }
            if(all_para_valid){
                int lnb_id,rmx_no,mxl_id,dvb_standard,demod_id,frequency,symbol_rate,mod,fec,rolloff,pilots,spectrum,lo_frequency,polarization,voltage;
                lnb_id = std::stoi(str_lnb_id);
                rmx_no = std::stoi(str_rmx_no);
                mxl_id = std::stoi(str_mxl_id);
                dvb_standard = std::stoi(str_dvb_standard);
                demod_id = std::stoi(str_demod_id);
                frequency = std::stoi(str_frequency);
                symbol_rate = std::stoi(str_symbol_rate);
                mod = std::stoi(str_mod);
                fec = std::stoi(str_fec);
                rolloff = std::stoi(str_rolloff);
                pilots = std::stoi(str_pilots);
                spectrum = std::stoi(str_spectrum);
                lo_frequency = std::stoi(str_lo_frequency);
                polarization = std::stoi(str_polarization);
                voltage = std::stoi(str_voltage);
                MXL_STATUS_E mxlStatus;
                int ifrequency = calculateIfrequency(frequency,lo_frequency); 
                if(ifrequency==-1){
                    json["error"]= true;
                    json["message"]= "Invalid frequency range!";
                }else{
                    int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                    if(connectI2Clines(target)){
                        usleep(100);
                        Json::Value fwJson = downloadMxlFW(mxl_id,rmx_no);
                        if(fwJson["error"]==true){
                            json["error"]= true;
                            json["message"]= "Download FW failed!";
                        }
                        usleep(1000000);
                        if(lnb_id==0 || lnb_id==1){
                            json["allegro"] = confAllegro(8,lnb_id,voltage,mxl_id);
                            usleep(1000);
                            std::cout<<"callReadAllegro(8) H LNB 0,1\n";
                        }
                        else{
                            json["allegro"] = confAllegro(9,lnb_id,voltage,mxl_id);
                            usleep(1000);
                            std::cout<<"callReadAllegro(9) H LNB 2,3\n";
                        }
                       //Set demod 
                        usleep(1000000);
                        json = tuneMxl(demod_id,lnb_id,dvb_standard,ifrequency,symbol_rate,mod,fec,rolloff,pilots,spectrum,lo_frequency,mxl_id,rmx_no);
                        //get Demod
                         if(connectI2Clines(target)){
                            usleep(1000000);
                            unsigned char locked=0,modulation=MXL_HYDRA_MOD_AUTO,standard= MXL_HYDRA_DVBS,Fec=MXL_HYDRA_FEC_AUTO,roll_off=MXL_HYDRA_ROLLOFF_AUTO,pilot=MXL_HYDRA_PILOTS_AUTO,spectrums= MXL_HYDRA_SPECTRUM_AUTO;
                            unsigned int freq,RxPwr,rate,SNR;
                            mxlStatus = getTuneInfo(demod_id,&locked,&standard,&freq, &rate, &modulation, &Fec, &roll_off, &pilot,&spectrums, &RxPwr,&SNR);
                            json["locked"]=locked;
                            //Set Output
                            usleep(1000000);
                            setMpegMode(demod_id,1,MXL_HYDRA_MPEG_CLK_CONTINUOUS,MXL_HYDRA_MPEG_CLK_IN_PHASE,104,MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG,1,1,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE,MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED);
                            //Set RF authorization
                            usleep(1000000);
                            // RFauthorization(rmx_no);
                            // write32bCPU(0,0,12);
                            // write32bI2C(32, 0 ,std::stoi(auth_bit));
                        }else{
                            json["error"]= true;
                            json["message"]= "Connection error!";   
                        }
                    }else{
                        json["error"]= true;
                        json["message"]= "Connection error!";
                    }
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function setDemodMxl                                                    */
    /*****************************************************************************/
    void  setDemodMxl(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;        
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        std::string rangeMsg[] = {"Required integer between 1-6!","Required integer between 1-6!","Required integer between 0-7!","Required integer between 0-3!","Required integer between 0-2!","Required integer!","Required integer!","Required integer between 0-2!","Required integer between 0-10!","Required integer between 0-3!","Required integer between 0-2!","Required integer between 0-2!","Required integer between 3650-11475!"};
        std::string para[] = {"mxl_id","rmx_no","demod_id","lnb_id","dvb_standard","frequency","symbol_rate","mod","fec","rolloff","pilots","spectrum","lo_frequency"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;   
        addToLog("setDemodMxl",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
           std::string str_mxl_id,str_rmx_no,str_demod_id,str_lnb_id,str_dvb_standard,str_frequency,str_symbol_rate,str_mod,str_fec,str_rolloff,str_pilots,str_spectrum,str_lo_frequency;        
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            str_demod_id = getParameter(request.body(),"demod_id");
            str_lnb_id = getParameter(request.body(),"lnb_id");
            str_dvb_standard = getParameter(request.body(),"dvb_standard");
            str_frequency = getParameter(request.body(),"frequency");
            str_symbol_rate = getParameter(request.body(),"symbol_rate");
            str_mod = getParameter(request.body(),"mod");
            str_fec = getParameter(request.body(),"fec");
            str_rolloff = getParameter(request.body(),"rolloff");
            str_pilots = getParameter(request.body(),"pilots");
            str_spectrum = getParameter(request.body(),"spectrum");
            str_lo_frequency= getParameter(request.body(),"lo_frequency");
            
            error[0] = verifyInteger(str_mxl_id,1,1,6,1);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            error[2] = verifyInteger(str_demod_id,1,1,7);
            error[3] = verifyInteger(str_lnb_id,1,1,3);
            error[4] = verifyInteger(str_dvb_standard,1,1,2);
            error[5] = verifyInteger(str_frequency);
            error[6] = verifyInteger(str_symbol_rate);
            error[7] = verifyInteger(str_mod,1,1,2);
            error[8] = verifyInteger(str_fec,0,0,10);
            error[9] = verifyInteger(str_rolloff,1,1,3);
            error[10] = verifyInteger(str_pilots,1,1,2);
            error[11] = verifyInteger(str_spectrum,1,1,2);
            error[12] = verifyInteger(str_lo_frequency,0,0,11475,3650);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= rangeMsg[i];
            }
            if(all_para_valid){
                int lnb_id,rmx_no,mxl_id,dvb_standard,demod_id,frequency,symbol_rate,mod,fec,rolloff,pilots,spectrum,lo_frequency;
                lnb_id = std::stoi(str_lnb_id);
                rmx_no = std::stoi(str_rmx_no);
                mxl_id = std::stoi(str_mxl_id);
                dvb_standard = std::stoi(str_dvb_standard);
                demod_id = std::stoi(str_demod_id);
                frequency = std::stoi(str_frequency);
                symbol_rate = std::stoi(str_symbol_rate);
                mod = std::stoi(str_mod);
                fec = std::stoi(str_fec);
                rolloff = std::stoi(str_rolloff);
                pilots = std::stoi(str_pilots);
                spectrum = std::stoi(str_spectrum);
                lo_frequency = std::stoi(str_lo_frequency);
                int ifrequency =calculateIfrequency(frequency,lo_frequency);
                if(ifrequency==-1){
                    json["error"]= true;
                    json["message"]= "Invalid frequency range!";
                }else{
                    json = tuneMxl(demod_id,lnb_id,dvb_standard,ifrequency,symbol_rate,mod,fec,rolloff,pilots,spectrum,lo_frequency,mxl_id,rmx_no);
                    json["freq"] = ifrequency;
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    Json::Value tuneMxl(int demod_id,int lnb_id,int dvb_standard,int frequency,int symbol_rate,int mod,int fec,int rolloff,int pilots,int spectrum,int lo_frequency,int mxl_id,int rmx_no, int notFromBootup = 1){
        unsigned char enable=1,low_hihg=1;
        Json::Value json;
        MXL_STATUS_E mxlStatus;
        int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
        if(connectI2Clines(target)){
            int symbol_rate_hrz = symbol_rate*1000; 
            mxlStatus = configureLNB(lnb_id,enable,low_hihg);
            usleep(1000);
            mxlStatus = getLNB(lnb_id,&enable,&low_hihg);
            usleep(10000);
            // mxlStatus = setTuner(demod_id,1,2,1274000000,14300000,0,0,0,2,0,0,0);
            // json["demod_id"] = demod_id;json["lnb_id"] = lnb_id;json["demod_id"] = demod_id;json["dvb_standard"] = dvb_standard;json["frequency"] = frequency;json["symbol_rate_hrz"] = symbol_rate_hrz;json["mod"] = mod;json["fec"] = fec;json["rolloff"] = rolloff;json["pilots"] = pilots;json["spectrum"] = spectrum;
            std::cout<<demod_id<<lnb_id<<dvb_standard<<frequency<<symbol_rate_hrz<<mod<<fec<<rolloff<<pilots<<spectrum;
            std::cout<<"------------------------TUNER-------------------------"<<std::endl;
            mxlStatus = setTuner(demod_id,lnb_id,dvb_standard,frequency,symbol_rate_hrz,mod,fec,rolloff,pilots,spectrum,0,0);
            if(mxlStatus == MXL_SUCCESS){
                if(notFromBootup){
                    int control_fpga =ceil(double(rmx_no)/2);
                    int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                    if(write32bCPU(0,0,target) != -1){
                        int tuner_ch= db->getTunerChannelType(control_fpga,rmx_no,demod_id,0);
                        if(write32bI2C(4,0,tuner_ch) == -1){
                            json["error"]= true;
                            json["message"]= "Failed IGMP Channel Number!";
                        }
                    }
                    json["add"] = db->addTunerDetails(mxl_id,rmx_no,demod_id,lnb_id,dvb_standard,frequency,symbol_rate,mod,fec,rolloff,pilots,spectrum,lo_frequency);
                }
                printf("\nStatus Set demode success");
                json["error"] = false;
                json["message"] = "Set demode success!";
            }else{
                printf("\nStatus Set demode fail");
                json["error"] = true;
                json["message"] = "Set demode failed!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Connection error!";
        }
        return json;
    }
    int calculateIfrequency(int frequency,int lo_frequency){
        if(frequency<=4800 && frequency>=2500){
            if(lo_frequency > frequency)
                return ((lo_frequency -frequency)*1000000);    
            else
                return -1;
        }else if(frequency<= 12750 && frequency>=10950){
            if(frequency > lo_frequency)
                return ((frequency - lo_frequency)*1000000);
            else
                return -1;
        }else
            return -1;
        
    }
    /*****************************************************************************/
    /*  function getDemodMxl                                                    */
    /*****************************************************************************/
    void  getDemodMxl(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        unsigned char locked,modulation=MXL_HYDRA_MOD_AUTO,standard= MXL_HYDRA_DVBS,fec=MXL_HYDRA_FEC_AUTO,rolloff=MXL_HYDRA_ROLLOFF_AUTO,pilots=MXL_HYDRA_PILOTS_AUTO,spectrum= MXL_HYDRA_SPECTRUM_AUTO;
        unsigned int freq,RxPwr,rate,SNR;
        std::string rangeMsg[] = {"Required integer between 1-6!","Required integer between 1-6!","Required integer between 0-7!"};
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"mxl_id","rmx_no","demod_id"};
         int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDemodMxl",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){ 
            std::string str_mxl_id,str_rmx_no,str_demod_id;
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            str_demod_id = getParameter(request.body(),"demod_id"); 
            error[0] = verifyInteger(str_mxl_id,1,1,6,1);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            error[2] = verifyInteger(str_demod_id,1,1,7);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= rangeMsg[i];
            }
            if(all_para_valid){
                int mxl_id =std::stoi(str_mxl_id); 
                int rmx_no=std::stoi(str_rmx_no); 
                int demod_id=std::stoi(str_demod_id); 
                
                MXL_STATUS_E mxlStatus;
                int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                if(connectI2Clines(target)){
                    mxlStatus = getTuneInfo(demod_id,&locked,&standard,&freq, &rate, &modulation, &fec, &rolloff, &pilots,&spectrum, &RxPwr,&SNR);
                    if(mxlStatus == MXL_SUCCESS){
                        printf("\nStatus Get demode success");
                        json["error"] = false;
                        json["message"] = "Get demode success!";
                    }else{
                        printf("\nStatus Get demode fail");
                        json["error"] = true;
                        json["message"] = "Get demode failed!";
                    }
                    json["locked"] = locked;
                    json["demod_id"] = demod_id;
                    json["dvb_standard"] = standard;
                    json["frequency"] = freq;
                    json["rate"] = rate;
                    json["fec"] =fec;
                    json["modulation"] = modulation;
                    json["rolloff"] = rolloff;
                    json["spectrum"] =spectrum;
                    json["pilots"] = pilots;
                    json["snr"] = SNR;
                    json["rx_pwr"] = RxPwr;
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function setConfAllegro                                                     */
    /*****************************************************************************/
    void  setConfAllegro(const Rest::Request& request, Net::Http::ResponseWriter response){
  
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;     
        std::string rangeMsg[] = {"Required integer between 0-3!","Required integer between 1-6!","Required integer!"};   
        std::string para[] = {"lnb_id","mxl_id","voltage"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setConfAllegro",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){     
            std::string lnb_id,voltage,str_mxl_id,str_rmx_no;
            // address = getParameter(request.body(),"address");
            lnb_id = getParameter(request.body(),"lnb_id");
            voltage = getParameter(request.body(),"voltage");
            str_mxl_id = getParameter(request.body(),"mxl_id");
            
            // error[0] = verifyInteger(address);
            error[0] = verifyInteger(lnb_id,1,1,3);
            error[1] = verifyInteger(str_mxl_id,1,1,6,1);
            error[2] = verifyInteger(voltage,1,1,5);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= rangeMsg[i];
            }
            if(all_para_valid){
                int mxl_id =std::stoi(str_mxl_id); 
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                if(connectI2Clines(target)){
                    int lnb_id_i = std::stoi(lnb_id);
                    int address = (lnb_id_i == 0 || lnb_id_i == 1)?8 : 9;
                    json = confAllegro(address,lnb_id_i,std::stoi(voltage),mxl_id);
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
                
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    Json::Value confAllegro(int address,int lnb_id,int voltage,int mxl_id){
        int mxlStatus =0;
        Json::Value json;

        callReadAllegro(address);
        usleep(500000);
        Json::Value json_allegro = db->getConfAllegro(mxl_id,address);
        int enable1,enable2,volt1,volt2;
        if(json_allegro["error"] == false){
            if(lnb_id == 0 || lnb_id == 2){
                enable2 = std::stoi(json_allegro["enable2"].asString());
                volt2 = std::stoi(json_allegro["volt2"].asString());
                enable1 = 1;
                volt1 = voltage;
            }else{
                enable1 = std::stoi(json_allegro["enable1"].asString());
                volt1 = std::stoi(json_allegro["volt1"].asString());
                enable2 = 1;
                volt2 = voltage;
            }
        }else{
            enable1 = 1;
            volt1 = voltage;
            enable2 = 1;
            volt2 = voltage;
        }
        std::cout<<address<<enable1<<volt1<<enable2<<volt2<<std::endl;
        mxlStatus = confAllegro8297(address,enable1,volt1,enable2,volt2);
        if(mxlStatus == 1){
            json["added"] = db->addConfAllegro(mxl_id, address,enable1,volt1,enable2,volt2);
            printf("\nStatus conf allegro  success");
            json["message"] = "Status conf allegro  success!";
            json["error"] = false;
        }else{
            printf("\nStatus conf allegro fail");
            json["message"] = "Status conf allegro  fail!";
            json["error"] = true;
        }
        return json;
    }
    /*****************************************************************************/
    /*  function readAllegro                                                    */
    /*****************************************************************************/
    void  readAllegro(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string para[] = {"address"};
        addToLog("readAllegro",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){  
            std::string str_addr = getParameter(request.body(),"address");
            if(verifyInteger(str_addr)){      
                int address =std::stoi(str_addr); 
                json = callReadAllegro(address);
            }else{
                json["message"] = "Required Integer!";
                json["error"] = true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callReadAllegro(int address){
        Json::Value json;
        readAllegro8297(address,0);
        usleep(10000);
        json["allegroStatus"] = readAllegro8297(address,0);
        // if(mxlStatus == 1){
            printf("\nStatus read allegro  success");
            json["message"] = "Status read allegro  success!";
            json["error"] = false;
        // }else{
        //     printf("\nStatus read allegro fail");
        //     json["message"] = "Status read allegro  fail!";
        //     json["error"] = true;
        // }
        return json;
    }
    /*****************************************************************************/
    /*  function setMPEGOut                                                     */
    /*****************************************************************************/
    void  setMPEGOut(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,injson;
        Json::FastWriter fastWriter; 
        std::string rangeMsg[] = {"Required integer between 1-6!","Required integer between 1-6!","Required integer between 0-7!"};
        std::string para[] = {"mxl_id","rmx_no","demod_id"};   
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setMPEGOut",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string str_mxl_id,str_rmx_no,str_demod_id;
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            str_demod_id = getParameter(request.body(),"demod_id"); 
            error[0] = verifyInteger(str_mxl_id,1,1,6,1);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            error[2] = verifyInteger(str_demod_id,1,1,7);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= rangeMsg[i];
            }
            if(all_para_valid){
                int mxl_id =std::stoi(str_mxl_id); 
                int rmx_no=std::stoi(str_rmx_no); 
                int demod_id=std::stoi(str_demod_id); 

                int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                if(connectI2Clines(target)){
                    setMpegMode(demod_id,1,MXL_HYDRA_MPEG_CLK_CONTINUOUS,MXL_HYDRA_MPEG_CLK_IN_PHASE,104,MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG,1,1,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE,MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED);
                    json["message"]= "Set OutPut";
                    json["error"] = false;
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function authorizeRFout                                                     */
    /*****************************************************************************/
    void  authorizeRFout(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;  
         std::string para[] = {"rmx_no"};   
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            int rmx_no =std::stoi(getParameter(request.body(),"rmx_no"));
            // json = RFauthorization(rmx_no);
            int rmx_bit = 1<<(rmx_no-1); 
            if(write32bCPU(0,0,12) != -1){
                usleep(100);
                write32bI2C(32, 0 ,63);
                json["added"]= db->addRFauthorization(rmx_no,1);        
                json["message"] = "Authorize RF out!";
                json["error"] = false;
                
            }else{
                json["error"] = true;
                json["message"] = "Connection error!";
            }
            json["VALUE"] = read32bI2C(32,0);
            // std::cout<<rmx_no<<"\n";
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value RFauthorization(int rmx_no){
        Json::Value json;
        int rmx_bit = 1<<(rmx_no-1); 
        if(write32bCPU(0,0,12) != -1){
            usleep(100);
            write32bI2C(32, 0 ,rmx_no);
            // json["added"]= db->addRFauthorization(rmx_no,1);        
            json["message"] = "Authorize RF out!";
            json["error"] = false;
            
        }else{
            json["error"] = true;
            json["message"] = "Connection error!";
        }
        return json;
    }
    /*****************************************************************************/
    /*  function setIPTV                                                    */
    /*****************************************************************************/
    void  setIPTV(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        Json::Value json,injson;
        Json::FastWriter fastWriter;  
        int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        write32bCPU(0,0,target);
        setEthernet_MULT(2);
        json["message"] = "IP out!";

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function connectMxl                                                     */
    /*****************************************************************************/
    void  connectMxl(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"mxl_id","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("connectMxl",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){  
            std::string str_mxl_id,str_rmx_no;
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(str_mxl_id,1,1,6);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]] = "Required integer between 1-6!";
            }
            if(all_para_valid){      
                int mxl_id =std::stoi(str_mxl_id); 
                int rmx_no=std::stoi(str_rmx_no); 
                int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                if(connectI2Clines(target)){
                    json["error"]= false;
                    json["message"]= "Mxl Connected!";  
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }   
                json["target"]= target;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    int connectI2Clines(int target){
        
        MXL_STATUS_E mxlStatus;
        MXL_HYDRA_VER_INFO_T versionInfo;

        if(write32bCPU(0,0,target) == -1)/**select interface**/
            return 0;
        write32bCPU(7,0,0);
        
        mxlStatus =  MxL_Connect(0X60, 200, 125000);//i2caddress,i2c_speed,i2c_baudrate
        if(mxlStatus != MXL_SUCCESS && mxlStatus != MXL_ALREADY_INITIALIZED ){
            printf("Status NOT Connected");
            return 0;
        }else{
            // printf("Status connected\n");
            MxL_GetVersion(&versionInfo);
            if(versionInfo.chipId != MXL_HYDRA_DEVICE_584){
                printf("Status chip connection failed");
                return 0;
            }else{
                printf("\n Status OK  chip version %d ",versionInfo.chipVer);
                // printf("\n Status chip connection successfull! ");
                // printf("\n Status OK \n chip version %d \n MxlWare vers %d.%d.%d.%d.%d",versionInfo.chipVer,versionInfo.mxlWareVer[0],versionInfo.mxlWareVer[1],versionInfo.mxlWareVer[2],versionInfo.mxlWareVer[3],versionInfo.mxlWareVer[4]);
                // printf("\n \n Firmware Vers %d.%d.%d.%d.%d",versionInfo.firmwareVer[0],versionInfo.firmwareVer[1],versionInfo.firmwareVer[2],versionInfo.firmwareVer[3],versionInfo.firmwareVer[4]);
            return 1;
            }
        }
    }
    /*****************************************************************************/
    /*  function getMxlVersion                                                      */
    /*****************************************************************************/
    void  getMxlVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;     
        MXL_STATUS_E mxlStatus;
        MXL_HYDRA_VER_INFO_T versionInfo;
        char *mxl_ware_ver,*frm_ware_version;
        /***mxl connect***/
        MxL_GetVersion(&versionInfo);
        if(versionInfo.chipId != MXL_HYDRA_DEVICE_584){
            printf("Status chip connection failed");
            json["message"] = "MxL not connected!";
            json["error"]= true;
        }else{
            // printf("\n Status chip connection successfull! ");
            printf("chip version %d \n MxlWare vers %d.%d.%d.%d.%d",versionInfo.chipVer,versionInfo.mxlWareVer[0],versionInfo.mxlWareVer[1],versionInfo.mxlWareVer[2],versionInfo.mxlWareVer[3],versionInfo.mxlWareVer[4]);
            printf(" Firmware Vers %d.%d.%d.%d.%d",versionInfo.firmwareVer[0],versionInfo.firmwareVer[1],versionInfo.firmwareVer[2],versionInfo.firmwareVer[3],versionInfo.firmwareVer[4]);
            json["message"] = "MxL connected!";
            json["chip_version"] =versionInfo.chipVer;
            json["mxlware_vers"] = std::to_string(versionInfo.mxlWareVer[0])+'.'+std::to_string(versionInfo.mxlWareVer[1])+'.'+std::to_string(versionInfo.mxlWareVer[2])+'.'+std::to_string(versionInfo.mxlWareVer[3])+'.'+std::to_string(versionInfo.mxlWareVer[4]);
            json["firmware_vers"] = std::to_string(versionInfo.firmwareVer[0])+'.'+std::to_string(versionInfo.firmwareVer[1])+'.'+std::to_string(versionInfo.firmwareVer[2])+'.'+std::to_string(versionInfo.firmwareVer[3])+'.'+std::to_string(versionInfo.firmwareVer[4]);
            // json["frm_ware_ver"] = frm_ware_version;
            if(versionInfo.firmwareDownloaded){
                printf("\n Firmware Loaded True");
                json["FW Downloaded"] = true; 
            }else{
                printf("\n Firmware Loaded False");
                json["FW Downloaded"] = false;
            }
            json["error"]= false;
        }
        addToLog("getMxlVersion",request.body());
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function downloadFirmware                                                       */
    /*****************************************************************************/
    void  downloadFirmware(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        
        std::string para[] = {"mxl_id","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("downloadFirmware",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string str_mxl_id,str_rmx_no;
            str_mxl_id = getParameter(request.body(),"mxl_id");
            str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(str_mxl_id,1,1,6,1);
            error[1] = verifyInteger(str_rmx_no,1,1,6,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]="Required integer between 1-6!";
            }
            if(all_para_valid){ 
                json = downloadMxlFW(std::stoi(str_mxl_id),(std::stoi(str_rmx_no) - 1));
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value downloadMxlFW(int mxl_id,int rmx_no){
        Json::Value json;
        MXL_STATUS_E mxlStatus;
        MXL_HYDRA_VER_INFO_T versionInfo;
        int target =((0&0x3)<<8) | ((rmx_no&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
        if(connectI2Clines(target)){
            char cwd[1024];
            if(getcwd(cwd,sizeof(cwd)) != NULL)
                printf("CWD %s\n",cwd);
            else
                printf("CWD error\n");
            strcat(cwd,"/FW/MxL_5xx_FW.mbin");
            if (std::ifstream(cwd))
            {
                MxL_GetVersion(&versionInfo);
                if(versionInfo.chipId == MXL_HYDRA_DEVICE_584 && versionInfo.firmwareDownloaded){
                    printf("\n MXL already downloaded! ");
                    printf("\n Status OK \n chip version %d \n MxlWare vers %d.%d.%d.%d.%d",versionInfo.chipVer,versionInfo.mxlWareVer[0],versionInfo.mxlWareVer[1],versionInfo.mxlWareVer[2],versionInfo.mxlWareVer[3],versionInfo.mxlWareVer[4]);
                    printf("\n \n Firmware Vers %d.%d.%d.%d.%d",versionInfo.firmwareVer[0],versionInfo.firmwareVer[1],versionInfo.firmwareVer[2],versionInfo.firmwareVer[3],versionInfo.firmwareVer[4]);
                    json["message"] = "MxL connected!";
                    json["chip_version"] =versionInfo.chipVer;
                    json["mxlware_vers"] = std::to_string(versionInfo.mxlWareVer[0])+'.'+std::to_string(versionInfo.mxlWareVer[1])+'.'+std::to_string(versionInfo.mxlWareVer[2])+'.'+std::to_string(versionInfo.mxlWareVer[3])+'.'+std::to_string(versionInfo.mxlWareVer[4]);
                    json["firmware_vers"] = std::to_string(versionInfo.firmwareVer[0])+'.'+std::to_string(versionInfo.firmwareVer[1])+'.'+std::to_string(versionInfo.firmwareVer[2])+'.'+std::to_string(versionInfo.firmwareVer[3])+'.'+std::to_string(versionInfo.firmwareVer[4]); 
                    json["FW Downloaded"] = true; 
                }else{
                    printf("\n\n \t\t\t DOWNLOADING FIRMWARE PLEASE WAIT.... \n");
                    mxlStatus = App_FirmwareDownload(cwd);
                    if(mxlStatus != MXL_SUCCESS){
                        printf("Status FW download");
                    }else{
                        printf("Status FW download fail");
                    }
                    MxL_GetVersion(&versionInfo);
                    if(versionInfo.chipId != MXL_HYDRA_DEVICE_584){
                        printf("Status chip connection failed");
                        json["message"] = "MxL not connected!";
                        json["error"]= true;
                    }else{
                        printf("\n Status chip connection successfull! ");
                        printf("\n Status OK \n chip version %d \n MxlWare vers %d.%d.%d.%d.%d",versionInfo.chipVer,versionInfo.mxlWareVer[0],versionInfo.mxlWareVer[1],versionInfo.mxlWareVer[2],versionInfo.mxlWareVer[3],versionInfo.mxlWareVer[4]);
                        printf("\n \n Firmware Vers %d.%d.%d.%d.%d",versionInfo.firmwareVer[0],versionInfo.firmwareVer[1],versionInfo.firmwareVer[2],versionInfo.firmwareVer[3],versionInfo.firmwareVer[4]);
                        json["message"] = "MxL connected!";
                        json["chip_version"] =versionInfo.chipVer;
                        json["mxlware_vers"] = std::to_string(versionInfo.mxlWareVer[0])+'.'+std::to_string(versionInfo.mxlWareVer[1])+'.'+std::to_string(versionInfo.mxlWareVer[2])+'.'+std::to_string(versionInfo.mxlWareVer[3])+'.'+std::to_string(versionInfo.mxlWareVer[4]);
                        json["firmware_vers"] = std::to_string(versionInfo.firmwareVer[0])+'.'+std::to_string(versionInfo.firmwareVer[1])+'.'+std::to_string(versionInfo.firmwareVer[2])+'.'+std::to_string(versionInfo.firmwareVer[3])+'.'+std::to_string(versionInfo.firmwareVer[4]);
                        if(versionInfo.firmwareDownloaded){
                            printf("\n Firmware Loaded True");
                            json["FW Downloaded"] = true; 
                        }else{
                            printf("\n Firmware Loaded False");
                            json["FW Downloaded"] = false; 
                        }
                        json["error"]= false;
                    }
                }
            }else{
                string str(cwd);
                json["message"]= "FirmWare file path "+str+" does not exists!";
                json["error"]= true;
                printf("\n FW file path not exists \n");
            }
        }else{
            printf("\n Download failed: Connection error! \n");
            json["error"]= true;
            json["message"]= "Connection error!";
        }
        json["target"]= target;
        return json;
    }
    /*****************************************************************************/
    /*  function setMxlTunerOff                                                     */
    /*****************************************************************************/
    void  setMxlTunerOff(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        bool input_error;
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter; 
        MXL_STATUS_E mxlStatus;       
        std::string para[] = {"demod_id","mxl_id"};
        addToLog("setMxlTunerOff",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){  
            std::string demod_id = getParameter(request.body(),"demod_id");
            std::string mxl_id = getParameter(request.body(),"mxl_id");
            (verifyInteger(demod_id,1,1,INPUT_COUNT) != 1 && verifyInteger(mxl_id,1,1,6,1) != 1)? input_error = true,json["demod_id"] = "Required integer between 0-7",json["mxl_id"] = "Required integer between 1-6" : ((verifyInteger(demod_id,1,1,INPUT_COUNT) != 1 && verifyInteger(mxl_id,1,1,6,1) == 1)? json["demod_id"] = "Required integer between 0-7",input_error = true :((verifyInteger(demod_id,1,1,INPUT_COUNT) == 1 && verifyInteger(mxl_id,1,1,6,1) != 1)? json["mxl_id"] = "Required integer between 1-6",input_error = true : input_error = false));	
            if(!input_error){
            	int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((std::stoi(mxl_id)+6)&0xF)<<1) | (0&0x1);
        		if(connectI2Clines(target)){
	                mxlStatus = setTuneOff(std::stoi(demod_id));
	                if(mxlStatus==MXL_SUCCESS){
	                    json["status"]=1;
	                    json["message"]="Mxl tuner off successfull!";
	                    json["error"]=false;
	                }
	                else{
	                    json["status"]=0;
	                    json["message"]="Mxl tuner off failed!";
	                    json["error"]=true;
	                }
	            }else{
	            	json["error"]= true;
            		json["message"]= "Connection error!";
	            }
            }else{
                json["message"] = "Invalid Input!";
                json["error"] = true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function readWrite32bUdpCpu                                                     */
    /*****************************************************************************/
    void  readWrite32bUdpCpu(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"address","data","cs","mode"};
        addToLog("readWrite32bUdpCpu",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string address = getParameter(request.body(),"address"); 
            std::string data = getParameter(request.body(),"data"); 
            std::string cs = getParameter(request.body(),"cs"); 
            std::string mode = getParameter(request.body(),"mode"); 
            if(std::stoi(mode) == 1){
                if(write32bCPU(std::stoi(cs),std::stoi(address),std::stoi(data)) != -1){
                    std::cout<<"SC => "<<cs<<"ADDRESS => "<<address<<"DATA => "<<data;
                    json["error"]= false;
                    json["message"]= "Success!";
                }else{
                    json["error"]= true;
                    json["message"]= "Failed!";
                }
            }
            // injson["address"] = address;
            // injson["data"] = data;
            // injson["cs"] = cs;
            // uLen = c2.callCommand(90,RxBuffer,20,20,injson,std::stoi(mode));
            // if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            //     json["error"]= true;
            //     json["message"]= "STATUS COMMAND ERROR!";
            //     addToLog("readWrite32bUdpCpu","Error");
            // }else{
            //     json["done"] = RxBuffer[6];
            //     json["error"]= false;
            //     json["message"]= "set up IP destination!";
            //     json["data"] =  RxBuffer[8]<<24 | RxBuffer[9]<<16 | RxBuffer[8]<<8 | RxBuffer[9];
            //      addToLog("readWrite32bUdpCpu","Success");
            // }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  function readWrite32bUdpI2C                                                     */
    /*****************************************************************************/
    void  readWrite32bUdpI2C(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,injson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"address","data","cs","mode"};
        addToLog("readWrite32bUdpI2C",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string address = getParameter(request.body(),"address"); 
            std::string data = getParameter(request.body(),"data"); 
            std::string cs = getParameter(request.body(),"cs"); 
            std::string mode = getParameter(request.body(),"mode"); 
            injson["address"] = address;
            injson["data"] = data;
            injson["cs"] = cs;
            if(std::stoi(mode) == 1){
                if(write32bI2C(std::stoi(cs),std::stoi(address),std::stoi(data)) != -1){
                    std::cout<<"SC => "<<cs<<"ADDRESS => "<<address<<"DATA => "<<data;
                    json["error"]= false;
                    json["message"]= "Success!";
                }
            }else{
                json["resp"]=read32bI2C(std::stoi(cs),std::stoi(address));
                std::cout<<"CS => "<<cs<<"ADDRESS => "<<address;
            }
            // uLen = c2.callCommand(90,RxBuffer,20,20,injson,std::stoi(mode));
            // if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            //     json["error"]= true;
            //     json["message"]= "STATUS COMMAND ERROR!";
            //     addToLog("readWrite32bUdpI2C","Error");
            // }else{
            //     json["done"] = RxBuffer[6];
            //     json["error"]= false;
            //     json["message"]= "set up IP destination!";
            //     json["data"] =  RxBuffer[8]<<24 | RxBuffer[9]<<16 | RxBuffer[8]<<8 | RxBuffer[9];
            //      addToLog("readWrite32bUdpI2C","Success");
            // }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    /*****************************************************************************/
    /*  function selectInterface                                                     */
    /*****************************************************************************/
    void  selectInterface(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,injson;
        Json::FastWriter fastWriter;
        std::string para[] = {"spi","uart","i2c"};
        addToLog("selectInterface",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string spi = getParameter(request.body(),"spi"); 
            std::string uart = getParameter(request.body(),"uart"); 
            std::string i2c = getParameter(request.body(),"i2c"); 
            int target =((std::stoi(spi)&0x3)<<8) | ((std::stoi(uart)&0x7)<<5) | ((std::stoi(i2c)&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1){
                json["error"]= false;
                json["message"]= "Success!";
            }else{
            	json["error"]= true;
                json["message"]= "Fail!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    
    void initRedisConn()
    {
        // const char *hostname = "127.0.0.1";
        // std::cout<<cnf.REDIS_IP<<"****"<<cnf.REDIS_PORT<<"****"<<cnf.REDIS_PASS<<"****"<<std::endl;
        context = redisConnect(cnf.REDIS_IP.c_str(),cnf.REDIS_PORT);
        if (context->err) {
            if (context) {
                printf("Connection error: %s\n", context->errstr);
                redisFree(context);
            } else {
                printf("Connection error: can't allocate redis context\n");
            }
            std::cout<<"Redis Connection error, please check credentials " <<std::endl; 
            exit(1);
        }
        reply = (redisReply *)redisCommand(context, ("AUTH "+std::string(cnf.REDIS_PASS)).c_str());  
        CHECK(reply);
        std::cout<<"Redis Connection success " <<std::endl;
    }
    
    void storeEntry(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        auto name = request.param(":name").as<std::string>();
        Json::Value sum = db->getRecords();
        Json::FastWriter fastWriter;
        std::string output = fastWriter.write(sum);
        char h='h';
        response.send(Http::Code::Ok, output);//std::to_string(sum));
    }
    /*****************************************************************************/
    /*  Commande 0x00   function getFirmwareVersion                          */
    /*****************************************************************************/
    void getFirmwareVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetFirmwareVersion(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }

         auto cookies = request.cookies();
    // std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto& c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    // std::cout << "]" << std::endl;
    std::cout << request.body();

        json["request"]=request.body();
        json["response"]=response.body();
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetFirmwareVersion(int rmx_no ){
        unsigned char RxBuffer[10]={0};
        int uLen;
        Json::Value json;
        uLen = c1.callCommand(00,RxBuffer,10,10,json,0);
        if (RxBuffer[0] != STX || RxBuffer[3] != CMD_VER_FIMWARE || uLen != 10 || RxBuffer[9] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen == 5 ) {
                json["maj_ver"] = RxBuffer[4];
                json["min_ver"] = RxBuffer[5];
                json["standard"] = RxBuffer[6];
                json["cust_opt_id"] = RxBuffer[7];
                json["cust_opt_ver"] = RxBuffer[8];
                json["error"]= false;
                json["message"]= "Firmware verion!";
                db->addFirmwareDetails((int)RxBuffer[4],(int)RxBuffer[5],(int)RxBuffer[6],(int)RxBuffer[7],(int)RxBuffer[8]);
            }
            else {
                json["error"]= true;
               json["message"]= "STATUS COMMAND ERROR!"+uLen;
            }    
        }
        return json;
    }
    void getFirmwareVersions(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getFirmwareVersion",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                // iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                // if(iojson["error"]==false){
                    json = callGetFirmwareVersion(std::stoi(str_rmx_no));
                // }else{
                //     json = iojson;
                // }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x01   function getHardwareVersion                          */
    /*****************************************************************************/
    void getHardwareVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetHardwareVersion(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetHardwareVersion(int rmx_no ){
        unsigned char RxBuffer[15]={0};
        unsigned char MAJOR,MINOR,INPUT,OUTPUT,FIFOSIZE,OPTIONS,CORE_CLK,PRESENCE_SFN;
        double clk;
        Json::Value json;
        c1.callCommand(01,RxBuffer,15,5,json,0);
        int uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
        if (uLen >= 6 ) 
        {
            MAJOR = RxBuffer[6];
            std ::cout << "%s MAJOR==>"+MAJOR;
            MINOR = RxBuffer[7];
            INPUT = RxBuffer[4];
            OUTPUT = RxBuffer[5];
            FIFOSIZE = RxBuffer[8]<<8 | RxBuffer[9];
            if(uLen == 8) {
                PRESENCE_SFN = RxBuffer[10];   
                clk = (double)RxBuffer[11];
            } else {
                PRESENCE_SFN = 0;   
                clk = 125.0;
            }
            json["input"] = INPUT;
            json["output"] = OUTPUT;
            json["maj_ver"] = MAJOR;
            json["min_ver"] = MINOR;
            json["fifo_size"] = FIFOSIZE;
            json["option_imp"] = PRESENCE_SFN;
            json["core_clk"] = clk;
            json["error"]= false;
            json["message"]= "Hardware verion!";
            db->addHWversion((int)MAJOR,(int)MINOR,(int)INPUT,(int)OUTPUT,(int)FIFOSIZE,(int)PRESENCE_SFN,(double)clk);
            addToLog("getHardwareVersion","Success");
        }else{
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getHardwareVersion","Error");
        }
        return json;
    }
    void getHardwareVersions(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;        
        std::string para[] = {"rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getHardwareVersion",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                json = callGetHardwareVersion(std::stoi(str_rmx_no));
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x02   function getInputOutput                          */
    /*****************************************************************************/
    void getInputOutput(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetInputOutput(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        json["request"]=request.body();
        json["response"]=response.body();
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetInputOutput(int rmx_no){
        unsigned char RxBuffer[7]={0};
        int uLen;
        Json::Value json;        
        uLen = c1.callCommand(02,RxBuffer,7,7,json,0);
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 02 || uLen != 7 || RxBuffer[6] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getInputOutput","Error");
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 2 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getInputOutput","Error");
            }else{
                json["input"] = RxBuffer[4];
                json["output"] = RxBuffer[5]; 
                json["error"]= false;
                json["message"]= "Get input/output!";
                addToLog("getInputOutput","Success");          
            }
        } 
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x04   function getInputMode                          */
    /*****************************************************************************/
    void getInputMode(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetInputMode(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }     
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }    
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetInputMode(int rmx_no){
        unsigned char RxBuffer[20]= {0};
        Json::Value json;
        unsigned int SPTS,PMT,SID,RISE,MASTER,INSELECT,bitrate;
        int uLen;

        uLen = c1.callCommand(04,RxBuffer,20,20,json,0);
           if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 04 || uLen != 14 || RxBuffer[13] != ETX ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getInputMode","Error");
            }else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 9 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getInputMode","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Get input (SPTS/MPTS)!";
                    SPTS = RxBuffer[4];
                    json["spts"] = SPTS;
                    PMT = RxBuffer[5]<<8;
                    PMT |= RxBuffer[6];
                    json["pmt"] = PMT;
                    SID = RxBuffer[7]<<8;
                    SID|=RxBuffer[8];
                    json["sid"] = SID;
                    bitrate = (RxBuffer[9])<<24;
                    bitrate |= RxBuffer[10]<<16;
                    bitrate |= RxBuffer[11]<<8;
                    bitrate |= RxBuffer[12];
                    RISE= bitrate >> 31;
                    json["rise"] =RISE;
                    MASTER= ((bitrate<<1)>>31);
                    json["master"] =MASTER;
                    INSELECT= ((bitrate<<2)>>31);
                    json["inselect"] =INSELECT;
                    bitrate = (bitrate << 3);
                    json["bitrate"] = (bitrate >> 3);;
                    json["error"]= false;
                    json["message"]= "Get input/output!";  
                    addToLog("getInputMode","Success");          
                }
            }
        return json;
    }
    void getInputModes(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetInputMode(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getCore                          */
    /*****************************************************************************/
    void getCore(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"cs","address","rmx_no"};
        addToLog("getCore",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            int rmx_no = std::stoi(getParameter(request.body(),"rmx_no")) ;
            if(rmx_no > 0 && rmx_no <= 6) {
                 std::string cs = getParameter(request.body(),"cs"); 
                if(verifyInteger(cs) && std::stoi(cs) <=128){
                    std::string address = getParameter(request.body(),"address"); 
                    int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                    if(write32bCPU(0,0,target) != -1)
                        json = callGetCore(cs,address);
                    else{
                        json["error"]= true;
                        json["message"]= "Connection error!";   
                    }
                }else{
                    json["error"]= true;
                    json["cs"]= "Should be integer between 1 to 128!";
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid remux id";    
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetCore(std::string cs,std::string address)
    {
        unsigned char RxBuffer[20]={0};
        Json::Value jsonInput,json;
        jsonInput["address"] = address; 
        jsonInput["cs"] = cs;
        int uLen = c1.callCommand(05,RxBuffer,20,20,jsonInput,0);
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 05 || uLen != 9 || RxBuffer[8] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getCORE","Error");
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);    
            if (uLen != 4 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR! ";
                addToLog("getCORE","Error");  
            }else{
                 // long unsigned int  data = (long unsigned int)(RxBuffer[4] << 24) |(RxBuffer[5] << 16) |(RxBuffer[6] << 8) |(RxBuffer[7]);
                json["data"] =std::to_string(((long unsigned int)(RxBuffer[4]) << 24) | ((long unsigned int)(RxBuffer[5]) << 16) | ((long unsigned int)(RxBuffer[6]) << 8) |(long unsigned int)(RxBuffer[7]));
                json["error"]= false;
                json["message"]= "Get CORE!";  
                addToLog("getCORE","Success");
            }
        }
        return json;
    }

     /*****************************************************************************/
    /*  Commande 0x06   function getGPIO                          */
    /*****************************************************************************/
    void getGPIO(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        unsigned char MAJOR,MINOR,INPUT,OUTPUT,FIFOSIZE,OPTIONS,CORE_CLK,PRESENCE_SFN;
        double clk;
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){   
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                int uLen = c1.callCommand(06,RxBuffer,20,5,json,0);
                if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x06 || uLen != 9 || RxBuffer[8] != ETX ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getGPIO","Error");
                }else{
                    uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                    if (uLen != 4 ) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("getGPIO","Error");
                    }else{
                        json["error"] = false;
                        json["message"] = "GET GPIO!";
                        json["data"] = (RxBuffer[4] << 24) |(RxBuffer[5] << 16) |(RxBuffer[6] << 8) |(RxBuffer[7]);  
                        addToLog("getGPIO","Success");
                    }
                    
                }
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x07   function readI2c                          */
    /*****************************************************************************/
    void readI2c(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[10]={0};
        unsigned char MAJOR,MINOR,INPUT,OUTPUT,FIFOSIZE,OPTIONS,CORE_CLK,PRESENCE_SFN;
        double clk;
        Json::Value json;
        Json::FastWriter fastWriter;
         
        int uLen = c1.callCommand(07,RxBuffer,10,200,json,0);
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 07 || uLen != 5 || RxBuffer[4] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("readI2c","Error");
        }else{
            json["error"] = false;
            json["message"] = "Read I2c!";
            addToLog("readI2c","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

     /*****************************************************************************/
    /*  Commande 0x08   function readSFN                          */
    /*****************************************************************************/
    void readSFN(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[10]={0};
        unsigned char MAJOR,MINOR,INPUT,OUTPUT,FIFOSIZE,OPTIONS,CORE_CLK,PRESENCE_SFN;
        double clk;
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                int uLen = c1.callCommand(8,RxBuffer,10,10,json,0);
                if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 07 || uLen != 5 || RxBuffer[4] != ETX ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("readSFN","Error");
                }else{
                    json["error"] = false;
                    json["message"] = "Read SNF!";
                    addToLog("readSFN","Success");
                }
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

     /*****************************************************************************/
    /*  Commande 0x10   function getNITmode                          */
    /*****************************************************************************/
    void getNITmode(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(0 < rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetNITmode(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetNITmode(int rmx_no){
        Json::Value json;
        unsigned char RxBuffer[6]={0};
        int uLen = c1.callCommand(10,RxBuffer,6,6,json,0);
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 16 || RxBuffer[5] != ETX) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getNITMode","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 1) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getNITMode","Error");
            }else{
                json["error"] = false;
                json["message"] = "Read NIT Mode!";    
                json["NIT_mode"] = (RxBuffer[4]==0)?"Passthrough":((RxBuffer[4]==1)?"Channel Naming":((RxBuffer[4]==2)?"Host Provided":"No NIT"));
                json["NIT_code"] = RxBuffer[4]; 
                addToLog("getNITMode","Success");
            }
        }
        return json;
    }
    void getNITmodes(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getNITmodes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput("0",output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetNITmode(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x11   function getLCN                          */
    /*****************************************************************************/
    void getLCN(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetLCN(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetLCN(int rmx_no){
        unsigned char RxBuffer[1024]={0};
        Json::Value json,program_num,channel_num;
        int uLen = c1.callCommand(11,RxBuffer,1024,10,json,0);
            
            if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x11) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getLCN","Error");
            }else{
                uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
                uLen = uLen/4;
                for (int i = 0; i < uLen; ++i)
                {
                    program_num[i] = ((RxBuffer[i*4+4]<<8) | (RxBuffer[i*4+5]));
                    channel_num[i] = (((RxBuffer[i*4+6]<<8) | (RxBuffer[i*4+7]))>>7);   
                }
                json["error"] = false;
                json["message"] = "GET LCN!";
                json["program_num"] = program_num;
                json["channel_num"] = channel_num;
                addToLog("getLCN","Success");
            }
            return json;
    }
    void getLcn(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetLCN(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x12   function getHostNIT                          */
    /*****************************************************************************/
    void getHostNIT(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[6]={0};
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                int uLen = c1.callCommand(12,RxBuffer,6,6,json,0);
                
                if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x12 || uLen != 6 || RxBuffer[5] != ETX) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getHostNIT","Error");
                }else{
                    uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
                    if (uLen != 1 ) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("getHostNIT","Error");
                    }else{
                        json["error"] = false;
                        json["message"] = "GET HOST NIT!";
                        json["data"] = RxBuffer[4];
                        addToLog("getHostNIT","Success");
                    }
                }
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x   function getTSDetails
        1. getTsId
        2. getNITmode                     */
    /*****************************************************************************/
    void getTSDetails(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[12]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                Json::Value ts_json;
                if(iojson["error"]==false){
                    ts_json = callGetTsId(std::stoi(str_rmx_no));
                    if(ts_json["error"]==false){
                        json["uNet_Id"] = ts_json["uNet_Id"];
                        json["uOrigNet_Id"] = ts_json["uOrigNet_Id"];
                        json["uTS_Id"] = ts_json["uTS_Id"];
                    }
                    ts_json = callGetNITmode(std::stoi(str_rmx_no));
                    if(ts_json["error"]==false){
                        json["NIT_code"] = ts_json["NIT_code"];
                        json["NIT_mode"] = ts_json["NIT_mode"];
                        json["error"] = ts_json["error"];
                    }
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x14   function getTsId                          */
    /*****************************************************************************/
    void getTsId(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[12]={0};
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetTsId(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetTsId(int rmx_no){
        unsigned char RxBuffer[12]={0};
        Json::Value json;
        int uLen = c1.callCommand(14,RxBuffer,12,11,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x14 || uLen != 11 || RxBuffer[10] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getTsId","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 6 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getTsId","Error");
            }else{
                json["error"] = false;
                json["message"] = "GET TS ID!";
                json["uTS_Id"] = (RxBuffer[4]<<8)|RxBuffer[5];
                json["uNet_Id"] = (RxBuffer[6]<<8)|RxBuffer[7];
                json["uOrigNet_Id"] = (RxBuffer[8]<<8)|RxBuffer[9];
                addToLog("getTsId","Success");
            }
        }
        return json;
    }
    void getTSID(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[12]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput("0",output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetTsId(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x16   function getDvbSpiOutputMode                          */
    /*****************************************************************************/
    void getDvbSpiOutputMode(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[10]={0};
        Json::Value json;
        Json::FastWriter fastWriter;
         int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetDvbSpiOutputMode(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetDvbSpiOutputMode(int rmx_no){
        unsigned char RxBuffer[10]={0};
        Json::Value json;
        int uLen = c1.callCommand(16,RxBuffer,10,9,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x16 || uLen != 9 || RxBuffer[8] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getDvbSpiOutputMode","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 4 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getDvbSpiOutputMode","Error");
            }else{
                unsigned int data = ((RxBuffer[4]&0x3F)<<24)|(RxBuffer[5]<<16)|(RxBuffer[6]<<8)|(RxBuffer[7]);
                unsigned int mdata = data >> 1;
                unsigned int ndata = ~data;
                unsigned int fdata = ndata | 1;
                ndata = ~mdata;
                ndata = ndata | 1;
                json["error"] = false;
                json["message"] = "GET TS ID!";
                json["falling"] = (data & fdata);//(RxBuffer[4]&0x80)>>7;
                json["mode"] = (mdata & ndata);
                json["uRate"] = (data>>2);
                addToLog("getDvbSpiOutputMode","Success");
            }
        }
        return json;
    }
    void getDvbSpiOutputModes(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[10]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetDvbSpiOutputMode(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x17   function getPsiSiInterval                          */
    /*****************************************************************************/
    void getPsiSiInterval(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[15]={0};
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetPsiSiInterval(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetPsiSiInterval(int rmx_no){
        unsigned char RxBuffer[15]={0};
        Json::Value json;
        int uLen = c1.callCommand(17,RxBuffer,15,11,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x17 || uLen != 11 || RxBuffer[10] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getPsiSiInterval","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 6 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getPsiSiInterval","Error");
            }else{
                json["error"] = false;
                json["message"] = "GET PSI/SI Int!";
                json["pat_int"] = (RxBuffer[4]<<8)|RxBuffer[5];
                json["sdt_int"] = (RxBuffer[6]<<8)|RxBuffer[7];
                json["nit_int"] = (RxBuffer[8]<<8)|RxBuffer[9];
                addToLog("getPsiSiInterval","Success");
            }
        }
        return json;
    }
    void getPsiSiIntervals(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[15]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput("0",output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetPsiSiInterval(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x18   function getNetworkName                          */
    /*****************************************************************************/
    void getNetworkName(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json;
        unsigned char *name;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetNetworkName(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
           
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";   
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetNetworkName(int rmx_no){
        unsigned char RxBuffer[100]={0};
        Json::Value json;
        unsigned char *name;
        int uLen = c1.callCommand(18,RxBuffer,100,8,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x18) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getNetworkName","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            name = (unsigned char *)malloc(uLen + 1);
            json["error"] = false;
            json["message"] = "GET Network name!";
            std::string nName="";
            for (int i = 0; i<uLen; i++) {
                name[i] = RxBuffer[4 + i];
                nName=nName+getDecToHex((int)RxBuffer[4 + i]);
            }
            json["nName"] = hex_to_string(nName);
            addToLog("getNetworkName","Success");
        }
        return json;
        
    }
    void getNetworkname(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json,iojson;
        unsigned char *name;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getNetworkName",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetNetworkName(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x19   function getServiceName                          */
    /*****************************************************************************/
    void getServiceName(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json;
        Json::Value jsonInput;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                std::string progNumber = request.param(":uProg").as<std::string>();
                json = callGetServiceName(rmx_no,progNumber);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"]= true;
            json["message"]= "Invalid remux id!";   
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
        
    }
    Json::Value callGetServiceName(int rmx_no,std::string progNumber){
        unsigned char RxBuffer[100]={0};
        Json::Value json,jsonInput;
        Json::Reader reader;
        unsigned char *name;
        jsonInput["uProg"] = progNumber;
        int uLen = c1.callCommand(19,RxBuffer,100,8,jsonInput,0);

        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x19) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getServiceName","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            name = (unsigned char *)malloc(uLen + 1);
            json["error"] = false;
            json["message"] = "GET Channel name!";
            std::string nName="";
            for (int i = 0; i<uLen; i++) {
                name[i] = RxBuffer[4 + i];
                nName=nName+getDecToHex((int)RxBuffer[4 + i]);
            }
            if(nName == "")
                json["nName"] = -1;
            else
                json["nName"] = hex_to_string(nName);
            addToLog("getServiceName","Success");
        }
       return json;
    }
    void getServicename(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json,jsonInput,iojson;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServicename",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[3] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetServiceName(std::stoi(str_rmx_no),progNumber);
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x21   function getDynamicStateWinSize                          */
    /*****************************************************************************/
    void getDynamicStateWinSize(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]={0};
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetDynamicStateWinSize(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetDynamicStateWinSize(int rmx_no){ 
        unsigned char RxBuffer[1024]={0};
        Json::Value json;
        int uLen = c1.callCommand(21,RxBuffer,1024,10,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x21) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getDynamicStateWinSize","Error");
        }else{
            json["error"] = false;
            json["message"] = "Dynamic state win size!";
            json["data"] = RxBuffer[4];
            addToLog("getDynamicStateWinSize","Success");
        }
        return json;
    }
    void getDynamicStateWinsize(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]={0};
        Json::Value json ,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDynamicStateWinSize",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetDynamicStateWinSize(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x23   function getFilterCA                          */
    /*****************************************************************************/
    void getFilterCA(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetFilterCA(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetFilterCA(int rmx_no){
        unsigned char RxBuffer[200]={0};
        Json::Value json;
        unsigned char *progNumber;
        int uLen = c1.callCommand(23,RxBuffer,200,20,json,0);
        
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x23 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getFilterCA","Error");
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            json["error"] = false;
            json["message"] = "Filtering CAT EMM and specific ECM!";
            if(uLen ==0) {
                json["ModeCAT"] = 0;
                json["length"] = 0;
            }
            json["ModeCAT"] = RxBuffer[4];

            int length = (uLen-1)/2;
            std::string pnum="";
            progNumber = (unsigned char *)malloc(length + 1);
            for(int i=0; i<length; i++) {
                progNumber[i] = (RxBuffer[2*i+5]<<8)|(RxBuffer[2*i+6]);
                pnum = pnum+getDecToHex((RxBuffer[2*i+5]<<8)|(RxBuffer[2*i+6]));
            }
            json["pnum"] = pnum;
            addToLog("getFilterCA","Success");
        }
        return json;
    }
    void getFilterCAS(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getFilterCA",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetFilterCA(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x24   function getServiceID                          */
    /*****************************************************************************/
    void getServiceID(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetServiceID(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetServiceID(int rmx_no){
        unsigned char RxBuffer[200]={0};
        Json::Value json,oldProgNumber,newProgNumber;
        unsigned char *name;
        int length=0;
        int uLen = c1.callCommand(24,RxBuffer,200,20,json,0);
        
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x24 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getServiceID","Error");
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            length = uLen/4;

            for(int i=0; i<length; i++) {
                oldProgNumber[i] = (RxBuffer[4*i+4]<<8)|RxBuffer[4*i+5];
                newProgNumber[i] = (RxBuffer[4*i+6]<<8)|RxBuffer[4*i+7];
            }
            json["error"] = false;
            json["oldProgNumber"]=oldProgNumber;
            json["newProgNumber"]=newProgNumber;
            json["message"] = "GET Service nos!";
            addToLog("getServiceID","Success");
        }
        return json;
    }
    void getServiceIDs(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServiceID",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetServiceID(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    /*****************************************************************************/
    /*  Commande 0x   function getTableDetails  
        1. getTablesVersion
        2. getPsiSiInterval                   */
    /*****************************************************************************/
    void getTableDetails(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[15]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getTablesVersion",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                Json::Value table_json;
                if(iojson["error"]==false){
                    table_json = callGetTablesVersion(std::stoi(str_rmx_no));
                    if(table_json["error"]==false){
                        json["nit_isenable"] = table_json["nit_isenable"];
                        json["nit_ver"] = table_json["nit_ver"]; 
                        json["pat_isenable"] = table_json["pat_isenable"];
                        json["pat_ver"] = table_json["pat_ver"];
                        json["sdt_isenable"] = table_json["sdt_isenable"];
                        json["sdt_ver"] = table_json["sdt_ver"];
                    }
                    table_json = callGetPsiSiInterval(std::stoi(str_rmx_no));
                    if(table_json["error"]==false){
                        json["nit_int"] = table_json["nit_int"];
                        json["pat_int"] = table_json["pat_int"];
                        json["sdt_int"] = table_json["sdt_int"];
                        json["error"] = table_json["error"];
                    }
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x25   function getTablesVersion                          */
    /*****************************************************************************/
    void getTablesVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[15]={0};
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetTablesVersion(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetTablesVersion(int rmx_no){
        unsigned char RxBuffer[15]={0};
        Json::Value json;
        int uLen = c1.callCommand(25,RxBuffer,15,11,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x25 || uLen != 8 || RxBuffer[7] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getTablesVersion","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 3 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }else{
                unsigned pat=(RxBuffer[4]);
                unsigned sdt=(RxBuffer[5]);
                unsigned nit=(RxBuffer[6]);
                unsigned pat_isenable =((1 << 7) & pat)>>7;
                unsigned sdt_isenable =((1 << 7) & sdt)>>7;
                unsigned nit_isenable =((1 << 7) & nit)>>7;
                pat &= ~(1<<7);
                sdt &= ~(1<<7);
                nit &= ~(1<<7);
                json["error"] = false;
                json["message"] = "Get table version!";
                json["pat_ver"] = pat;
                json["sdt_ver"] = sdt;
                json["nit_ver"] = nit;
                json["pat_isenable"] = pat_isenable;
                json["sdt_isenable"] = sdt_isenable;
                json["nit_isenable"] = nit_isenable;
                
                addToLog("getTablesVersion","Success");
            }
        }
        return json;
    }
    void getTablesVersions(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[15]={0};
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getTablesVersion",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput("0",output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetTablesVersion(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x30   function getSmoothFilterInfo                        */
    /*****************************************************************************/
    void getSmoothFilterInfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetSmoothFilterInfo(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetSmoothFilterInfo(int rmx_no){
        unsigned char RxBuffer[9]={0};        
        Json::Value json;
        int uLen = c1.callCommand(30,RxBuffer,9,5,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x30 || uLen != 9 || RxBuffer[8] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getSmoothFilterInfo","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 4 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }else{
                json["error"] = false;
                json["message"] = "Get smooth filter information!";
                json["uCurLevel"] = ((RxBuffer[4]<<8) | RxBuffer[5]);
                json["uMaxLevel"] = ((RxBuffer[6]<<8) | RxBuffer[7]);
                json["status"] = 1;
                addToLog("getSmoothFilterInfo","Success");
            }
        }
        return json;
    }
    void getSmoothFilterinfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getSmoothFilterInfo",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetSmoothFilterInfo(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x31   function getDataflowRates                        */
    /*****************************************************************************/
    void getDataflowRates(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetDataflowRates(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetDataflowRates(int rmx_no){
        unsigned char RxBuffer[17]={0};
        Json::Value json;
        unsigned char *l188;
        unsigned char *l204;
        unsigned char *stream;
        unsigned int *uInuputRate;
        unsigned int *uPayloadRate;
        unsigned int *uOutuputRate;
        int uLen = c1.callCommand(31,RxBuffer,17,5,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x31 || uLen != 17 || RxBuffer[16] != ETX) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getDataflowRates","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 12 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getDataflowRates","Error");
            }else{
                json["error"] = false;
                json["message"] = "Get smooth filter information!";
                json["l188"] = (RxBuffer[4] & 0x40) >> 6;
                json["l204"] = (RxBuffer[4] & 0x20) >> 5;
                json["stream"] = (RxBuffer[4] & 0x10) >> 4;
                json["uInuputRate"] = (((RxBuffer[4] & 0x0F) << 24) | (RxBuffer[5] << 16) | (RxBuffer[6] << 8) | RxBuffer[7]);
                json["uPayloadRate"] = ((RxBuffer[8] << 24) | (RxBuffer[9] << 16) | (RxBuffer[10] << 8) | RxBuffer[11]);
                json["uOutuputRate"] = ((RxBuffer[12] << 24) | (RxBuffer[13] << 16) | (RxBuffer[14] << 8) | RxBuffer[15]);
                json["status"] = 1;
                addToLog("getDataflowRates","Success");
            }
        }
       return json;
    }
    void getDataFlowRates(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDataflowRates",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetDataflowRates(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x32,   function getAllInputServices                        */
    /*****************************************************************************/
    void getAllInputServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson,progList,list;
        Json::FastWriter fastWriter;
        int is_error=0;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            for (int input = 0; input <= INPUT_COUNT; ++input)
            {
                iojson=callSetInputOutput(std::to_string(input),"0",rmx_no);
                if(iojson["error"]==false){
                    progList = callGetProgramList(input,rmx_no);
                    // std::cout<<"================>>"<<progList["progNums"].size()<<std::endl;
                    if(progList["error"]==false){
                        for (int i = 0; i < progList["progNums"].size(); ++i)
                        {
                            Json::Value progName,progProvider,progInfo,progDetails;
                            std::string progNumber = std::to_string(progList["progNums"][i].asInt());
                            progDetails["input"] = input;
                            progDetails["progNum"] = progNumber;
                            progName= callGetProgramOriginalName(progNumber,rmx_no);
                            if(progName["error"]==false){
                                progDetails["originalName"] = progName["name"];
                            }else{
                                progDetails["originalName"] = "NoName";
                            }
                            progProvider = callGetProgramOriginalProviderName(progNumber,rmx_no);
                            if(progProvider["error"]==false){
                                progDetails["providerName"] = progProvider["name"];
                            }else{
                                progDetails["providerName"] = "NoName";
                            }
                            progInfo = callGetPrograminfo(progNumber,rmx_no);
                            if(progInfo["error"]==false){
                                progDetails["PIDS"] = progInfo["PIDS"];
                                progDetails["Type"] = progInfo["Type"];
                                progDetails["Number_of_program"] = progInfo["Number_of_program"];
                                progDetails["Band"] = progInfo["Band"];
                            }else{
                                Json::Value empty;
                                progDetails["PIDS"] = empty;
                                progDetails["Type"] = empty;
                                progDetails["Number_of_program"] = empty;
                                progDetails["Band"] = empty;
                            }
                            list.append(progDetails);
                        }
                    }
                }else{
                    json["error"] = true;
                    json["message"] = iojson["message"];
                }
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        if(list.size()>0){
            std::cout<<"================>> No of Programs"<<list.size()<<std::endl;
            json["error"] = false;
            json["list"] = list;
            json["message"] = "Program List";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x32   function getProgramList                        */
    /*****************************************************************************/
    void getProgramList(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            iojson =callGetInputOutput(rmx_no);
            if(iojson["error"]==false){
                json = callGetProgramList(iojson["input"].asInt(),rmx_no);
            }else{
               json = iojson; 
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetProgramList(int input,int rmx_no){
        unsigned char RxBuffer[1024]={0};
        
        Json::Value json,iojson;
        Json::Value ProgNum,ProgNames;
        Json::Value band;
        Json::Value uishared;
        int uNumProg;
        unsigned short *uProgNum; 
        unsigned short *uBand; 
        unsigned char *uiShared;
        iojson=callSetInputOutput(std::to_string(input),"0",rmx_no);
        if(iojson["error"]==false)  {
            int uLen = c1.callCommand(32,RxBuffer,1024,5,json,0);

            if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x32) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getProgramList","Error");
            }else{
                uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
                if (!uLen) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getProgramList","Error");
                }else{
                    json["error"] = false;
                    json["message"] = "Get program list!";
                    uProgNum = (unsigned short *)malloc(uLen*sizeof(unsigned short));
                    uBand = (unsigned short *)malloc(uLen*sizeof(unsigned short));
                    uiShared = (unsigned char *)malloc(uLen*sizeof(unsigned char));

                    uProgNum[0] = 8191;         // NULL
                    uBand[0] = ((RxBuffer[4] << 8) | RxBuffer[5]);
                    uiShared[0] = 0;
                    uProgNum[1] = 0;            // PAT
                    uBand[1] = ((RxBuffer[6] << 8) | RxBuffer[7]);
                    uiShared[1] = 0;
                    //prog list
                    uLen -= 4;
                    uNumProg = (uLen /= 4);
                    for (int i = 0; i<uLen; i++) {
                        // uProgNum[2 + i] = ((RxBuffer[8 + i * 4] << 8) | RxBuffer[8 + i * 4 + 1]);
                        int pnumber = ((RxBuffer[8 + i * 4] << 8) | RxBuffer[8 + i * 4 + 1]); 
                        ProgNum[i]= pnumber;
                        // uBand[2 + i] = (((RxBuffer[8 + i * 4 + 2] & 0x7F) << 8) | RxBuffer[8 + i * 4 + 3]);
                        band[i] = (((RxBuffer[8 + i * 4 + 2] & 0x7F) << 8) | RxBuffer[8 + i * 4 + 3]);
                        // uiShared[2 + i] = ((RxBuffer[8 + i * 4 + 2] & 0x80) >> 7);
                        uishared[i] = ((RxBuffer[8 + i * 4 + 2] & 0x80) >> 7);
                        Json::Value prog_name =  callGetProgramOriginalName(std::to_string(pnumber),rmx_no);
                        if(prog_name["error"] == false){
                           std::string str_name = prog_name["name"].asString();
                           str_name = 'S'+str_name;
                           str_name.erase(std::remove_if(str_name.begin(), str_name.end(),[](char c) {
                                   if(!isalnum(c) && c != ' ' && !isalpha(c)){return true;}}),str_name.end());
                           ProgNames.append( str_name);
                       }else{
                           ProgNames.append("NoName");
                       }
                        db->addChannelList(input,ProgNum[i].asInt(),rmx_no);
                    }
                    // for (int i = 0; i < ProgNum.size(); ++i)
                    // {
                    //     db->addChannelList(input,ProgNum[i].asInt(),rmx_no);
                    //     // std::cout<<"channel number"<<ProgNum[i]<<endl;
                    // }
                    json["prog_names"] = ProgNames;
                    json["progNums"] = ProgNum;
                    json["uband"] = band;
                    json["uishared"] = uishared;
                    json["status"] = 1;
                    addToLog("getProgramList","Success");
                }
            }
        }else{
           json = iojson;
        }
        return json;
    }
    void getProgramsList(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getProgramsList",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
               json = callGetProgramList(std::stoi(input),std::stoi(str_rmx_no));
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x33   function getCryptedProg                        */
    /*****************************************************************************/
    void getCryptedProg(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if( rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetCryptedProg(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetCryptedProg(int rmx_no){
        unsigned char RxBuffer[1024]={0};
        
        Json::Value json;
        Json::Value progNum;
        Json::Value service_type,encrypted_flag;
        int uLen = c1.callCommand(33,RxBuffer,1024,5,json,0);

        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x33) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getCryptedProg","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
           if (RxBuffer[4 + uLen] != ETX) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getCryptedProg","Error");
            }else{
                json["error"] = false;
                json["message"] = "Get encrypted program list!";
                json["NumProg"] = (uLen /= 4);
                for (int i = 0; i<uLen; i++) {
                        progNum[i] = ((RxBuffer[4 + i * 4] << 8) | RxBuffer[4 + i * 4 + 1]);
                        //service_type[i] = ((RxBuffer[4 + i * 4 + 2] << 8) | RxBuffer[4 + i * 4 + 3]);
                        service_type[i] = RxBuffer[4 + i * 4 + 2];
                        encrypted_flag[i] = RxBuffer[4 + i * 4 + 3];
                    }
                json["progNums"] = progNum;
                json["service_type"] = service_type;
                json["encrypted_flag"] = encrypted_flag;
                addToLog("getCryptedProg","Success");
            }
        }
        return json;
    }
    void getCryptedPrograms(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getCryptedProg",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetCryptedProg(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     /*****************************************************************************/
    /*  Commande 0x34   function getChannelDetails  Combination of follwing api's
        1. getProgramOriginalProviderName
        2. getPrograminfo
        3. getPmtAlarm
        4. getDataflowRates
        5. getServiceProvider                 */
    /*****************************************************************************/
    void getChannelDetails(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json,jsonInput,iojson;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServicename",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[3] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                iojson=callSetInputOutput(input,output,rmx_no);
                Json::Value grog_info_json;
                if(iojson["error"]==false){
                    grog_info_json = callGetProgramOriginalProviderName(progNumber,rmx_no);
                    if(grog_info_json["error"]==false){
                        json["original_provider_name"] = grog_info_json["name"];
                    }
                    grog_info_json = callGetPrograminfo(progNumber,rmx_no);
                    if(grog_info_json["error"]==false){
                        json["no_of_program"] = grog_info_json["Number_of_program"];
                        json["band"] = grog_info_json["Band"];
                        json["type"] = grog_info_json["Type"];
                        json["PIDS"] = grog_info_json["PIDS"];
                    }
                    grog_info_json = callGetPmtAlarm(rmx_no);
                    if(grog_info_json["error"]==false){
                        json["affected_input"] = grog_info_json["Affected_input"];
                        json["progNumber"] = grog_info_json["ProgNumber"];
                    }
                    grog_info_json = callGetDataflowRates(rmx_no);
                    if(grog_info_json["error"]==false){
                        json["status"] = grog_info_json["status"];
                        json["stream"] = grog_info_json["stream"];
                        json["uInuputRate"] = grog_info_json["uInuputRate"];
                        json["uOutuputRate"] = grog_info_json["uOutuputRate"];
                        json["uPayloadRate"] = grog_info_json["uPayloadRate"];
                        json["l188"] = grog_info_json["l188"];
                        json["l204"] = grog_info_json["l204"];
                        json["error"] = grog_info_json["error"];
                    }
                    grog_info_json = callGetServiceProvider(rmx_no,progNumber);
                    if(grog_info_json["error"]==false){
                        json["provider_name"] = grog_info_json["pName"];
                    }
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x34   function getPrograminfo                          */
    /*****************************************************************************/
    void getPrograminfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                auto uProg = request.param(":uProg").as<std::string>();
                json = callGetPrograminfo(uProg,rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetPrograminfo(std::string uProg,int rmx_no){
        unsigned char RxBuffer[1024]= {0};
        int uNumProg,i,j,uLen;
        Json::Value uProgNum,uType,uShared,uBand,json,jsonMsg;
        char *name;

        jsonMsg["uProg"] = uProg;
        uLen = c1.callCommand(34,RxBuffer,1024,8,jsonMsg,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] !=  CMD_GET_STAT_PROG) {        
            json["error"] = true;
            json["message"] = "STATUS COMMAND ERROR!";
            addToLog("getPrograminfo","Error");
        }else{
            j = 0;
            for (i = 4; i<uLen - 1; i += 6) {
                uProgNum[j] = RxBuffer[i] << 8 | RxBuffer[i + 1];
                uType[j] = RxBuffer[i + 2];
                uShared[j] = RxBuffer[i + 3];
                uBand[j] = RxBuffer[i + 4] << 8 | RxBuffer[i + 5];
                j++;
            }
            json["PIDS"] = uProgNum;
            json["Type"] = uType;
            json["Number_of_program"] = uShared;
            json["Band"] = uBand;
            json["error"] = false;
            json["message"] = "program information!";
            addToLog("getPrograminfo","Success");
        }
        return json;
    }
    void getProgramInfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[100]={0};
        Json::Value json,jsonInput,iojson;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServicename",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[3] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetPrograminfo(progNumber,std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x35   function getAllReferencedPIDInfo                        */
    /*****************************************************************************/
    void getAllReferencedPIDInfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter; 
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){  
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetAllReferencedPIDInfo(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetAllReferencedPIDInfo(int rmx_no){
        unsigned char RxBuffer[255 * 1024]={0};
        
        Json::Value json,pids,types,noOfProg,bandwidth;
        int uLen = c1.callCommand(35,RxBuffer,255 * 1024,11,json,0);
       
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x35 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getAllReferencedPIDInfo","Error"); 
        }else{
            int i = 0;
            int j = 0;
            for (i = 4; i<uLen; i += 4) {
                pids[j] = RxBuffer[i] << 8 | RxBuffer[i + 1];
                types[j] = RxBuffer[i + 2];
                noOfProg[j] = RxBuffer[i + 3];
                bandwidth[j] = RxBuffer[i + 4];
                j++;
            }
            json["pids"] = pids;
            json["types"] = types;
            json["noOfProg"] = noOfProg;
            json["bandwidth"] = bandwidth;
            json["error"] = false;
            json["message"] = "Get PID information!";
            addToLog("getAllReferencedPIDInfo","Success");
        }
        return json;
    }
    void getAllReferencedPidInfo(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter; 
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getAllReferencedPidInfo",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetAllReferencedPIDInfo(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x37   function getEraseCAMod                        */
    /*****************************************************************************/
    void getEraseCAMod(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetEraseCAMod(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetEraseCAMod(int rmx_no){
        unsigned char RxBuffer[256]={0};
        Json::Value json;
        Json::Value progNum;
        int length;
        int uLen = c1.callCommand(37,RxBuffer,256,10,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x37) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getEraseCAMod","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            length = uLen/2;
            for(int i=0; i<length; i++) {
                progNum[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
            }
            json["progNum"] = progNum;
            json["error"] = false;
            json["message"] = "Get erase CA mode!";
            addToLog("getEraseCAMod","Success");
        }
        return json;
    }
    void getEraseCAmod(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getEraseCAmod",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetEraseCAMod(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x37   function setEraseCAMod                        */
    /*****************************************************************************/
    void setEraseCAMod(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"programNumber","input","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getEraseCAMod",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
                std::string rmx_no = getParameter(request.body(),"rmx_no"); 
                std::string programNumber = getParameter(request.body(),"programNumber"); 
                std::string input = getParameter(request.body(),"input"); 
                std::string output = getParameter(request.body(),"output"); 
                error[0] = verifyInteger(programNumber,6);
                error[1] = verifyInteger(input,1,1,INPUT_COUNT);
                error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
                error[3] = verifyInteger(output,1,1,RMX_COUNT,1);;
               
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= (i == 0)? "Require Integer between 1 to 65535!" :((i==3)? "Require Integer between 1 to 6!":"Require Integer between 0 to 3!");
                }
                if(all_para_valid){
                    json = callSetEraseCAMod(programNumber,input,output,std::stoi(rmx_no));
                }else{
                    json["error"]= true;
                }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetEraseCAMod(std::string programNumber,std::string input,std::string output, int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,paraJson;
        Json::Value root; 
        std::cout<<programNumber<<"----------->>"<<input<<std::endl;
        Json::Value iojson = callSetInputOutput(input,output,rmx_no); 

        if(iojson["error"]==false){ 
            Json::Value freeca_services = db->getFreeCAModePrograms(programNumber,input,output);
            std::cout<<input<<"----------->>"<<programNumber;
            if(freeca_services["error"]==false){
                for (int i = 0; i < freeca_services["list"].size(); ++i)
                {
                    root.append(freeca_services["list"][i]["program_number"].asString());
                    std::cout<<" "<<freeca_services["list"][i]["program_number"].asString()<<" ";
                }
            }
            std::cout<<std::endl;
            root.append(programNumber);
            paraJson["programNumbers"] = root; 
            int msgLen=((root.size())*2)+4;
            uLen = c1.callCommand(37,RxBuffer,6,msgLen,paraJson,1);
                         
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_PROG_ERASE_CA || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getEraseCAMod","Error");
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getEraseCAMod","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Erase CA mode!";  
                    db->addfreeCAModePrograms(programNumber,input,output,rmx_no);
                    addToLog("getEraseCAMod","Error");        
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x37   function eraseCAMod                           */
    /*****************************************************************************/
    void  eraseCAMod(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =  callEraseCAMod(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callEraseCAMod(int rmx_no){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,paraJson;
        Json::Value root,root2;  
        Json::Reader reader;
        uLen = c1.callCommand(37,RxBuffer,20,200,paraJson,2);      
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_PROG_ERASE_CA || RxBuffer[4]!=0 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("EraseCAMod","Error");
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("EraseCAMod","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "All service IDs reinitiated!!!";
                db->deletefreeCAModePrograms();   
                addToLog("EraseCAMod","Success");       
            }
        } 
        return json;
    }
    void  eraseCAmod(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("eraseCAMod",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callEraseCAMod(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x39   function getStatPID                        */
    /*****************************************************************************/
    void getStatPID(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pNbOccurence;
        Json::Value pNbCrypt;
        Json::Value pNbCCnt;
        int num_prog;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                int uLen = c1.callCommand(39,RxBuffer,4090,7,json,0);
                
                if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x39) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("getStatPID","Error");       
                }else{
                    uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
                    num_prog = uLen/9;
                    for(int i=0; i<num_prog; i++) {
                        pNbOccurence[i] = (RxBuffer[9*i+4]<<16)|RxBuffer[9*i+5]<<8|RxBuffer[9*i+6];
                        pNbCrypt[i] = (RxBuffer[9*i+7]<<16)|RxBuffer[9*i+8]<<8|RxBuffer[9*i+9];
                        pNbCCnt[i] = (RxBuffer[9*i+10]<<16)|RxBuffer[9*i+11]<<8|RxBuffer[9*i+12];
                    }
                    json["pNbOccurence"] = pNbOccurence;
                    json["pNbCrypt"] = pNbCrypt;
                    json["pNbCCnt"] = pNbCCnt;
                    json["error"] = false;
                    json["message"] = "Get stat PID!";
                    addToLog("getStatPID","Success");       
                }
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x40   function getProgActivation                        */
    /*****************************************************************************/
    void getProgActivation(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =callGetProgActivation(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetProgActivation(int rmx_no){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,jpnames;
        int num_prog;
        int uLen = c1.callCommand(40,RxBuffer,4090,7,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x40) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getProgActivation","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            num_prog = uLen/2;
            Json::Value jsonNewIds;
            jsonNewIds = callGetServiceID(rmx_no);

            for(int i=0; i<num_prog; i++) {
                Json::Value jsondata,jservName;
                // pProg[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
                int pnum = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
                jpnames = callGetProgramOriginalName(std::to_string(pnum),rmx_no);
                if(jpnames["error"] == false){
                    jsondata["original_name"] = jpnames["name"];
                }else{
                    jsondata["original_name"] = -1;
                }
                jservName = callGetServiceName(rmx_no,std::to_string(pnum));
                if(jservName["error"]==false){
                    jsondata["new_name"] = jservName["nName"];
                }else{
                    jsondata["new_name"] = -1;
                }
                jsondata["original_service_id"] = pnum;
                jsondata["new_service_id"] = getNewServiceIdIfExists(jsonNewIds,pnum);

                pProg[i] = jsondata;
            }
            json["pProg"] = pProg;
            json["error"] = false;
            json["message"] = "Get program activation!";
            addToLog("getProgActivation","Success");
        }
        return json;
    }
    int getNewServiceIdIfExists(Json::Value jsonNewIds,int progNum){
        int serviceId=-1;
        if(jsonNewIds["error"]==false){
            Json::Value oldProgNumber = jsonNewIds["oldProgNumber"];
            Json::Value newProgNumber = jsonNewIds["newProgNumber"];
            if(oldProgNumber.size() > 0){
                for (int i = 0; i < oldProgNumber.size(); ++i)
                {
                    if(progNum != oldProgNumber[i].asInt())
                        continue;
                    serviceId = newProgNumber[i].asInt();
                }   
            }
        }
        return serviceId;
    }
    void getActivatedProgs(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getProgActivation",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetProgActivation(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    void getAllActivatedServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson,progArray;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getProgActivation",request.body());
        std::string str_rmx_no;
        int rmx_no;

        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            if(verifyInteger(str_rmx_no,1,1,RMX_COUNT,1)){
                for (int input = 0; input <= INPUT_COUNT; ++input)
                {
                    for (int output = 0; output <= OUTPUT_COUNT; ++output)
                    {
                        rmx_no = std::stoi(str_rmx_no);
                        Json::Value jsonObj;
                        iojson=callSetInputOutput(std::to_string(input),std::to_string(output),rmx_no);
                        if(iojson["error"]==false){
                            Json::Value progList = callGetProgActivation(rmx_no);
                            if(progList["error"] == false){
                                progArray.append(progList["pProg"]);
                            }else{
                                progArray.append(jsonObj);
                            }
                        }else{
                            progArray.append(jsonObj);
                        }
                    }
                }
                json["error"] = false;
                json["prog_array_list"] = progArray; 
                json["message"] = "Active service list!";
            }else{
                json["error"]= true;
                json["message"]= "Require Integer between 1-6!"; 
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    void getKeptProgramFromOutput(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getKeptProgramFromOutput",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-7!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
               json = callGetKeptProgramFromOutput(str_rmx_no,output);
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetKeptProgramFromOutput(std::string str_rmx_no,std::string output){
    	 unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,jpnames;
        int num_prog;
        bool error_flag=false;
        for (int input = 0; input <= INPUT_COUNT; ++input)
        {
        	Json::Value iojson = callSetInputOutput(std::to_string(input),output,std::stoi(str_rmx_no));
        	 if(iojson["error"]==false){
		        int uLen = c1.callCommand(40,RxBuffer,4090,7,json,0);
		        
		        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x40) {
		            json["error"]= true;
		            json["message"]= "STATUS COMMAND ERROR!";
		            addToLog("getProgActivation","Error");
		            error_flag=true;
		        }else{
		            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
		            num_prog = uLen/2;
		            Json::Value jsonNewIds;

		            for(int i=0; i<num_prog; i++) {
		                Json::Value jsondata,jservName;
		                pProg.append((RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5]);
		            }
		        }
	        }else{
	        	error_flag=true;
	        }
        }
        json["pProg"] = pProg;
        json["error"] =error_flag;
        json["message"] = "Get program activation!";
        addToLog("getProgActivation","Success");
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x41   function getLockedPIDs                          */
    /*****************************************************************************/
    void getLockedPIDs(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetLockedPIDs(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetLockedPIDs(int rmx_no){
        unsigned char RxBuffer[10]={0};
        
        Json::Value json,pProg;
        int num_prog;
        int uLen = c1.callCommand(41,RxBuffer,10,10,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x41) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getLockedPIDs","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            num_prog = uLen/2;
            for(int i=0; i<num_prog; i++) {
                pProg[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
            }
            json["pProg"] = pProg;
            json["error"] = false;
            json["message"] = "Get clocked PID!";
            addToLog("getLockedPIDs","Success");
            // json["pid"] = RxBuffer[4];
        }
        return json;
    }
    void getLockedPids(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getLockedPIDs",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetLockedPIDs(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x41   function flushLockedPIDs                           */
    /*****************************************************************************/
    void  flushLockedPIDs(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =callFlushLockedPIDs(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }       
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callFlushLockedPIDs(int rmx_no){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json,paraJson;
        Json::Value root,root2;  
        uLen = c1.callCommand(41,RxBuffer,6,6,paraJson,2);      
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x41 || RxBuffer[4]!=1 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("flushLockedPIDs","Error");
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("flushLockedPIDs","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "All locked PIDs reinitiated!!!"; 
                db->deleteLockedPrograms();    
                addToLog("flushLockedPIDs","Success");     
            }
        }
        return json;
    }
    void  flushLockedPids(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
            int error[ sizeof(para) / sizeof(para[0])];
            bool all_para_valid=true;
            addToLog("flushLockedPIDs",request.body());
            std::string input, output,str_rmx_no;
            std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
            if(res=="0"){
                str_rmx_no = getParameter(request.body(),"rmx_no");
                input = getParameter(request.body(),"input");
                output = getParameter(request.body(),"output");
                error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
                error[1] = verifyInteger(input,1,1,INPUT_COUNT);
                error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
                }
                if(all_para_valid){
                    iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                    if(iojson["error"]==false){
                        json = callFlushLockedPIDs(std::stoi(str_rmx_no));
                    }else{
                        json = iojson;
                    }
                }      
            }else{
                json["error"]= true;
                json["message"]= res;
            }        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x42   function getHighPriorityServices                          */
    /*****************************************************************************/
    void getHighPriorityServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetHighPriorityServices(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetHighPriorityServices(int rmx_no){
         unsigned char RxBuffer[10]={0};
        
        Json::Value json,pProg;
        int num_prog;
        int uLen = c1.callCommand(42,RxBuffer,10,10,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x42) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getHighPriorityServices","Error"); 
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            num_prog = uLen/2;
            for(int i=0; i<num_prog; i++) {
                pProg[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
            }
            json["pProg"] = pProg;
            json["error"] = false;
            json["message"] = "Get high priority service!";
            addToLog("getHighPriorityServices","Success"); 
            // json["pid"] = RxBuffer[4];
        }
        return json;
    }
    void getHighPriorityService(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getHighPriorityServices",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetHighPriorityServices(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x42   function flushHighPriorityServices                           */
    /*****************************************************************************/
    void  flushHighPriorityServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callFlushHighPriorityServices(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
           
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }       
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callFlushHighPriorityServices(int rmx_no){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json,paraJson;
        uLen = c1.callCommand(42,RxBuffer,6,6,paraJson,2);      
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x42 || RxBuffer[4]!=1 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("flushHighPriorityServices","Error"); 
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("flushHighPriorityServices","Error"); 
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "All locked PIDs reinitiated!!!"; 
                db->deleteHighPriorityServices();         
                addToLog("flushHighPriorityServices","Success"); 
            }
        }
        return json;
    }
    void  flushHighPriorityService(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("flushHighPriorityServices",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callFlushHighPriorityServices(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }     
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x53   function getPsiSiDecodingStatus                           */
    /*****************************************************************************/
    void getPsiSiDecodingStatus(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetPsiSiDecodingStatus(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetPsiSiDecodingStatus(int rmx_no){
        unsigned char RxBuffer[10]={0};
        Json::Value json;
        int uLen = c1.callCommand(53,RxBuffer,10,5,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x53 || uLen != 6 || RxBuffer[5] != ETX) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getPsiSiDecodingStatus","Error"); 
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 1) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getPsiSiDecodingStatus","Error"); 
            }else{
                json["error"] = false;
                json["message"] = "PsiSi Decoding Status!"; 
                unsigned short resp = RxBuffer[4];   
                unsigned short fifo = resp << 12;
                json["FIFO_threshold_alarm"] = fifo >> 12;  
                unsigned short pat = ((resp >> 7)<<15);
                unsigned short sdt = ((resp >> 6)<<15);
                unsigned short all_pmt = ((resp >> 5)<<15);
                unsigned short overflow_fifo = ((resp >> 4)<<15);
                json["overflow_fifo"] = overflow_fifo >>15;
                json["all_pmt_tables"] = all_pmt >> 15;
                json["sdt_table"] = sdt >> 15;
                json["pat_table"] = pat >> 15;
                addToLog("getPsiSiDecodingStatus","Success"); 
            }
        }
        return json;
    }
    void getPSISIDecodingStatus(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getPsiSiDecodingStatus",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetPsiSiDecodingStatus(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x54   function getTSFeatures                           */
    /*****************************************************************************/
    void getTSFeatures(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetTSFeatures(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetTSFeatures(int rmx_no){
        unsigned char RxBuffer[20]={0};
        
        Json::Value json;
        int uLen = c1.callCommand(54,RxBuffer,20,5,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x54 || uLen != 15 || RxBuffer[14] != ETX) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getTSFeatures","Error"); 
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            if (uLen != 10) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getTSFeatures","Error"); 
            }else{
                json["error"] = false;
                json["message"] = "Get TS features!";    
                json["ts_id"] = RxBuffer[4] << 8 | RxBuffer[5];
                json["version"] = RxBuffer[6];
                json["net_id"] = RxBuffer[7] << 8 | RxBuffer[8];
                json["originNetID"] = RxBuffer[9] << 8 | RxBuffer[10];
                json["numberOfProg"] = RxBuffer[11] << 8 | RxBuffer[12];
                unsigned short resp = RxBuffer[13];
                unsigned short pat = ((resp >> 7)<<15);
                unsigned short sdt = ((resp >> 6)<<15);
                unsigned short nit = ((resp >> 5)<<15);
                json["nit_available"] = nit >> 15;
                json["sdt_available"] = sdt >> 15;
                json["pat_available"] = pat >> 15;
                addToLog("getTSFeatures","Success"); 
            }
        }
        return json;
    }
    void getTsFeatures(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getTSFeatures",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetTSFeatures(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x1B (27)  function getLcnProvider                          */
    /*****************************************************************************/
    void getLcnProvider(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetLcnProvider(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
           
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    Json::Value callGetLcnProvider(int rmx_no){
        unsigned char RxBuffer[1024]= {0};        
        int uLen;
        Json::Value json,jsonMsg;
        uLen = c1.callCommand(27,RxBuffer,1024,8,jsonMsg,0);
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 27 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getLcnProvider","Error"); 
        }else{
            json["error"]= false;
            json["message"]= "LCN provider!";
            json["provID"] = RxBuffer[4]; 
            addToLog("getLcnProvider","Success"); 
        }
        return json;
    }
    void getLcnProviders(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getLcnProvider",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            // input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            // error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-3!" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                // iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                // if(iojson["error"]==false){
                    json = callGetLcnProvider(std::stoi(str_rmx_no));
                // }else{
                //     json = iojson;
                // }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x60   function getProgramOriginalName                          */
    /*****************************************************************************/
    void getProgramOriginalName(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                auto uProg = request.param(":uProg").as<std::string>();
                json = callGetProgramOriginalName(uProg,rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetProgramOriginalName(std::string uProg,int rmx_no){
        unsigned char RxBuffer[1024]= {0};
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        jsonMsg["uProg"] = uProg;
        uLen = c1.callCommand(60,RxBuffer,1024,8,jsonMsg,0);
        
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x60 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getProgramOriginalName","Error"); 
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            name = (char *)malloc(uLen);
            sprintf(name, "%s", (char*) &RxBuffer[4]);
            json["error"]= false;
            json["message"]= "get the name";
            json["name"] = name; 
            addToLog("getProgramOriginalName","Success"); 
        }
        return json;
    }
    void getProgramOriginalname(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServicename",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,"0",std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetProgramOriginalName(progNumber,std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x61   function getProgramOriginalProviderName                          */
    /*****************************************************************************/
    void getProgramOriginalProviderName(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                auto uProg = request.param(":uProg").as<std::string>();
                json = callGetProgramOriginalProviderName(uProg,rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetProgramOriginalProviderName(std::string uProg,int rmx_no){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        jsonMsg["uProg"] = uProg;

        uLen = c1.callCommand(61,RxBuffer,1024,8,jsonMsg,0);
        
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x61 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getProgramOriginalProviderName","Error"); 
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            name = (char *)malloc(uLen);
            sprintf(name, "%s", (char*) &RxBuffer[4]);
            json["error"]= false;
            json["message"]= "Get Program Original Provider Name!";
            json["name"] = name; 
            addToLog("getProgramOriginalProviderName","Success"); 
        }
        return json;
    }
    void getProgramOriginalProvidername(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","output","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServicename",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[3] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,output,std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetProgramOriginalProviderName(progNumber,std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x62   function getProgramOriginalNetworkName                          */
    /*****************************************************************************/
    void getProgramOriginalNetworkName(const Rest::Request& request, Net::Http::ResponseWriter response){
       Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                auto uProg = request.param(":uProg").as<std::string>();
                json = callGetProgramOriginalNetworkName(rmx_no,uProg);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetProgramOriginalNetworkName(int rmx_no,std::string progNumber){
         unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        jsonMsg["uProg"] = progNumber;
        uLen = c1.callCommand(62,RxBuffer,1024,8,jsonMsg,0);
        
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x62 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getProgramOriginalNetworkName","Error"); 
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            name = (char *)malloc(uLen);
            sprintf(name, "%s", (char*) &RxBuffer[5]);
            json["error"]= false;
            json["message"]= "get the name";
            json["name"] = name; 
            json["nit_ver"] = RxBuffer[4]; 
            addToLog("getProgramOriginalNetworkName","Success"); 
        }
        return json;
    }
     void getProgramOriginalNetworkname(const Rest::Request& request, Net::Http::ResponseWriter response){
       Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","progNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getProgramOriginalNetworkName",request.body());
        std::string input, output,str_rmx_no,progNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            progNumber = getParameter(request.body(),"progNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(progNumber);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-3!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,"0",std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetProgramOriginalNetworkName(std::stoi(str_rmx_no),progNumber);
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x29   function getAffectedOutputServices                          */
    /*****************************************************************************/
    void getAffectedOutputServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg,root,progNum;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        //jsonMsg["length"] = getParameter(request.body(),"length"); 
        std::string para[] = {"service_list","rmx_no"}; //{"service_list":["9001"]}
        addToLog("getAffectedOutputServices",request.body()); 
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            int rmx_no = std::stoi(getParameter(request.body(),"rmx_no")) ; 
            if(rmx_no > 0 && rmx_no <= 6)
            {
                std::string service_list = getParameter(request.body(),"service_list"); 
                std::string serviceList = UriDecode(service_list);
                bool parsedSuccess = reader.parse(serviceList,root,false);
                if (parsedSuccess)
                {
                    if(verifyJsonArray(root,"service_list",1)){
                        int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                        if(write32bCPU(0,0,target) != -1) {
                            jsonMsg["service_list"] = root["service_list"];
                            uLen = c1.callCommand(29,RxBuffer,1024,1024,jsonMsg,0);
                            
                            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x29 ) {
                                json["error"]= true;
                                json["message"]= "STATUS COMMAND ERROR!";
                                addToLog("getAffectedOutputServices","Error"); 
                            }else{
                                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                                json["error"]= parsedSuccess;
                                json["message"]= "Affected service list.";
                                uLen /= 2;
                                for (int i = 0; i<uLen; i++) {
                                    progNum[i] = ((RxBuffer[4 + i * 2] << 8) | RxBuffer[4 + i * 2 + 1]);
                                }
                                json["services"] = progNum; 
                                addToLog("getAffectedOutputServices","Success"); 
                            }
                        }else{
                            json["error"]= true;
                            json["message"]= "Connection error!";
                        }
                    }else{
                        json["error"]= true;
                        json["message"]= Invalid_json;
                    }
                }
            }else{
                json["error"] = false;
                json["message"] = "Invalid remux id";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    //***********************SET COMMANDS *****************************************/


    
    /*****************************************************************************/
    /*  Commande 0x02   function setInputOutput                          */
    /*****************************************************************************/
    void setInputOutput(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[7]={0};
        int uLen;
        std::string in,out;
        Json::Value json;
        Json::FastWriter fastWriter;
        
        std::string para[] = {"input","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        addToLog("setInputOutput",request.body());
        bool all_para_valid=true;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no");

            in = getParameter(request.body(),"input");
            out = getParameter(request.body(),"output"); 
            
            error[0] = verifyInteger(in,1,1,INPUT_COUNT);
            error[1] = verifyInteger(out,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==2)? "Require Integer between 1-6!": "Require Integer between 0-3!";
            }
            if(all_para_valid){
                json = callSetInputOutput(in,out,std::stoi(rmx_no));
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }

            auto cookies = request.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto& c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
    // std::cout << req.body();

        json["response"]=response.body();
        json["request"]=request.body();

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetInputOutput(std::string in, std::string out,int rmx_no){
        unsigned char RxBuffer[7]={0};
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;

        if(is_numeric(in) && is_numeric(out)){
            if(std::stoi(in)<=INPUT_COUNT && std::stoi(out)<=OUTPUT_COUNT){
                jsonMsg["input"]=in;
                jsonMsg["output"]=out;
                int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((0)&0xF)<<1) | (0&0x1);

                if(write32bCPU(0,0,target) != -1) {
                    uLen = c1.callCommand(02,RxBuffer,6,7,jsonMsg,1);
                    if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 02 || uLen != 6 || RxBuffer[4]!=1 || RxBuffer[5] != ETX ) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                    }else{
                        uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                        if (uLen != 1 ) {
                            json["error"]= true;
                            json["message"]= "STATUS COMMAND ERROR!";
                            addToLog("setInputOutput","Error");
                        }else{
                            json["status"] = RxBuffer[4]; 
                            json["error"]= false;
                            json["message"]= "SET input/output!";  
                            addToLog("setInputOutput","Success");        
                        }
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }else{
                json["error"]= true;
                json["message"]= "input/output should be in range 0-3!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Validate input/output parameters!";
        }
        return json;
    }

    /*****************************************************************************/
    /*  Commande 0x04   function setInputMode                          */
    /*****************************************************************************/
    void setInputMode(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        std::string SPTS,PMT,SID,RISE,MASTER,INSELECT,BITRATE,input,rmx_no;
        Json::Value json;
        Json::FastWriter fastWriter;
        std::string para[] = {"spts","pmt","sid","rise","master","inselect","bitrate","input","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setInputMode",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            SPTS =getParameter(request.body(),"spts");
            PMT = getParameter(request.body(),"pmt");
            SID = getParameter(request.body(),"sid");
            RISE = getParameter(request.body(),"rise");
            MASTER = getParameter(request.body(),"master");
            INSELECT = getParameter(request.body(),"inselect");
            BITRATE = getParameter(request.body(),"bitrate");
            input =getParameter(request.body(),"input");
            rmx_no = getParameter(request.body(),"rmx_no"); 

            error[0] = verifyInteger(SPTS,1,1);
            error[1] = verifyInteger(PMT);
            error[2] = verifyInteger(SID);
            error[3] = verifyInteger(RISE,1,1);
            error[4] = verifyInteger(MASTER,1,1);
            error[5] = verifyInteger(INSELECT,1,1);
            error[6] = verifyInteger(BITRATE);
            error[7] = verifyInteger(input,1,1,INPUT_COUNT);
            error[8] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=8)? "Require Integer!":"Require Integer between 1 to 6!";
            }
            if(all_para_valid){
                json = callSetInputMode(SPTS,PMT,SID,RISE,MASTER,INSELECT,BITRATE,input,std::stoi(rmx_no));
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetInputMode(std::string SPTS,std::string PMT,std::string SID,std::string RISE,std::string MASTER,std::string INSELECT,std::string BITRATE,std::string input,int rmx_no){
        unsigned char RxBuffer[20]= {0};
        unsigned char* ss;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::Value iojson = callSetInputOutput(input,"0",rmx_no);
        if(iojson["error"]==false){
            jsonMsg["SPTS"] = SPTS;
            jsonMsg["PMT"] = PMT;   
            jsonMsg["SID"] = SID;
            jsonMsg["RISE"] = RISE;
            jsonMsg["MASTER"] = MASTER;
            jsonMsg["INSELECT"] = INSELECT;
            jsonMsg["BITRATE"] = BITRATE;
            uLen = c1.callCommand(04,RxBuffer,20,20,jsonMsg,1);
           if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 04 || uLen != 6 || RxBuffer[4]!=1 || RxBuffer[5] != ETX ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setInputMode","Error");
                }else{
                    ss=(unsigned char*)(jsonMsg["SPTS"].asString()).c_str();
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set input mode!";   
                    db->addInputMode(SPTS,PMT,SID,RISE,MASTER,INSELECT,BITRATE,input,rmx_no);  
                    addToLog("setInputMode","Success");     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson;
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x1B (27)  function setLcnProvider                          */
    /*****************************************************************************/
    void setLcnProvider(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        std::string pro_id;

        std::string para[] = {"provId","rmx_no"};
        addToLog("setLcnProvider",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ; 
            if(verifyInteger(rmx_no,1,1,RMX_COUNT)==1)
            {
                pro_id = getParameter(request.body(),"provId");
                if(verifyInteger(pro_id)){
                    int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                    if(write32bCPU(0,0,target) != -1) {
                        json = callSetLcnProvider(pro_id,std::stoi(rmx_no));
                    }else{
                        json["error"]= true;
                        json["message"]= "Connection error!";
                    }
                }else{
                    json["error"]= true;
                    json["provId"]= "Required Integer!";
                }
            }else{
                json["error"] = false;
                json["message"] = "Invalid remux id";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetLcnProvider(std::string pro_id,int rmx_no){
        unsigned char RxBuffer[20]= {0};
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        jsonMsg["provId"] = pro_id;
        uLen = c1.callCommand(27,RxBuffer,6,6,jsonMsg,1);
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 27 || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setLcnProvider","Error");
            }else{
                json["error"]= false;
                json["message"]= "Setting LCN provider!";
                json["status"] = RxBuffer[4]; 
                int row=db->addLcnProviderid(std::stoi(pro_id),rmx_no);
                addToLog("setLcnProvider","Success");
            }
        }
        return json;    
    }
    /*****************************************************************************/
    /*  Commande 0x15  function setCreateAlarmFlags                          */
    /*****************************************************************************/
    void setCreateAlarmFlags(const Rest::Request& request, Net::Http::ResponseWriter response){

        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string FIFO_Threshold0,FIFO_Threshold1,FIFO_Threshold2,FIFO_Threshold3,mode,rmx_no;
        std::string para[] = {"FIFO_Threshold0","FIFO_Threshold1","FIFO_Threshold2","FIFO_Threshold3","mode","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCreateAlarmFlags",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            mode = getParameter(request.body(),"mode");
            FIFO_Threshold0 = getParameter(request.body(),"FIFO_Threshold0");
            FIFO_Threshold1 = getParameter(request.body(),"FIFO_Threshold1");
            FIFO_Threshold2 = getParameter(request.body(),"FIFO_Threshold2");
            FIFO_Threshold3 = getParameter(request.body(),"FIFO_Threshold3");
            rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(FIFO_Threshold0);
            error[1] = verifyInteger(FIFO_Threshold1);
            error[2] = verifyInteger(FIFO_Threshold2);
            error[3] = verifyInteger(FIFO_Threshold3);
            error[4] = verifyInteger(mode,1,1);
            error[5] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==5)? "Require Integer between 1 to 6!":"Require Integer!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    json = callCreateAlarmFlags(FIFO_Threshold0,FIFO_Threshold1,FIFO_Threshold2,FIFO_Threshold3,mode,std::stoi(rmx_no));
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callCreateAlarmFlags(std::string FIFO_Threshold0,std::string FIFO_Threshold1,std::string FIFO_Threshold2,std::string FIFO_Threshold3,std::string mode,int rmx_no){
        unsigned char RxBuffer[20]= {0};
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        jsonMsg["mode"] = mode;
        jsonMsg["FIFO_Threshold0"] = FIFO_Threshold0;
        jsonMsg["FIFO_Threshold1"] = FIFO_Threshold1;
        jsonMsg["FIFO_Threshold2"] = FIFO_Threshold2;
        jsonMsg["FIFO_Threshold3"] = FIFO_Threshold3;
        uLen = c1.callCommand(15,RxBuffer,6,14,jsonMsg,1);
        
       if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x15 || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setCreateAlarmFlags","Error");
            }else{
                json["error"]= false;
                json["message"]= "Setting Fifo Tresh!";
                json["status"] = RxBuffer[4]; 
                db->createAlarmFlags(mode,FIFO_Threshold0,FIFO_Threshold1,FIFO_Threshold2,FIFO_Threshold3,rmx_no);
                addToLog("setCreateAlarmFlags","Success");
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x25  function setTablesVersion                          */
    /*****************************************************************************/
    void setTablesVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]= {0};
        
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,output,rmx_no;
        std::string para[] = {"pat_ver","pat_isenable","sdt_ver","sdt_isenable","nit_ver","nit_isenable","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setTablesVersion",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            rmx_no = getParameter(request.body(),"rmx_no") ; 
            if(verifyInteger(rmx_no,1,1,RMX_COUNT)==1)
            {
                pat_ver = getParameter(request.body(),"pat_ver");
                pat_isenable = getParameter(request.body(),"pat_isenable");
                sdt_ver = getParameter(request.body(),"sdt_ver");
                sdt_isenable =  getParameter(request.body(),"sdt_isenable");
                nit_ver = getParameter(request.body(),"nit_ver");
                nit_isenable = getParameter(request.body(),"nit_isenable");
                output = getParameter(request.body(),"output");
                
                error[0] = verifyInteger(pat_ver,0,0,15,0);
                error[1] = verifyInteger(pat_isenable,1,1,1,0);
                error[2] = verifyInteger(sdt_ver,0,0,15,0);
                error[3] = verifyInteger(sdt_isenable,1,1,1,0);
                error[4] = verifyInteger(nit_ver,0,0,15,0);
                error[5] = verifyInteger(nit_isenable,1,1,1,0);
                error[6] = verifyInteger(output,1,1,OUTPUT_COUNT);
                error[7] = 1;
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= ((i+1)%2==0)? "Require Integer between 0 to 1!" : "Require Integer between 0 to 15!";
                }
                if(all_para_valid){
                    json = callSetTablesVersion(pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,output,std::stoi(rmx_no));
                    if(json["error"] == false)
                    	db->addTablesVersion(pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,output,std::stoi(rmx_no));
                }
            }else{
                json["error"] = false;
                json["message"] = "Invalid remux id";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetTablesVersion(std::string pat_ver,std::string pat_isenable,std::string sdt_ver,std::string sdt_isenable,std::string nit_ver,std::string nit_isenable,std::string output,int rmx_no){
        unsigned char RxBuffer[20]= {0};
        
        int uLen;
        Json::Value json,iojson;
        Json::Value jsonMsg;
        jsonMsg["pat_ver"] = pat_ver;
        jsonMsg["pat_isenable"] = pat_isenable;
        jsonMsg["sdt_ver"] = sdt_ver;
        jsonMsg["sdt_isenable"] =sdt_isenable;
        jsonMsg["nit_ver"] = nit_ver;
        jsonMsg["nit_isenable"] = nit_isenable;
        iojson = callSetInputOutput("0",output,rmx_no);
        if(iojson["error"]==false){
            uLen = c1.callCommand(25,RxBuffer,6,11,jsonMsg,1);
           
           if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x25 ||  RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setTablesVersion","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Setting table version!";
                    json["status"] = RxBuffer[4];
                    addToLog("setTablesVersion","Success");
                }
            }
        }else{
            json=iojson;
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x25  function setTablesVersionToAllRmx                          */
    /*****************************************************************************/
    void setTablesVersionToAllRmx(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]= {0};
        
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,output,rmx_no;
        std::string para[] = {"pat_ver","pat_isenable","sdt_ver","sdt_isenable","nit_ver","nit_isenable"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,error_on_update_all=false;
        addToLog("setTablesVersionToAllRmx",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
                pat_ver = getParameter(request.body(),"pat_ver");
                pat_isenable = getParameter(request.body(),"pat_isenable");
                sdt_ver = getParameter(request.body(),"sdt_ver");
                sdt_isenable =  getParameter(request.body(),"sdt_isenable");
                nit_ver = getParameter(request.body(),"nit_ver");
                nit_isenable = getParameter(request.body(),"nit_isenable");
                
                error[0] = verifyInteger(pat_ver,0,0,15,0);
                error[1] = verifyInteger(pat_isenable,1,1,1,0);
                error[2] = verifyInteger(sdt_ver,0,0,15,0);
                error[3] = verifyInteger(sdt_isenable,1,1,1,0);
                error[4] = verifyInteger(nit_ver,0,0,15,0);
                error[5] = verifyInteger(nit_isenable,1,1,1,0);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= ((i+1)%2==0)? "Require Integer between 0 to 1!" : "Require Integer between 0 to 15!";
                }
                if(all_para_valid){
                	for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
                	{
                		for (int output = 0; output < OUTPUT_COUNT; ++output)
                		{
                			Json::Value json_table_ver = callSetTablesVersion(pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,std::to_string(output),rmx_no);		
                			if(json_table_ver["error"] == true)
                				error_on_update_all= true;
                		}
                	}
                	if(error_on_update_all){
                		json["error"] = true;
                		json["message"] = "Partially update";
                	}else{
                		 db->addTablesVersion(pat_ver,pat_isenable,sdt_ver,sdt_isenable,nit_ver,nit_isenable,"0",-1);
                		json["error"] = false;
                		json["message"] = "Table version updated successfully";
                	}
                }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x11   function setLcn                           */
    /*****************************************************************************/
     void setLcn(const Rest::Request& request, Net::Http::ResponseWriter response){        
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"programNumber","channelNumber","input","rmx_no","output","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setLcn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no") ; 
            std::string programNumber = getParameter(request.body(),"programNumber"); 
            std::string channelNumber = getParameter(request.body(),"channelNumber"); 
            std::string input = getParameter(request.body(),"input"); 
            std::string output = getParameter(request.body(),"output"); 
            std::string addFlag = getParameter(request.body(),"addFlag");

            error[0] = verifyInteger(programNumber);
            error[1] = verifyInteger(channelNumber);
            error[2] = verifyInteger(input,1,1,INPUT_COUNT);
            error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[4] = verifyInteger(output,1,1,INPUT_COUNT);
            error[5] = verifyInteger(addFlag,1,1,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 2)? ("Require Integer between 0 - "+std::to_string(INPUT_COUNT)+"!").c_str() :((i==3)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 65535!");
            }
            if(all_para_valid){
            	int flag = std::stoi(addFlag);
                json = callSetLcn(programNumber,channelNumber,input,std::stoi(rmx_no),output,flag);
                if(json["error"] == false)
                	db->addLcnNumbers(programNumber,channelNumber,input,std::stoi(rmx_no),flag);
            }else{
                json["error"]= true;
            }    
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetLcn(std::string programNumber,std::string channelNumber,std::string input,int rmx_no,std::string output,int addflag){
        unsigned char RxBuffer[6]={0};
        Json::Value json,paraJson;
        Json::Value root,root2; 
        Json::Value iojson = callSetInputOutput(input,output,rmx_no);
        if(iojson["error"]==false){
            Json::Value existing_lcn = db->getLcnNumbers(input,programNumber);
            std::cout<<input<<"----------->>"<<programNumber;
            if(existing_lcn["error"]==false){
                for (int i = 0; i < existing_lcn["list"].size(); ++i)
                {
                    root.append(existing_lcn["list"][i]["program_number"].asString());
                    root2.append(existing_lcn["list"][i]["channel_number"].asString());
                    std::cout<<" "<<existing_lcn["list"][i]["program_number"].asString()<<" ";
                }
            }
            
            if(addflag){
	            root.append(programNumber);
	            root2.append(channelNumber);
	        }
	        std::cout<<root<<std::endl;
            paraJson["uProg"] = root;
            paraJson["uChan"] =root2;
            int uLen = c1.callCommand(11,RxBuffer,6,1024,paraJson,1);
            
            if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x11 || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }else{
                uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
                if (uLen != 1) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setLcn","Error");
                }else{
                    json["error"] = false;
                    json["message"] = "Set LCN!";    
                    json["status"] = RxBuffer[4];
                    
                    addToLog("setLcn","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    

    /*****************************************************************************/
    /*  Commande 0x41   function setLockedPIDs                           */
    /*****************************************************************************/
    void  setLockedPIDs(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"programNumber","input","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setLockedPIDs",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string programNumbers = getParameter(request.body(),"programNumber"); 
            std::string input = getParameter(request.body(),"input"); 
            error[0] = verifyInteger(programNumbers,6);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer  between 1 to 65535!" :(i==1)? "Require Integer "+std::to_string(INPUT_COUNT)+"!" : "Require Integer  between 1 to 6!";
            }
            if(all_para_valid){
                json = callSetLockedPIDs(programNumbers,input,std::stoi(rmx_no));
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetLockedPIDs(std::string programNumber,std::string input,int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,paraJson;
        Json::Value root;  
        Json::Value iojson = callSetInputOutput(input,"0",rmx_no); 
        if(iojson["error"]==false){
            Json::Value existing_services = db->getLockedPids(input,programNumber);
            std::cout<<input<<"----------->>"<<programNumber;
            if(existing_services["error"]==false){
                for (int i = 0; i < existing_services["list"].size(); ++i)
                {
                    root.append(existing_services["list"][i]["program_number"].asString());
                    std::cout<<" "<<existing_services["list"][i]["program_number"].asString()<<" ";
                }
            }
            std::cout<<std::endl;
            root.append(programNumber);
            paraJson["programNumbers"] = root; 
            int msgLen=((root.size())*2)+4;
            uLen = c1.callCommand(41,RxBuffer,6,msgLen,paraJson,1);
                         
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_SET_FILT || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!"+getDecToHex((int)RxBuffer[3]);
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setLockedPIDs","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set locked PID!"; 
                    db->addLockedPrograms(programNumber,input,rmx_no); 
                    addToLog("setLockedPIDs","Success");        
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x42   function setHighPriorityServices                           */
    /*****************************************************************************/
    void  setHighPriorityServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"programNumber","input","rmx_no"};
         int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setHighPriorityServices",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]) );
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string programNumbers = getParameter(request.body(),"programNumber"); 
            std::string input = getParameter(request.body(),"input"); 
            error[0] = verifyInteger(programNumbers,6);
            error[1] = verifyInteger(input,1,1);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer  between 1 to 65535!" :(i==1)? "Require Integer "+std::to_string(INPUT_COUNT)+"!" : "Require Integer  between 1 to 6!";
            }
            if(all_para_valid){
                json = callSetHighPriorityServices(programNumbers,input,std::stoi(rmx_no));
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetHighPriorityServices(std::string programNumbers,std::string input,int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,paraJson;
        Json::Value root; 
        Json::Value iojson = callSetInputOutput(input,"0",rmx_no); 
        if(iojson["error"]==false){
            Json::Value existing_services = db->getHighPriorityServices(input,programNumbers);
            std::cout<<input<<"----------->>"<<programNumbers;
            if(existing_services["error"]==false){
                for (int i = 0; i < existing_services["list"].size(); ++i)
                {
                    root.append(existing_services["list"][i]["program_number"].asString());
                    std::cout<<" "<<existing_services["list"][i]["program_number"].asString()<<" ";
                }
            }
            std::cout<<std::endl;
            root.append(programNumbers);
            paraJson["programNumbers"] = root; 
            uLen = c1.callCommand(42,RxBuffer,6,6,paraJson,1);
                         
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!"+getDecToHex((int)RxBuffer[3]);
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setHighPriorityServices","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set setHighPriorityServices!";    
                    db->addHighPriorityServices(programNumbers,input,rmx_no);     
                    addToLog("setHighPriorityServices","Success");   
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x10   function setNITmode                           */
    /*****************************************************************************/
    void  setNITmode(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        

        std::string para[] = {"mode","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setNitMode",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string mode = getParameter(request.body(),"mode"); 
            std::string output = getParameter(request.body(),"output");
            std::string rmx_no =getParameter(request.body(),"rmx_no") ; 
            
            error[0] = verifyInteger(mode,1,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!" :(i==1) ? "Require Integer" : "Require Integer between 0 - "+std::to_string(INPUT_COUNT)+"!" ;
            }
            if(all_para_valid){
                Json::Value iojson=callSetInputOutput("0",output,std::stoi(rmx_no));
                if(iojson["error"]==false){
                    json=callSetNITmode(mode,output,std::stoi(rmx_no));
                    if(json["error"] == false)
                    	db->addNitMode(mode,output,std::stoi(rmx_no)); 
                }else{
                    json["error"]= true;
                    json["message"]= "Error while selecting output!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetNITmode(std::string mode,std::string output,int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,jsonMsg;
        jsonMsg["Mode"] = mode;
        json["error"]= false;
        uLen = c1.callCommand(10,RxBuffer,6,6,jsonMsg,1);
                     
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != CMD_INIT_NIT_MODE || uLen != 6 || RxBuffer[5] != ETX){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                // json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setNitMode","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "set Nit Mode!!";    
                     
                addToLog("setNitMode","Success");
            }
        }
        return json;
    }

    /*****************************************************************************/
    /*  Commande 0x10   function setNITmodeToAllRmx                           */
    /*****************************************************************************/
    void  setNITmodeToAllRmx(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        

        std::string para[] = {"mode"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setNITmodeToAllRmx",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string mode = getParameter(request.body(),"mode"); 
            if(verifyInteger(mode,1,1)){
                for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
                {
                    for (int output = 0; output < OUTPUT_COUNT; ++output)
                    {
                        Json::Value iojson=callSetInputOutput("0",std::to_string(output),rmx_no);
                        if(iojson["error"]==false){
                            json=callSetNITmode(mode,std::to_string(output),rmx_no);
                            if(json["error"] == false)
                                db->addNitMode(mode,std::to_string(output),rmx_no); 
                        }else{
                            json["error"]= true;
                            json["message"]= "Error while selecting output!";     
                        }
                    }
                }
                
            }else{
                json["error"]= true;
                json["mode"]="Require Integer between 0-1!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x13   function setTableTimeout                         */
    /*****************************************************************************/
    void  setTableTimeout(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        
        std::string para[] = {"table","timeout","rmx_no"};
        addToLog("setTableTimeout",request.body());
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string table = getParameter(request.body(),"table"); 
            std::string timeout = getParameter(request.body(),"timeout"); 
            error[0] = verifyInteger(table,1,1,4,0); 
            error[1] = verifyInteger(timeout,0,0,65535,500);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "table: Require Integer between 0-4!" :((i==1)?"timeout: Require Integer between 500-65535!": "Require Integer between 1-6!");
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    json=callSetTableTimeout(table,timeout,std::stoi(rmx_no)); 
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }   
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value  callSetTableTimeout(std::string table,std::string timeout,int rmx_no){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json,jsonMsg;
        jsonMsg["timeout"] = timeout;
        jsonMsg["table"] = table;
        uLen = c1.callCommand(13,RxBuffer,20,10,jsonMsg,1);
                     
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x13 || RxBuffer[4]!=0 || uLen != 6 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setTableTimeout","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "Set table timeout!";      
                db->addTableTimeout(table,timeout,rmx_no); 
                addToLog("setTableTimeout","Success");
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x14   function setTsId                         */
    /*****************************************************************************/
    void  setTsId(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;  
        std::string transportid,networkid,originalnwid;      
        
        std::string para[] = {"transportid","networkid","originalnwid","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setTsId",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]) );
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string transportid = getParameter(request.body(),"transportid"); 
            std::string networkid = getParameter(request.body(),"networkid"); 
            std::string originalnwid = getParameter(request.body(),"originalnwid"); 
            std::string output = getParameter(request.body(),"output"); 

            error[0] = verifyInteger(transportid,0 ,0,65535,1);
            error[1] = verifyInteger(networkid,0 ,0,65535,1);
            error[2] = verifyInteger(originalnwid,0 ,0,65535,1);
            error[3] = verifyInteger(output,1,1,INPUT_COUNT);
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 4)? "Require Integer between 1 to 6!" : (i==3)? "Require Integer between 0 to "+std::to_string(INPUT_COUNT)+"!" : "Require Integer between 1 to 65535!";
            }
            if(all_para_valid){
                json = callSetTsId(transportid,networkid,originalnwid,output,std::stoi(rmx_no));
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetTsId(std::string transportid,std::string networkid,std::string originalnwid, std::string output,int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,iojson,jsonMsg;
        iojson = callSetInputOutput("0",output,rmx_no); 
        if(iojson["error"]==false){
            jsonMsg["transportid"] = transportid;
            jsonMsg["networkid"] = networkid;
            jsonMsg["originalnwid"] = originalnwid;
            uLen = c1.callCommand(14,RxBuffer,6,11,jsonMsg,1);
        
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_INIT_TS_ID || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setTsId","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set TS details";
                    db->addNetworkDetails(output,transportid,networkid,originalnwid,rmx_no);
                    addToLog("setTsId","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }


    /*****************************************************************************/
    /*  Commande 0x14   function setTransportStreamId    {,"networkid","originalnwid" from DB}                     */
    /*****************************************************************************/
    void  setTransportStreamId(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;  
        std::string transportid,networkid,originalnwid;      
        
        std::string para[] = {"transportid","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setTransportStreamId",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]) );
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string transportid = getParameter(request.body(),"transportid");  
            std::string output = getParameter(request.body(),"output"); 

            error[0] = verifyInteger(transportid,0 ,0,65535,1);
            error[1] = verifyInteger(output,1,1,INPUT_COUNT);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 2)? "Require Integer between 1 to 6!" : (i==1)? "Require Integer between 0 to "+std::to_string(INPUT_COUNT)+"!" : "Require Integer between 1 to 65535!";
            }
            if(all_para_valid){
                Json::Value json_netw_id =  db->getNetworkId(rmx_no,output);
                if(json_netw_id["error"] == false){
                    networkid = json_netw_id["network_id"].asString();
                    originalnwid = json_netw_id["original_netw_id"].asString();
                    json = callSetTsId(transportid,networkid,originalnwid,output,std::stoi(rmx_no));
                }else{
                    json["error"]= true;
                    json["message"]= "Database is not in synch!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

     /*****************************************************************************/
    /*  Commande 0x14   function setTsId                         */
    /*****************************************************************************/
    void  setONId(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;  
        std::string transportid,networkid,originalnwid;      
        
        std::string para[] = {"networkid","originalnwid"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setTsId",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]) );
        if(res=="0"){
            std::string networkid = getParameter(request.body(),"networkid"); 
            std::string originalnwid = getParameter(request.body(),"originalnwid"); 

            error[0] = verifyInteger(networkid,0 ,0,65535,1);
            error[1] = verifyInteger(originalnwid,0 ,0,65535,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]="Require Integer between 1 to 65535!";
            }
            if(all_para_valid){
                Json::Value tsJason , network_details;
                network_details = db->getNetworkDetails();
                 if(network_details["error"]==false)
                 {
                    for (int i = 0; i < network_details["list"].size(); ++i)
                    {
                        if(network_details["list"][i]["ts_id"].asString() != "-1"){
                            tsJason = callSetTsId(network_details["list"][i]["ts_id"].asString(),networkid,originalnwid,network_details["list"][i]["output"].asString(),std::stoi(network_details["list"][i]["rmx_no"].asString())); 
                        }
                    }
                    json["error"]= false;
                    json["message"]= "Assaigned network id"; 
                }

            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x16   function setDvbSpiOutputMode                         */
    /*****************************************************************************/
    void  setDvbSpiOutputMode(const Rest::Request& request, Net::Http::ResponseWriter response){ 
        Json::Value json;
        Json::FastWriter fastWriter;        
        
        std::string para[] = {"rate","falling","mode","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setDvbSpiOutputMode",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string rate = getParameter(request.body(),"rate"); 
            
            std::string falling = getParameter(request.body(),"falling"); 
            
            std::string mode = getParameter(request.body(),"mode"); 
            
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rate);
            error[1] = verifyInteger(falling,1,1,1);
            error[2] = verifyInteger(mode,1,1,1);
            error[3] = verifyInteger(output,1,1,OUTPUT_COUNT);  
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);

            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer!!" :((i==3)? "Require Integer between 0 to "+std::to_string(INPUT_COUNT)+"!" : ((i==4)?"Require Integer between 1 to 6!" : "Require Integer 0 or 1!"));
            }
            if(all_para_valid){
                
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    json = callSetDvbSpiOutputMode(rate,falling,mode,output,std::stoi(rmx_no));
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetDvbSpiOutputMode(std::string rate,std::string falling,std::string mode,std::string output,int rmx_no)
    {
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,jsonMsg;
        jsonMsg["rate"] = rate;
        jsonMsg["falling"] = falling;
        jsonMsg["mode"] = mode;
        uLen = c1.callCommand(16,RxBuffer,6,9,jsonMsg,1);
                             
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_INIT_OUTPUT_MODE || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setDvbSpiOutputMode","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "set input";  
                db->addDVBspiOutputMode(output,rate,falling,mode,rmx_no);
                addToLog("setDvbSpiOutputMode","Success");  
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x17   function setPsiSiInterval                         */
    /*****************************************************************************/
    void  setPsiSiInterval(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        
        std::string para[] = {"patint","sdtint","nitint","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setPsiSiIntervalsetPsiSiInterval",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string patint = getParameter(request.body(),"patint"); 
            std::string sdtint = getParameter(request.body(),"sdtint"); 
            std::string nitint = getParameter(request.body(),"nitint"); 
            std::string output = getParameter(request.body(),"output"); 
            
            error[0] = verifyInteger(patint);
            error[1] = verifyInteger(sdtint);
            error[2] = verifyInteger(nitint);
            error[3] = verifyInteger(output,1,1,3,0);
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==3)? "Require Integer between 0- "+std::to_string(INPUT_COUNT)+" !" : (i==4)? "Require Integer between 1 to 6!" : "Require Integer!";
            }
            if(all_para_valid){
                json = callSetPsiSiInterval(patint,sdtint,nitint,output,std::stoi(rmx_no));
                if(json["error"] == false)
                	db->addPsiSiInterval(patint,sdtint,nitint,output,std::stoi(rmx_no));
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetPsiSiInterval(std::string patint,std::string sdtint,std::string nitint,std::string output,int rmx_no){
        unsigned char RxBuffer[6]={0};
        int uLen;
        Json::Value json,jsonMsg,iojson;
        iojson=callSetInputOutput("0",output,rmx_no);
        if(iojson["error"]==false){
            jsonMsg["patint"] = patint;
            jsonMsg["sdtint"] = sdtint;
            jsonMsg["nitint"] = nitint;
            uLen = c1.callCommand(17,RxBuffer,6,11,jsonMsg,1);
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_SET_SCHEDULER_TIME || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    // addToLog("setPsiSiInterval","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set PSI/SI interval!";  
                    // addToLog("setPsiSiInterval","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }

    /*****************************************************************************/
    /*  Commande 0x17   function setPsiSiIntervalToAllRmx                         */
    /*****************************************************************************/
    void  setPsiSiIntervalToAllRmx(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        
        std::string para[] = {"patint","sdtint","nitint"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,error_on_update_all=false;
        addToLog("setPsiSiIntervalToAllRmx",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string patint = getParameter(request.body(),"patint"); 
            std::string sdtint = getParameter(request.body(),"sdtint"); 
            std::string nitint = getParameter(request.body(),"nitint"); 
            
            error[0] = verifyInteger(patint);
            error[1] = verifyInteger(sdtint);
            error[2] = verifyInteger(nitint);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Require Integer!";
            }
            if(all_para_valid){
	            for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
	            {
	            	for (int output = 0; output < OUTPUT_COUNT; ++output)
	            	{
            			Json::Value json_psi_int = callSetPsiSiInterval(patint,sdtint,nitint,std::to_string(output),rmx_no);
            			if(json_psi_int["error"] == true)
            			{
            				error_on_update_all = true; 
            			}	
	            	}
	            }
	            if(error_on_update_all){
					json["error"] = true;
					json["message"] = "Partially updated!";
	            }else{
	            	db->addPsiSiInterval(patint,sdtint,nitint,"0",-1);
	            	json["error"] = false;
					json["message"] = "Interval updated successfully!";
	            }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x24   function setListofServiceID                           */
    /*****************************************************************************/
    void  setListofServiceID(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,paraJson;
        Json::Value root,root2;  
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"oldpronum","newprognum"};
        addToLog("setListofServiceID",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string oldpronum = getParameter(request.body(),"oldpronum"); 
            std::string oldPNumber = UriDecode(oldpronum);
            bool parsedSuccess = reader.parse(oldPNumber,root,false);
            std::string newprognum = getParameter(request.body(),"newprognum"); 
            newprognum = UriDecode(newprognum);
            bool parsedSuccess1 = reader.parse(newprognum,root2,false);
            if(parsedSuccess && parsedSuccess1)
            {
                bool oldp=verifyJsonArray(root,"old_list",1);
                bool newp=verifyJsonArray(root2,"new_list",1);
                 (oldp==true && newp==true)?"0":((oldp==false && newp==false)?(json["oldpronum"]=Invalid_json,json["newprognum"]=Invalid_json):((oldp==true && newp==false)?(json["newprognum"]=Invalid_json):json["oldpronum"]=Invalid_json));
                if(oldp && newp){
                    if((root["old_list"].size())==(root2["new_list"].size())){
                        paraJson["oldpronum"] = root["old_list"];
                        paraJson["newprognum"] = root2["new_list"];
                        uLen = c1.callCommand(24,RxBuffer,20,200,paraJson,1);      
                        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CHANGE_SID || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
                            json["error"]= true;
                            json["message"]= "STATUS COMMAND ERROR!";
                        }            
                        else{
                            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                            if (uLen != 1 ) {
                                json["error"]= true;
                                json["message"]= "STATUS COMMAND ERROR!";
                                addToLog("setListofServiceID","Error");
                            }else{
                                json["status"] = RxBuffer[4];
                                json["error"]= false;
                                json["message"]= "set input";          
                                addToLog("setListofServiceID","Success");
                            }
                        }
                    }else{
                        json["error"]= true;
                        json["message"]="Old program list and new list must same length!" ;    
                    }
                }else{
                    json["error"]= true;
                }
            }
            else{
                json["error"]= true;
                json["message"]= "Failed to parse JSON!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void  setServiceID(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;
        std::string para[] = {"original_service_id","new_service_id","input","rmx_no","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setServiceID",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string oldpronum = getParameter(request.body(),"original_service_id"); 
            std::string newprognum = getParameter(request.body(),"new_service_id"); 
            std::string input = getParameter(request.body(),"input");
            std::string addFlag = getParameter(request.body(),"addFlag");
            error[0] = verifyInteger(oldpronum);
            error[1] = verifyInteger(newprognum);
            error[2] = verifyInteger(input,1,1,INPUT_COUNT);
            error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[4] = verifyInteger(addFlag,1,1,1,0);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]=(i==3)? "Require Integer between 1 to 6 !" : ((i==2)?"Require Integer between 1 to "+std::to_string(INPUT_COUNT)+"!" :"Require Integer between 1 to 65535 !");
            }
            if(all_para_valid){
                Json::Value service_ids,new_service_ids,old_service_ids;
                int rmx_no_i = std::stoi(rmx_no);
                int flag = std::stoi(addFlag);
                Json::Value iojson = callSetInputOutput(input,"0",rmx_no_i);
                if(iojson["error"] == false){
                    Json::Value service_ids = db->getServiceIds(rmx_no_i,oldpronum,input);
                    int is_present = 0;
                    if(service_ids["error"] == false){
                        for (int i = 0; i < service_ids["list"].size(); ++i)
                        {
                            std::string old_service_id = service_ids["list"][i]["channel_number"].asString();
                            if(std::stoi(oldpronum) != std::stoi(old_service_id)){
                                old_service_ids[i] = old_service_id;
                                new_service_ids[i] = service_ids["list"][i]["service_id"].asString();   
                            }
                        }
                        if(flag){
                            old_service_ids[service_ids["list"].size()] = oldpronum;
                            new_service_ids[service_ids["list"].size()] = newprognum;   
                        }
                    }else{
                        if(flag){
                            old_service_ids[0] = oldpronum;
                            new_service_ids[0] = newprognum;    
                        }
                    }
                    json["old_service_ids"] = old_service_ids;
                    json["new_service_ids"] = new_service_ids;
                    json = callSetServiceID(old_service_ids,new_service_ids,rmx_no_i);
                    if(json["error"] == false)
                        db->addServiceId(oldpronum,newprognum,rmx_no,input,flag);
                }
                else
                    json = iojson;
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetServiceID(Json::Value oldpronum,Json::Value newprognum,int rmx_no){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json,paraJson;
        paraJson["oldpronum"] = oldpronum;
        paraJson["newprognum"] = newprognum;
        uLen = c1.callCommand(24,RxBuffer,20,200,paraJson,1);      
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CHANGE_SID || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setServiceID","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "set input";          
                addToLog("setServiceID","Success");
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x24   function flushServiceIDs                           */
    /*****************************************************************************/
    void  flushServiceIDs(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json,paraJson;
        Json::Value root,root2;  
        Json::Reader reader;
        Json::FastWriter fastWriter;
        addToLog("flushServiceIDs",request.body());
        std::string rmx_no = getParameter(request.body(),"rmx_no") ; 
        if(verifyInteger(rmx_no,1,1,RMX_COUNT)==1)
        {
            int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                uLen = c1.callCommand(241,RxBuffer,20,200,paraJson,1);      
                if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CHANGE_SID || RxBuffer[4]!=1 || RxBuffer[5] != ETX ){
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                }            
                else{
                    uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                    if (uLen != 1 ) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("flushServiceIDs","Error");
                    }else{
                        json["status"] = RxBuffer[4];
                        json["error"]= false;
                        json["message"]= "All service IDs reinitiated!!!";
                        addToLog("flushServiceIDs","Success");  
                        db->flushServiceId();        
                    }
                }
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = true;
            json["message"] = "Required integer between 1 to 6!";
        }         
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x05   function setCore                         */
    /*****************************************************************************/
    void  setCore(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"cs","address","data","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCore",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string cs = getParameter(request.body(),"cs"); 
            std::string address = getParameter(request.body(),"address"); 
            std::string data = getParameter(request.body(),"data"); 
            
            error[0] = verifyInteger(cs,0,0,128,1);
            error[1] = verifyInteger(address);
            error[2] = verifyInteger(data);
            error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : ((i==1)? "Require Integer between 1 to 128!" : "Require Integer!");
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    json = callSetCore(cs,address,data,std::stoi(rmx_no));
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetCore(std::string cs,std::string address,std::string data,int rmx_no){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json,jsonMsg;
        jsonMsg["cs"] = cs;
        jsonMsg["address"] = address;
        jsonMsg["data"] = data;
        json["error"]= false;
        uLen = c1.callCommand(05,RxBuffer,20,20,jsonMsg,1);
                             
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CORE || uLen != 6 || RxBuffer[4]!=1 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setCore","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "set CORE";  
                db->addRmxRegisterData(cs,address,data,rmx_no);    
                addToLog("setCore","Success");    
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x36   function setPmtAlarm                         */
    /*****************************************************************************/
    int setPmtAlarm(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::Reader reader;
        Json::FastWriter fastWriter;

        std::string para[] = {"programNumber","alarm","input","rmx_no"};
         int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setPmtAlarm",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){  
            std::string programNumber = getParameter(request.body(),"programNumber"); 
            std::string alarm = getParameter(request.body(),"alarm"); 
            std::string input = getParameter(request.body(),"input"); 
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            error[0] = verifyInteger(programNumber,6);
            error[1] = verifyInteger(alarm,1,1);
            error[2] = verifyInteger(input,1,1,INPUT_COUNT);
            error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 2)? "Require Integer between 0- "+std::to_string(INPUT_COUNT)+"!" : ((i==3)? "Require Integer between 1-6!" : "Require Integer!");
            }
            if(all_para_valid){
                json = callSetPmtAlarm(programNumber,alarm,input,std::stoi(rmx_no));
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetPmtAlarm(std::string programNumber,std::string alarm,std::string input,int rmx_no){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json,paraJson;
        Json::Value root,root2; 
        Json::Value iojson = callSetInputOutput(input,"0",rmx_no); 
        if(iojson["error"]==false){
            Json::Value existing_pmt_alarm = db->getPmtalarm(input,programNumber);
            std::cout<<input<<"----------->>"<<programNumber;
            if(existing_pmt_alarm["error"]==false){
                for (int i = 0; i < existing_pmt_alarm["list"].size(); ++i)
                {
                    root.append(existing_pmt_alarm["list"][i]["program_number"].asString());
                    root2.append(existing_pmt_alarm["list"][i]["alarm_enable"].asString()); 
                    std::cout<<" "<<existing_pmt_alarm["list"][i]["program_number"].asString()<<" ";
                }
            }
            std::cout<<std::endl;
            root.append(programNumber);
            root2.append(alarm); 
            paraJson["alarm"] = root2;
            paraJson["programs"] = root;
            uLen = c1.callCommand(36,RxBuffer,20,2000,paraJson,1);       
            if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != CMD_PROG_PMT_ALARM || RxBuffer[4]!=1 || uLen != 6 || RxBuffer[5] != ETX){
                json["error"]= true;
                if(RxBuffer[4]==0)
                    json["message"]= "Error:May already alarm is set by some other input!";
                else
                    json["message"]= "STATUS COMMAND ERROR1!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR2!";
                    addToLog("setPmtAlarm","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Setting PMT alarm!";  
                    db->addPmtAlarm(programNumber,alarm,input,rmx_no); 
                    addToLog("setPmtAlarm","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }

    /*****************************************************************************/
    /*  Commande 0x36   function getPmtAlarm                          */
    /*****************************************************************************/
    void getPmtAlarm(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json = callGetPmtAlarm(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
            
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }   
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetPmtAlarm(int rmx_no){
        unsigned char RxBuffer[200]={0};
        
        Json::Value json,ProgNumber,Affected_input;
        unsigned char *name;
        int length=0;
        int uLen = c1.callCommand(36,RxBuffer,200,20,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != CMD_PROG_PMT_ALARM) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getPmtAlarm","Error"); 
        }else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            length = uLen/4;

            for(int i=0; i<length; i++) {
                ProgNumber[i] = (RxBuffer[4*i+4]<<8)|RxBuffer[4*i+5];
                Affected_input[i] = RxBuffer[4*i+6];
            }
            json["error"] = false;
            json["ProgNumber"]=ProgNumber;
            json["Affected_input"]=Affected_input;
            json["message"] = "GET Pmt Alarm nos!";
            addToLog("getPmtAlarm","Success"); 
        }
        return json;
    }
    void getPMTAlarm(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputModes",request.body());
        std::string input, output,str_rmx_no;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i!=0)? "Require Integer between 0-"+std::to_string(INPUT_COUNT)+" !" : "Require Integer between 1-6!";
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,"0",std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetPmtAlarm(std::stoi(str_rmx_no));
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        }  
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x18   function setNetworkName                         */
    /*****************************************************************************/
    void  setNetworkName(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        bool all_para_valid=true;
        std::string para[] = {"NewName","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        addToLog("setNetworkName",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string NewName = getParameter(request.body(),"NewName"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyString(NewName);
            error[1] = verifyInteger(output,1,1,INPUT_COUNT);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]] = (i==0)? "Require string!" :((i==1) ?"Require integer between 0-"+std::to_string(INPUT_COUNT)+" !" :"Require integer between 1 to 6"); 
            }
            if(all_para_valid){
                json = callSetNetworkName(NewName,output,std::stoi(rmx_no));
             }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }          
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetNetworkName(std::string NewName,std::string output,int rmx_no){
        unsigned char RxBuffer[10]={0};
        int uLen;
        Json::Value json,iojson;
        unsigned int len = 8+NewName.length();
        iojson = callSetInputOutput("0",output,rmx_no);
        if(iojson["error"]==false){
            json["NewName"] = NewName;
            uLen = c1.callCommand(18,RxBuffer,10,len,json,1);
            if (!uLen || RxBuffer[0] != STX || std::stoi(getDecToHex((int)RxBuffer[3])) != 18 || uLen != 6 || RxBuffer[4] != 1 ||  RxBuffer[5] != ETX){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!1"+getDecToHex((int)RxBuffer[3]);
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setNetworkName","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set new Netwoek name!";   
                    db->addNetworkname(NewName,output,rmx_no);       
                    addToLog("setNetworkName","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x19   function setServiceName                         */
    /*****************************************************************************/
    void  setServiceName(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;        
        bool all_para_valid=true;
        std::string para[] = {"addFlag","pnumber","input","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        addToLog("setServiceName",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string addFlag_str = getParameter(request.body(),"addFlag");
            if(verifyInteger(addFlag_str,1,1)){
                std::string NewName;
                int addFlag = std::stoi(addFlag_str);
                if(addFlag == 1){
                    NewName = getParameter(request.body(),"pname"); 
                    error[0] =verifyString(NewName,30); 
                }else{
                    NewName = -1;
                    error[0] =1;    
                }   
                std::string rmx_no =getParameter(request.body(),"rmx_no");
                std::string input = getParameter(request.body(),"input"); 
                std::string progNumber = getParameter(request.body(),"pnumber"); 
               
                error[1] = verifyInteger(progNumber);
                error[2] = verifyInteger(input,1,1,INPUT_COUNT);
                error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);

                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= (i==0)? "Require Integer between 1-6!" :((i==1)? "Require Integer between 0-"+std::to_string(INPUT_COUNT)+" !" : ((i==2)?  "Require Integer!" : "Require valid string (Max 30 charecter)!") );
                }
                if(all_para_valid){
                   json = callSetServiceName(NewName,progNumber,input,std::stoi(rmx_no),addFlag);
                   if(json["error"] == false) 
                        db->addChannelname(progNumber,NewName,rmx_no,input,addFlag);  
                }
            }else{
                json["error"]= true;
                json["addFlag"]= "Required Integer between 0-1!";   
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }          
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetServiceName(std::string NewName,std::string progNumber,std::string input_channel,int rmx_no,int addFlag = 1){
        unsigned char RxBuffer[10]={0};
        int uLen;
        Json::Value json,iojson,jsonMsg;
        unsigned int len = 8+NewName.length();
        iojson = callSetInputOutput(input_channel,"0",rmx_no);
        if(iojson["error"]==false){
            jsonMsg["addFlag"] = addFlag;
            jsonMsg["NewName"] = NewName;
            jsonMsg["progNumber"] = progNumber;
            uLen = c1.callCommand(19,RxBuffer,10,len,jsonMsg,1);
            if (!uLen || RxBuffer[0] != STX || std::stoi(getDecToHex((int)RxBuffer[3])) != 19 || uLen != 6 || RxBuffer[4] != 1 ||  RxBuffer[5] != ETX){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!1"+getDecToHex((int)RxBuffer[4]);
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setServiceName","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "Set new name!";                                
                    addToLog("setServiceName","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x1A  function setServiceProvider                         */
    /*****************************************************************************/
    void  setServiceProvider(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        bool all_para_valid=true;
        std::string para[] = {"providerName","serviceNumber","rmx_no","input","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        addToLog("setServiceProvider",request.body());
        std::string res=validateRequiredParameter(request.body(),para,  sizeof(para) / sizeof(para[0]));
        if(res=="0"){ 
            std::string rmx_no = getParameter(request.body(),"rmx_no") ; 
            if(verifyInteger(rmx_no,1,1,RMX_COUNT)==1){  
                std::string providerName = getParameter(request.body(),"providerName"); 
                std::string serviceNumber = getParameter(request.body(),"serviceNumber"); 
                std::string input = getParameter(request.body(),"input"); 
                std::string addFlag = getParameter(request.body(),"addFlag"); 
                error[0] = verifyString(providerName,30);
                error[1] = verifyInteger(serviceNumber,0,0,65535,1);
                error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                error[3] = verifyInteger(input,1,1,INPUT_COUNT);
                error[4] = verifyInteger(addFlag,1,1,1);
                
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= (i==0)? "Require string (length 1-30)!" :((i==3)? "Require Integer between 0-"+std::to_string(INPUT_COUNT)+" !" : ((i==2)?  "Require Integer between 1-"+std::to_string(RMX_COUNT)+" !" : ((i == 1) ? "Require Integer between 1 - 65535!" : "Require Integer 0 or 1!") ));
                }
                if(all_para_valid){
                    json = callSetServiceProvider(providerName,serviceNumber,input,std::stoi(rmx_no),std::stoi(addFlag));
                    if(json["error"] == false)
                        db->addNewProviderName(serviceNumber,providerName,rmx_no,addFlag);        
                }
            }else{
                json["error"] = false;
                json["message"] = "Invalid remux id";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetServiceProvider(std::string providerName,std::string serviceNumber,std::string input_channel,int rmx_no,int addFlag = 1){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json,jsonMsg,iojson;
        
        iojson = callSetInputOutput(input_channel,"0",rmx_no);
        // std::cout<<"------------------TEST--------------------- "<<progNumber<<std::endl;
        if(iojson["error"]==false){
            jsonMsg["providerName"] = providerName;
            jsonMsg["serviceNumber"] = serviceNumber;
            jsonMsg["addFlag"] = addFlag;
            uLen = c1.callCommand(26,RxBuffer,10,1024,jsonMsg,1);
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CHANGE_PROV || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }            
            else{
                addToLog("callSetServiceProvider",serviceNumber);
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("callSetServiceProvider","Error");
                }else{
                    json["status"] = RxBuffer[4];
                    json["error"]= false;
                    json["message"]= "set provider name";  
                    addToLog("callSetServiceProvider","Success");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= iojson["message"];
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x1A   function getServiceProviderName                         */
    /*****************************************************************************/
    void getServiceProviderName(const Rest::Request& request, Net::Http::ResponseWriter response){
         Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                auto serviceNumber = request.param(":uProg").as<std::string>();
                json = callGetServiceProvider(rmx_no , serviceNumber);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetServiceProvider(int rmx_no,std::string serviceNumber){
        unsigned char RxBuffer[100]={0};
        Json::Value json;
        Json::Value jsonInput;
        unsigned char *name;
        jsonInput["uProg"] = serviceNumber;
        addToLog("getNewProvName",serviceNumber);

        int uLen = c1.callCommand(26,RxBuffer,100,8,jsonInput,0);
        
        if (!uLen || RxBuffer[0]!= STX || RxBuffer[3] != 0x1A) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getNewProvName","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | RxBuffer[2]);
            name = (unsigned char *)malloc(uLen + 1);
            json["error"] = false;
            json["message"] = "GET Service provider name!";
            std::string nName="";
            if(uLen>0){
                for (int i = 0; i<uLen; i++) {
                    name[i] = RxBuffer[4 + i];
                    nName=nName+getDecToHex((int)RxBuffer[4 + i]);
                }
                json["pName"] = hex_to_string(nName);
            }else{
                json["pName"] = "-1";
            }
            
            addToLog("getNewProvName","Success");
        }
        return json;
    }
    void getServiceProvider(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,iojson;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","input","serviceNumber"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getServiceProvider",request.body());
        std::string input, output,str_rmx_no,serviceNumber;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_rmx_no = getParameter(request.body(),"rmx_no");
            input = getParameter(request.body(),"input");
            // output = getParameter(request.body(),"output");
            serviceNumber = getParameter(request.body(),"serviceNumber");
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            // error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(serviceNumber,0,0,65535,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1-6!": ((i==3)?"Require Integer" : "Require Integer between 0-"+std::to_string(INPUT_COUNT)+"!");
            }
            if(all_para_valid){
                iojson=callSetInputOutput(input,"0",std::stoi(str_rmx_no));
                if(iojson["error"]==false){
                    json = callGetServiceProvider(std::stoi(str_rmx_no),serviceNumber);
                }else{
                    json = iojson;
                }
            }      
        }else{
            json["error"]= true;
            json["message"]= res;
        } 
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     /*****************************************************************************/
    /*  Commande 0x47   function setCsa                       
        
      */
    /*****************************************************************************/
    void  setCsa(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,programNumbers,keyIDS;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"input","output","auth_bit","parity","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCsa",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){

                std::string rmx_no =getParameter(request.body(),"rmx_no") ;
                std::string input = getParameter(request.body(),"input"); 
                std::string output = getParameter(request.body(),"output"); 
                std::string auth_bit = getParameter(request.body(),"auth_bit"); 
                std::string parity = getParameter(request.body(),"parity"); 
                error[0] = verifyInteger(parity,1,1);
                error[1] = verifyInteger(input,1,1);
                error[2] = verifyInteger(output,1,1);
                error[3] = verifyInteger(auth_bit,0,0,255,1);
                error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= "Require Integer!";
                }
                if(all_para_valid){
                    json = callSetcsa(input,output,std::stoi(rmx_no),auth_bit,parity);
                }else{
                    json["error"]= true;
                }
  
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetcsa(std::string input,std::string output,int rmx_no, std::string auth_bit,std::string parity){
        unsigned char RxBuffer[261]={0};
        int uLen;
        Json::Value json,jsonMsg,root,root1;
        std::string prog_nos_str,key_str;
        // Json::Value iojson = callSetInputOutput(input,output,rmx_no); 

        jsonMsg["auth_bit"] = auth_bit; 
        jsonMsg["parity"] = parity; 
        // uLen = c1.callCommand(47,RxBuffer,6,4090,jsonMsg,1);
        uLen = c1.callCommand(47,RxBuffer,6,4090,jsonMsg,0);
        if (RxBuffer[0] != STX || RxBuffer[3] != 0x47 || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR 1!";
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR 2!";
            }else{
                json["error"]= false;
                addToLog("addEncryptedPrograms","Success");
                                json["message"]= "Set encrypted programs!";    
            }
        }
        
        return json;
    }

    // /*****************************************************************************/
    // /*  Commande 0x40   function setKeepProg                       
    //     includeFlag 1-is for add, 0 - is for delete
    //   */
    // /*****************************************************************************/
    // void  setCustomPids(const Rest::Request& request, Net::Http::ResponseWriter response){
    //     Json::Value json,Pids,AuthOutputs;
    //     Json::FastWriter fastWriter;
    //     Json::Reader reader;
    //     std::string para[] = {"pids","auth_outputs","rmx_no","output"};
    //     int error[ sizeof(para) / sizeof(para[0])];
    //     bool all_para_valid=true;
    //     addToLog("setCustomPids",request.body());
    //     std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
    //     if(res=="0"){
            
    //         std::string service_list = getParameter(request.body(),"pids"); 
    //         std::string serviceList = UriDecode(service_list);
    //         std::string keyids = getParameter(request.body(),"auth_outputs"); 
    //         std::string keyIds = UriDecode(keyids);
    //         bool parsedSuccess = reader.parse(serviceList,Pids,false);
    //         bool parsedSuccess1 = reader.parse(keyIds,AuthOutputs,false);
    //         if (parsedSuccess && parsedSuccess1)
    //         {
    //             std::string rmx_no =getParameter(request.body(),"rmx_no") ;
    //             std::string output =getParameter(request.body(),"output") ;
    //             error[0] = verifyJsonArray(Pids,"pids",1);
    //             error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
    //             error[2] = verifyJsonArray(AuthOutputs,"auth_outputs",1);
    //             error[3] = verifyInteger(output,1,1,OUTPUT_COUNT,0);
    //             for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
    //             {
    //                if(error[i]!=0){
    //                     continue;
    //                 }
    //                 all_para_valid=false;
    //                 json["error"]= true;
    //                 json[para[i]]= "Require Integer!";
    //             }
    //             if(all_para_valid){
    //                 json = callsetCustomPids(Pids["pids"],AuthOutputs["auth_outputs"],std::stoi(rmx_no),output);
    //             }else{
    //                 json["error"]= true;
    //             }
    //         }else{
    //             json["error"]= true;
    //             json["message"]= "Invalide json input!";
    //         }
    //     }else{
    //         json["error"]= true;
    //         json["message"]= res;
    //     }
    //     std::string resp = fastWriter.write(json);
    //     response.send(Http::Code::Ok, resp);
    // }
    /*****************************************************************************/
    /*  Commande 0x40   function setKeepProg                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setCustomPids(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,Pids,AuthOutputs;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"pid","auth_output","rmx_no","output","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCustomPids",request.body());

        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){            
            std::string pid = getParameter(request.body(),"pid"); 
            std::string auth_output = getParameter(request.body(),"auth_output");            
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output =getParameter(request.body(),"output") ;
            std::string addFlag = getParameter(request.body(),"addFlag");
            error[0] = verifyInteger(pid,0,0,65535,1);
            error[2] = verifyInteger(auth_output,0,0,255,1);
            error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);            
            error[3] = verifyInteger(output,1,1,OUTPUT_COUNT,0);
            error[4] = verifyInteger(addFlag,1,1,1,0);
            // Json::Value json_pids = db->getCustomPids(rmx_no);
            // std::cout<<json_pids<<std::endl;
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Require Integer!";
            }
            db->addCustomPid(rmx_no,output,pid,std::stoi(addFlag));
            Json::Value json_pids = db->getCustomPids(rmx_no);
            std::cout<<json_pids<<std::endl;
            if(json_pids["error"] == false){
                json = callsetCustomPids(json_pids["pids"],json_pids["output_auths"],std::stoi(rmx_no),output);
            }else{
                json["error"]= true;
                json["message"]= "No record found!";
            }
           
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callsetCustomPids(Json::Value Pids,Json::Value AuthOutputs,int rmx_no,std::string output){
        unsigned char RxBuffer[261]={0};
        int uLen;
        Json::Value json,jsonMsg,root,root1;
        std::string prog_nos_str,key_str;
        Json::Value iojson = callSetInputOutput("0",output,rmx_no); 
        for(int i = 0; i<Pids.size();i++){
            prog_nos_str= prog_nos_str+','+Pids[i].asString();
            root.append(Pids[i].asString());
        }
        // std::cout<<prog_nos_str;
        prog_nos_str = prog_nos_str.substr(1);

        for(int i = 0; i<AuthOutputs.size();i++){
            key_str= key_str+','+AuthOutputs[i].asString();
            root1.append(AuthOutputs[i].asString());
        }
        
        key_str = key_str.substr(1);
        if(iojson["error"]==false){
            
            jsonMsg["Pids"] = root; 
            jsonMsg["AuthOutputs"] = root1; 
            uLen = c1.callCommand(48,RxBuffer,6,4090,jsonMsg,1);
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_SET_PID || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR 1!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR 2!";
                }else{
                    json["error"]= false;
                    addToLog("addEncryptedPrograms","Success");
                    json["message"]= "Set Custom  pids!";    
                }
            }
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x49   function setCATCADescriptor                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setCATCADescriptor(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,Pids,private_data_list,JSON_CA_System_id,JSON_emm_pids;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"channel_id","rmx_no","output","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCATCADescriptor",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
            std::string privateData = getParameter(request.body(),"privateData"); 
            std::string privateData_list = UriDecode(privateData);
            bool parsedSuccess = true;
            if(privateData_list != ""){
            	json["privateData"] = "PRESENT";
            	bool parsedSuccess = reader.parse(privateData_list,private_data_list,false);
            }
            if (parsedSuccess)
            {
            	std::string channel_id =getParameter(request.body(),"channel_id") ;
            	// std::string emm_pid =getParameter(request.body(),"emm_pid") ;
                std::string rmx_no =getParameter(request.body(),"rmx_no") ;
                std::string output =getParameter(request.body(),"output") ;
                std::string addFlag =getParameter(request.body(),"addFlag") ;
                error[0] = verifyInteger(channel_id);
                // error[1] = verifyInteger(emm_pid);
                error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                error[2] = verifyInteger(output,1,1,OUTPUT_COUNT,0);
                error[3] = verifyInteger(addFlag,1,1,1,0);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= "Require Integer!";
                }
                if(all_para_valid){
                	Json::Value auth_outputs,emm_pids;
                	// JSON_CA_System_id.append(std::stoi(CA_System_id));JSON_emm_pids.append(std::stoi(emm_pid));
                	db->enableEMM(channel_id,rmx_no,output,std::stoi(addFlag));
                	Json::Value JSON_emmgs = db->getEMMGChannels(rmx_no,output,"-1");
                    std::cout<<"-------------------------------------------------------------------------------";
                    if (JSON_emmgs["error"] == false)
                    {
                    	for (int i = 0; i < JSON_emmgs["ca_system_ids"].size(); ++i)
                    	{

                            std::string system_id = JSON_emmgs["ca_system_ids"][i].asString();

                            system_id = system_id.substr(2,system_id.length()-6);
                            std::cout<<system_id;
                    		JSON_CA_System_id.append(std::to_string(getHexToLongDec(system_id)));
                    		auth_outputs.append("255");
                    		emm_pids.append(JSON_emmgs["emm_pids"][i].asString());
                    		/* code */
                    	}
                    	json = callsetCATCADescriptor(JSON_CA_System_id,JSON_emmgs["emm_pids"],private_data_list,std::stoi(rmx_no),output);

                    	Json::Value customPid = callsetCustomPids(emm_pids,auth_outputs,std::stoi(rmx_no),output);
		               	if(customPid["error"] == false){
		               		json["customPid"] = "Successfully enabled EMM PID!";
		               	}
		               	else{
		               		json["customPid"] = "Failed enabled EMM PID!";
		               	}
		            }else{
		            	//Empty the CA descriptor
		            	json = callsetCATCADescriptor(JSON_CA_System_id,emm_pids,private_data_list,std::stoi(rmx_no),output);
		        		json["error"]= false;
                		json["message"]= "Successfully resetted!";    	
		            }

                }else{
                    json["error"]= true;
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalide json input!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callsetCATCADescriptor(Json::Value JSON_CA_System_id,Json::Value emm_pids, Json::Value private_data_lists,int rmx_no,std::string output){
    	Json::Value json,jsonMsg;
    	 unsigned char RxBuffer[15]={0};
    	 int uLen;
    	 // JSON_CA_System_id.append(CA_System_id);
    	 // JSON_CA_System_id.append(10190);
    	jsonMsg["CA_System_ids"] =JSON_CA_System_id;
    	// emm_pids.append(emm_pid);
    	// emm_pids.append(1456);
    	jsonMsg["emm_pids"] = emm_pids;
    	// private_data_list.append(1);
    	// private_data_list.append(2);
    	// private_data_lists.append(private_data_list);
    	jsonMsg["private_data_list"] = private_data_lists;
        // std::cout<<JSON_CA_System_id;
    	int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        if(write32bCPU(0,0,target) != -1) { 
         Json::Value iojson = callSetInputOutput("0",output,rmx_no);
         if(iojson["error"] == false){
	    	 	uLen = c1.callCommand(49,RxBuffer,15,4090,jsonMsg,1);
	            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x49 || uLen != 6 || RxBuffer[4] < 1 || RxBuffer[5] != ETX ){
	                json["error"]= true;
	                json["message"]= "STATUS COMMAND ERROR 1!";
	            }           
	            else{

	                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
	                if (uLen != 1 ) {
	                    json["error"]= true;
	                    json["message"]= "STATUS COMMAND ERROR 2!";
	                }else{
	                    json["resp"]=RxBuffer[4];
	                    json["error"]=false;
	                    json["message"]= "Set CA descriptor for CAT table!";    
	                }
	            }
	        }else{
	        	json["error"] = true;
        		json["message"] = iojson["message"];
	        }
        }else{
        	json["error"] = true;
        	json["message"] = "Connection error!";	
        }

    	return json;
    }

    void updateCATCADescriptor(std::string channel_id){
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
            for (int output = 0; output <OUTPUT_COUNT; ++output)
            {
                Json::Value JSON_emmgs,JSON_CA_System_id,emm_pids,private_data_list,auth_outputs,json;
                JSON_emmgs = db->getEMMGChannels(std::to_string(rmx_no),std::to_string(output),channel_id);
                if (JSON_emmgs["error"] == false)
                    {
                        for (int i = 0; i < JSON_emmgs["ca_system_ids"].size(); ++i)
                        {
                            std::string CA_System_id = JSON_emmgs["ca_system_ids"][i].asString();
                            std::string System_id = CA_System_id.substr(2,CA_System_id.length()-6);
                            JSON_CA_System_id.append(std::to_string(getHexToLongDec(System_id)));
                            auth_outputs.append("255");
                            emm_pids.append(JSON_emmgs["emm_pids"][i].asString());
                            // std::cout<<std::to_string(getHexToLongDec(System_id))<<std::endl;
                        }
                         std::cout<<JSON_CA_System_id<<std::endl;
                        json = callsetCATCADescriptor(JSON_CA_System_id,JSON_emmgs["emm_pids"],private_data_list,rmx_no,std::to_string(output));
                        Json::Value customPid = callsetCustomPids(emm_pids,auth_outputs,rmx_no,std::to_string(output));
                        if(customPid["error"] == false){
                            std::cout<< "----------------Successfully enabled EMM PID! -----------------------"<<std::endl;
                        }
                        else{
                            std::cout<<"----------------Failed enabled EMM PID! -----------------------"<<std::endl;
                        }
                    }
            }
        }
    }
    /*****************************************************************************/
    /*  Commande 0x49   function setPMTCADescriptor                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setPMTCADescriptor(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,Pids,private_data_list,JSON_CA_System_id,JSON_emm_pids;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"channel_id","stream_id","rmx_no","output","service_pid","programNumber","addFlag","input"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setPMTCADescriptor",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
            std::string privateData = getParameter(request.body(),"privateData"); 
            std::string privateData_list = UriDecode(privateData);
            bool parsedSuccess = true;
            if(privateData_list != ""){
            	json["privateData"] = "PRESENT";
            	bool parsedSuccess = reader.parse(privateData_list,private_data_list,false);
            }
            if (parsedSuccess)
            {
            	std::string channel_id =getParameter(request.body(),"channel_id") ;
            	std::string stream_id =getParameter(request.body(),"stream_id") ;
                std::string rmx_no =getParameter(request.body(),"rmx_no") ;
                std::string output =getParameter(request.body(),"output") ;
                std::string service_pid =getParameter(request.body(),"service_pid") ;
                std::string programNumber =getParameter(request.body(),"programNumber") ;
                std::string addFlag =getParameter(request.body(),"addFlag") ;
                std::string input =getParameter(request.body(),"input") ;
                error[0] = verifyInteger(channel_id,0,0,65535,1);
                error[1] = verifyInteger(stream_id,0,0,65535,1);
                error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                error[3] = verifyInteger(output,1,1,OUTPUT_COUNT,0);
                error[4] = verifyInteger(service_pid,0,0,65535,1);
                error[5] = verifyInteger(programNumber,0,0,65535,1);
                error[6] = verifyInteger(addFlag,1,1,1,0);
                error[7] = verifyInteger(output,1,1,INPUT_COUNT,0);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= "Require Integer!";
                }
                if(all_para_valid){
                	long int key_index = db->getCWKeyIndex(rmx_no,input,output,programNumber);
	        		if(key_index != -1){
	                	json["service_pid"] = service_pid;
	                	Json::Value auth_outputs,ecm_pids,elementryPid,service_pids;
		                if(db->isServiceEncrypted(programNumber,rmx_no,output,input)){
		                	db->enableECM(channel_id,stream_id,service_pid,programNumber,rmx_no,output,input,std::stoi(addFlag));
		                	Json::Value JSON_ecm_desc = db->getECMDescriptors(rmx_no,input);
		                	std::cout<<JSON_ecm_desc<<std::endl;
		                	// json["ecms"] = JSON_ecm_desc;
		                    std::cout<<"-------------------------------------------------------------------------------";
		                    if (JSON_ecm_desc["error"] == false)
		                    {
		                    	for (int i = 0; i < JSON_ecm_desc["ca_system_ids"].size(); ++i)
		                    	{

		                            std::string system_id = JSON_ecm_desc["ca_system_ids"][i].asString();

		                            system_id = system_id.substr(2,system_id.length()-6);
		                            // std::cout<<system_id;
		                    		JSON_CA_System_id.append(std::to_string(getHexToLongDec(system_id)));
		                    		auth_outputs.append("1");
		                    		service_pids.append(JSON_ecm_desc["service_pids"][i].asString());

		                    		ecm_pids.append(JSON_ecm_desc["ecm_pids"][i].asString());

		                    	}
		                    	json = callSetPMTCADescriptor(service_pids,JSON_CA_System_id,ecm_pids,private_data_list,std::stoi(rmx_no),output,input);
		                  //   	Json::Value customPid = callsetCustomPids(ecm_pids,auth_outputs,std::stoi(rmx_no),output);
				               	// if(customPid["error"] == false){
				               	// 	json["customPid"] = "Successfully enabled ECM PID!";
				               	// }
				               	// else{
				               	// 	json["customPid"] = "Failed enabled ECM PID!";
				               	// }
				            }else{
				            	//Empty the CA descriptor
				            	json = callSetPMTCADescriptor(service_pids,JSON_CA_System_id,ecm_pids,private_data_list,std::stoi(rmx_no),output,input);
				            }
				            if(json["error"] == false){
				            	int flag = std::stoi(addFlag);
		        				int rmx_id =std::stoi(rmx_no);
                                if(rmx_id%2 == 0)
                                    key_index = key_index+128;
				            	std::string port =(rmx_id == 1 || rmx_id == 2)? "5000" : ((rmx_id == 3 || rmx_id == 4)? "5001" : "5002");
				            	updateCWIndex(channel_id,stream_id,std::to_string(key_index),port,output,flag);
				            	// db->updateCWIndex(rmx_no,output,programNumber,index,indexValue,flag);
					        }
				        }else{
				        	json["error"]= true;
	                		json["message"]= "ECM stream does not exists!";
				        }
				    }else{
				    	json["error"]= true;
	                	json["message"]= "Encrypt service before adding to PMT descriptor!";
				    }
                }else{
                    json["error"]= true;
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalide json input!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void updateCWIndex(std::string channel_id,std::string stream_id,std::string cw_index,std::string port,std::string output,int addFlag){
    	reply = (redisReply *)redisCommand(context,"SELECT 5");
    	if(addFlag){
			std::string query= "HMSET cw_provision:channel_"+channel_id+":stream_id_"+stream_id+" stream_id "+stream_id+" channel_id "+channel_id+" cp_count 0 sent_status 0 key_index "+cw_index+" port "+port+" output "+output+"";
                std::cout<<"----------------------- "+channel_id+":"+stream_id<<std::endl;
                reply = (redisReply *)redisCommand(context,query.c_str());

                // std::string query1="EXPIREAT cw_provision:ch_"+channel_id+"";
                // reply = (redisReply *)redisCommand(context,query1.c_str());
        }else{
        	reply = (redisReply *)redisCommand(context,("DEL cw_provision:channel_"+channel_id+":stream_id_"+stream_id).c_str());
        }


    }
    Json::Value callSetPMTCADescriptor(Json::Value elementryPid,Json::Value JSON_CA_System_id,Json::Value ecm_pids, Json::Value private_data_lists,int rmx_no,std::string output,std::string input){
    	Json::Value json,jsonMsg,private_data_list;
    	 unsigned char RxBuffer[15]={0};
    	 int uLen;
    	 // elementryPid.append("3520");
    	jsonMsg["elementry_pids"] =elementryPid; 
    	 // JSON_CA_System_id.append(CA_System_id);
    	 // JSON_CA_System_id.append("10144");
    	jsonMsg["CA_System_ids"] =JSON_CA_System_id;
    	// ecm_pids.append(emm_pid);
    	// ecm_pids.append("2105");
    	jsonMsg["ecm_pids"] = ecm_pids;
    	// private_data_list.append(1);
    	// private_data_list.append(2);
    	// private_data_lists.append(private_data_list);
    	jsonMsg["private_data_list"] = private_data_lists;
        // std::cout<<JSON_CA_System_id;
    	int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        if(write32bCPU(0,0,target) != -1) { 
         Json::Value iojson = callSetInputOutput(input,output,rmx_no);
         if(iojson["error"] == false){
	    	 	uLen = c1.callCommand(50,RxBuffer,15,4090,jsonMsg,1);
	            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x4A || uLen != 6 || RxBuffer[4] < 1 || RxBuffer[5] != ETX ){
	                json["error"]= true;
	                json["message"]= "STATUS COMMAND ERROR 1!";
	            }           
	            else{

	                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
	                if (uLen != 1 ) {
	                    json["error"]= true;
	                    json["message"]= "STATUS COMMAND ERROR 2!";
	                }else{
	                    json["resp"]=RxBuffer[4];
	                    json["error"]=false;
	                    json["message"]= "Set CA descriptor for PMT table!";    
	                }
	            }
	        }else{
	        	json["error"] = true;
        		json["message"] = iojson["message"];
	        }
        }else{
        	json["error"] = true;
        	json["message"] = "Connection error!";	
        }

    	return json;
    }

    int allocateScramblingIndex(int indexValue){
    	unsigned i = 1, pos = 0;

		while((i & indexValue))
		{
			i = i<<1;
			++pos;
		}
		return pos;
    }
    /*****************************************************************************/
    /*  Commande 0x49   function getPMTCADescriptor                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  getPMTCADescriptor(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,Pids,private_data_list,JSON_CA_System_id,JSON_emm_pids;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"rmx_no","output","input"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getPMTCADescriptor",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
           	std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output =getParameter(request.body(),"output") ;
            std::string input =getParameter(request.body(),"input") ;

            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT,0);
            error[2] = verifyInteger(input,1,1,INPUT_COUNT,0);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Require Integer!";
            }
            if(all_para_valid){
     			json = callGetPMTCADescriptors(std::stoi(rmx_no),input,output);       	
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callGetPMTCADescriptors(int rmx_no,std::string input,std::string output){
    	Json::Value json,jsonMsg;
    	unsigned char RxBuffer[1024]={0};
    	unsigned int uLen;
    	int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        if(write32bCPU(0,0,target) != -1) { 
         Json::Value iojson = callSetInputOutput(input,output,rmx_no);
         if(iojson["error"] == false){
	    	 	uLen = c1.callCommand(50,RxBuffer,4090,15,jsonMsg,0);
	            if (RxBuffer[0] != STX || RxBuffer[3] != 0x4A){
	                json["error"]= true;
	                json["message"]= "STATUS COMMAND ERROR 1!";
	            }           
	            else{

	                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
	                if (uLen < 0 ) {
	                    json["error"]= true;
	                    json["message"]= "STATUS COMMAND ERROR 2!";
	                }else{
	                	json["error"]=false;
	                	Json::Value descriptor_list;unsigned int len=0;
	                	// std::cout<<(int)RxBuffer[15]<<std::endl;
	                	// std::cout<<uLen<<std::endl;
	                	for (int i = 4; i < uLen+4;i++)
	                	{
	                		Json::Value descriptor;unsigned int data_len=0;unsigned int ecm_pid,service_pid,CA_System_id;
	                		
	                		service_pid = (RxBuffer[i]<<8) | RxBuffer[i+1];
	                		// std::cout<<service_pid<<endl;
	                		descriptor["service_pid"] = service_pid;
	                		data_len = RxBuffer[i+3];
	                		// std::cout<<"DATA LEN"<<i+3<<"<=>"<<data_len<<endl;
	                		CA_System_id = (RxBuffer[i+4]<<8) | RxBuffer[i+5];
                			// std::cout<<CA_System_id<<endl; 
                			ecm_pid = (RxBuffer[i+6] & 0x1F)<<8 | RxBuffer[i+7];
                			descriptor["ecm_pid"] = ecm_pid;
                			// std::cout<<ecm_pid<<endl;
                			Json::Value private_data;
                			if(data_len>4){
		                		for (int j = 0; j < data_len-4; ++j)
		                		{
		                			private_data.append(RxBuffer[i+12+j]);
		                		}
		                	}
		                	descriptor["private_data"] = private_data;
	                		// len=4+data_len;
	                		std::string hex_ca_system_id = "0x"+getDecToHex(CA_System_id)+"0000";
	                		descriptor["CA_System_id"] = hex_ca_system_id;
	                		Json::Value ECMchannel_details = db->getECMChannelDetails(hex_ca_system_id,std::to_string(service_pid),std::to_string(ecm_pid),std::to_string(rmx_no),output);
	                		if(ECMchannel_details["error"] == false){
	                			descriptor["channel_id"] = ECMchannel_details["channel_id"];
	                			descriptor["stream_id"] = ECMchannel_details["stream_id"];
	                		}else{
	                			json["error"]=true;
	                			json["message"]= "Controller database is not in sync!";
	                		}
	                		descriptor_list.append(descriptor);
	                		i=i+7+data_len-4;
	                		// std::cout<<"length=====>"<<i<<endl;
	                	}
	                	json["ca_list"] = descriptor_list;	                    
	                    json["message"]= "Set CA descriptor for PMT table!";    
	                }
	            }
	        }else{
	        	json["error"] = true;
        		json["message"] = iojson["message"];
	        }
        }else{
        	json["error"] = true;
        	json["message"] = "Connection error!";	
        }

    	return json;
    }
    /*****************************************************************************/
    /*  Commande 0x49   function setCSAParity                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setCSAParity(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,Pids,private_data_list,JSON_CA_System_id,JSON_emm_pids;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"parity"};
        // int error[ sizeof(para) / sizeof(para[0])];
        // bool all_para_valid=true;
        // addToLog("setCSAParity",request.body());
        // std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        // if(res=="0"){
            	std::string parity =getParameter(request.body(),"parity");
            	
                // if(verifyInteger(parity,1,1,1,0)){
                	json = callSetCSAParity(parity);
                // }else{
                //     json["error"]= true;
                // }
            
        // }else{
        //     json["error"]= true;
        //     json["message"]= res;
        // }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetCSAParity(std::string parity){
    	Json::Value json;
    	bool error_flag = false;
    	// int remx_bit[]= {0,4,1,5,2,6};
    	int iParity = (std::stoi(parity) == 0)? 0 : 1;
    	for (int rmx_no = 0; rmx_no < RMX_COUNT; ++rmx_no)
    	{
    		Json::Value core_json;
    		// core_json = callSetCore("2","0",std::to_string(remx_bit[i]),i+1);
    		// if(core_json["status"] == 1){
    		int target =((0&0x3)<<8) | (((rmx_no)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        	if(write32bCPU(0,0,target) != -1) { 	
				Json::Value json_csa= callSetcsa("0","0",rmx_no+1,"255",std::to_string(iParity));
				if(json_csa["error"]== false)
					error_flag = false;
    			 // std::cout<<remx_bit[rmx_no]<<std::endl;
    		}
    		
    	}
    	json["error"] = error_flag;
    	return json;

    }
    // Json::Value setCSAParity(Json::Value elementryPid,Json::Value JSON_CA_System_id,Json::Value ecm_pids, Json::Value private_data_lists,int rmx_no,std::string output){
    // 	Json::Value json,jsonMsg,private_data_list;
    // 	 unsigned char RxBuffer[15]={0};
    // 	 int uLen;
    // 	 // elementryPid.append("3520");
    // 	jsonMsg["elementry_pids"] =elementryPid; 
    // 	 // JSON_CA_System_id.append(CA_System_id);
    // 	 // JSON_CA_System_id.append("10144");
    // 	jsonMsg["CA_System_ids"] =JSON_CA_System_id;
    // 	// ecm_pids.append(emm_pid);
    // 	// ecm_pids.append("2105");
    // 	jsonMsg["ecm_pids"] = ecm_pids;
    // 	// private_data_list.append(1);
    // 	// private_data_list.append(2);
    // 	// private_data_lists.append(private_data_list);
    // 	jsonMsg["private_data_list"] = private_data_lists;
    //     // std::cout<<JSON_CA_System_id;
    // 	int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
    //     if(write32bCPU(0,0,target) != -1) { 
    //      Json::Value iojson = callSetInputOutput("0",output,rmx_no);
    //      if(iojson["error"] == false){
	   //  	 	uLen = c1.callCommand(50,RxBuffer,15,4090,jsonMsg,1);
	   //          if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != 0x4A || uLen != 6 || RxBuffer[4] < 1 || RxBuffer[5] != ETX ){
	   //              json["error"]= true;
	   //              json["message"]= "STATUS COMMAND ERROR 1!";
	   //          }           
	   //          else{

	   //              uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
	   //              if (uLen != 1 ) {
	   //                  json["error"]= true;
	   //                  json["message"]= "STATUS COMMAND ERROR 2!";
	   //              }else{
	   //                  json["resp"]=RxBuffer[4];
	   //                  json["error"]=false;
	   //                  json["message"]= "Set CA descriptor for CAT table!";    
	   //              }
	   //          }
	   //      }else{
	   //      	json["error"] = true;
    //     		json["message"] = iojson["message"];
	   //      }
    //     }else{
    //     	json["error"] = true;
    //     	json["message"] = "Connection error!";	
    //     }

    // 	return json;
    // }
    /*****************************************************************************/
    /*  Commande 0x40   function setKeepProg                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setEncryptedServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,programNumbers,keyIDS;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"programNumbers","keyids","input","output","includeFlag","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setEncryptedServices",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
            std::string service_list = getParameter(request.body(),"programNumbers"); 
            std::string serviceList = UriDecode(service_list);
            std::string keyids = getParameter(request.body(),"keyids"); 
            std::string keyIds = UriDecode(keyids);
            bool parsedSuccess = reader.parse(serviceList,programNumbers,false);
            bool parsedSuccess1 = reader.parse(keyIds,keyIDS,false);
            if (parsedSuccess && parsedSuccess1)
            {
                std::string rmx_no =getParameter(request.body(),"rmx_no") ;
                // std::string programNumbers = getParameter(request.body(),"programNumbers"); 
                std::string input = getParameter(request.body(),"input"); 
                std::string output = getParameter(request.body(),"output"); 
                std::string includeFlag = getParameter(request.body(),"includeFlag"); 
                error[0] = verifyJsonArray(programNumbers,"programNumber",1);
                error[1] = verifyJsonArray(keyIDS,"keys",1);
                error[2] = verifyInteger(input,1,1);
                error[3] = verifyInteger(output,1,1);
                error[4] = verifyInteger(includeFlag,1,1);
                error[5] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= "Require Integer!";
                }
                if(all_para_valid){
                    json = callsetEncryptedServices(programNumbers["programNumber"],keyIDS["keys"],input,output,std::stoi(rmx_no),std::stoi(includeFlag));
                }else{
                    json["error"]= true;
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalide json input!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callsetEncryptedServices(Json::Value programNumbers,Json::Value keyIDS,std::string input,std::string output,int rmx_no, int includeFlag = 1){
        unsigned char RxBuffer[261]={0};
        int uLen;
        Json::Value json,jsonMsg,root,root1;
        std::string prog_nos_str,key_str;
        Json::Value iojson = callSetInputOutput(input,output,rmx_no); 
        for(int i = 0; i<programNumbers.size();i++){
            prog_nos_str= prog_nos_str+','+programNumbers[i].asString();
            if(includeFlag)
                root.append(programNumbers[i].asString());
        }

        prog_nos_str = prog_nos_str.substr(1);

        for(int i = 0; i<keyIDS.size();i++){
            key_str= key_str+','+keyIDS[i].asString();
            if(includeFlag)
                root1.append(keyIDS[i].asString());
        }
        
        key_str = key_str.substr(1);
        if(iojson["error"]==false){
            Json::Value active_prog = db->getEncryptedPrograms(prog_nos_str,input,output,std::to_string(rmx_no));
            if(active_prog["error"]==false){
                for (int i = 0; i < active_prog["list"].size(); ++i)
                {
                    root.append(active_prog["list"][i].asString());
                    std::cout<<"-> "<<active_prog["list"][i].asString()<<" ";
                }
            }
            jsonMsg["programNumbers"] = root; 
            jsonMsg["keyids"] = root1; 
            uLen = c1.callCommand(45,RxBuffer,6,4090,jsonMsg,1);
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_ENCRYPT_PROG_STATE || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR 1!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR 2!";
                }else{
                    // Json::Value pnameList;
                    // if(includeFlag){
                    //     for(int i = 0; i<programNumbers.size();i++){
                    //         Json::Value jpnames = callGetProgramOriginalName(programNumbers[i].asString(),rmx_no);
                    //         if(jpnames["error"] == false){
                    //             pnameList[programNumbers[i].asString()] = jpnames["name"];
                    //         }else{
                    //             pnameList[programNumbers[i].asString()] = -1;
                    //         }
                    //     }
                    // }
                    // json["names"] = pnameList;
                	json["error"]= false;
					addToLog("addEncryptedPrograms","Success");
                    db->addEncryptedPrograms(input,output,programNumbers,rmx_no,includeFlag,prog_nos_str,keyIDS);
                    json["message"]= "Set encrypted programs!";    
                }
            }
        }
        return json;
    }
     void  setEncrypteService(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,programNumbers,keyIDS;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"programNumber","input","output","includeFlag","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        // std::cout<<request.body()<<endl;
        addToLog("setEncryptedServices",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
           
    		std::string programNumber =getParameter(request.body(),"programNumber") ;
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string input = getParameter(request.body(),"input"); 
            std::string output = getParameter(request.body(),"output"); 
            std::string includeFlag = getParameter(request.body(),"includeFlag"); 
            error[0] = verifyInteger(programNumber,0,0,65535,1);
            error[1] = verifyInteger(input,1,1);
            error[2] = verifyInteger(output,1,1);
            error[3] = verifyInteger(includeFlag,1,1);
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Require Integer!";
            }
            if(all_para_valid){
            	unsigned int flag = std::stoi(includeFlag);
            	long int key_index;
            	int uiRmx_no = std::stoi(rmx_no);
	        	unsigned long int indexValue =  db->getScramblingIndex(rmx_no,output);
	        	if(flag){
	        		key_index = db->getCWKeyIndex(rmx_no,input,output,programNumber);
	        		if(key_index == -1){
	        			key_index = allocateScramblingIndex(indexValue);
	        			// if(uiRmx_no%2 == 0)
	        			// 	key_index+=128;
	        		}
	        	}else{
	        		key_index = db->getCWKeyIndex(rmx_no,input,output,programNumber);
	        	}
	        	db->addEncryptedService(rmx_no,input,output,programNumber,key_index,flag);
                json = callsetEncryptedServices(programNumber,key_index,input,output,uiRmx_no,flag);
                // json["error"] = false;
                if(json["error"] == false){
		        	db->updateCWIndex(rmx_no,output,programNumber,key_index,indexValue,flag);
                }
            }else{
                json["error"]= true;
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
   Json::Value callsetEncryptedServices(std::string programNumbers,long int keyID,std::string input,std::string output,int rmx_no, int flag){
	   	Json::Value json,service_nos,key_indexes,jsonMsg;
	   	unsigned int uLen;
	   	unsigned char RxBuffer[4090]={0};
   		Json::Value active_prog = db->getEncryptedPrograms("-1",input,output,std::to_string(rmx_no));
   		std::cout<<active_prog<<std::endl;
        if(active_prog["error"]==false){
            for (int i = 0; i < active_prog["list"].size(); ++i)
            {
                service_nos.append(active_prog["list"][i]["service_no"].asString());
                key_indexes.append(active_prog["list"][i]["key_index"].asString());
            }
        }
        std::cout<<service_nos<<std::endl;
        jsonMsg["programNumbers"] = service_nos; 
        jsonMsg["keyids"] = key_indexes; 
        Json::Value iojson = callSetInputOutput(input,output,rmx_no);
        if(iojson["error"] == false){
	        uLen = c1.callCommand(45,RxBuffer,6,4090,jsonMsg,1);
	        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_ENCRYPT_PROG_STATE || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ){
	            json["error"]= true;
	            json["message"]= "STATUS COMMAND ERROR 1!";
	        }            
	        else{
	            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
	            if (uLen != 1 ) {
	                json["error"]= true;
	                json["message"]= "STATUS COMMAND ERROR 2!";
	            }else{
	            	json["error"]= false;
					addToLog("addEncryptedPrograms","Success");
	                json["message"]= "Set encrypted programs!";    
	            }
	        }
	    }else{
	    	json["error"]= true;
            json["message"]= "Connection error!";
	    }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x45   function getEncryptedServices                        */
    /*****************************************************************************/
    void getEncryptedServices(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
       
       std::string para[] = {"input","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setEncryptedServices",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
           
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string input = getParameter(request.body(),"input"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(input,1,1);
            error[1] = verifyInteger(output,1,1);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Require Integer!";
            }
	        if(all_para_valid){
	           
	           json =callgetEncryptedServices(std::stoi(rmx_no),input,output);
	           
	        }else{
	            json["error"] = true;
	        }
	    }else{
	    	json["error"] = true;
	    	json["message"] = res;
	    }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callgetEncryptedServices(int rmx_no,std::string input,std::string output){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,keys;
        int num_prog;
        Json::Value iojson = callSetInputOutput(input,output,rmx_no);
        if(iojson["error"] == false){
	 		int uLen = c1.callCommand(45,RxBuffer,4090,7,json,0);

	        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != CMD_ENCRYPT_PROG_STATE) {
	            json["error"]= true;
	            json["message"]= "STATUS COMMAND ERROR!";
	            addToLog("getEncryptedServices","Error");
	        }else{
	            uLen = ((RxBuffer[1] << 8) | (RxBuffer[2]));
	            num_prog = uLen/3;
	            // Json::Value jsonNewIds;
	            // jsonNewIds = callGetServiceID(rmx_no);

	            for(int i=0; i<num_prog; i++) {
	                Json::Value jsondata,jservName;
	                // pProg[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
	                int pnum = (RxBuffer[3*i+4]<<8)|(RxBuffer[3*i+5]);
	                int keynum= (RxBuffer[3*i+6]);

	                pProg[i] = pnum;
	                keys[i]=keynum;
	            }
	            json["pProg"] = pProg;
	            json["keys"] = keys;
	            json["error"] = false;
	            json["message"] = "Get EncryptedServices!";
	            addToLog("getEncryptedServices","Success");
	        }
        }else{
	    	json["error"]= true;
	        json["message"]= "Connection error!";
    	}
        return json;
    }

    /*****************************************************************************/
    /*  Commande 0x45   function getEncryptedServices                        */
    /*****************************************************************************/
    void getCustomPids(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =callgetCustomPids(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callgetCustomPids(int rmx_no){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,keys;
        int num_prog;
        int uLen = c1.callCommand(48,RxBuffer,4090,7,json,0);
       
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x48) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getCustomPids","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | (RxBuffer[2]));
            num_prog = uLen/3;
            Json::Value jsonNewIds;
            

            for(int i=0; i<num_prog; i++) {
                Json::Value jsondata,jservName;
                // pProg[i] = (RxBuffer[2*i+4]<<8)|RxBuffer[2*i+5];
                int pnum = (RxBuffer[3*i+4]<<8)|(RxBuffer[3*i+5]);
                int keynum= (RxBuffer[3*i+6]);

                pProg[i] = pnum;
                keys[i]=keynum;
            }
            json["pids"] = pProg;
            json["auth_outputs"] = keys;
            json["error"] = false;
            json["message"] = "Get getCustomPids!";
            addToLog("getCustomPids","Success");
        }
        return json;
    }
   
    /*****************************************************************************/
    /*  Commande 0x46   function initCsa                        */
    /*****************************************************************************/
    void initCsa(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =callinitCsa(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callinitCsa(int rmx_no){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,keys;
        int num_prog;
        int uLen = c1.callCommand(46,RxBuffer,4090,7,json,1);
       
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x46) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("initCsa","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | (RxBuffer[2]));
            json["error"] = false;
            json["message"] = "intialized CSA!";
            addToLog("initCsa","Success");
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x46   function getinitCsa                        */
    /*****************************************************************************/
    void getinitCsa(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        int rmx_no = request.param(":rmx_no").as<int>();
        if(rmx_no > 0 && rmx_no <= 6){
            int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                json =callgetinitCsa(rmx_no);
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"] = false;
            json["message"] = "Invalid remux id";
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callgetinitCsa(int rmx_no){
        unsigned char RxBuffer[4090]={0};
        
        Json::Value json;
        Json::Value pProg,keys;
        int num_prog;
        int uLen = c1.callCommand(46,RxBuffer,4090,7,json,0);
        
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x46) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getinitCsa","Error");
        }else{
            uLen = ((RxBuffer[1] << 8) | (RxBuffer[2]));
            json["length"]=uLen;
            json["error"] = false;
            json["message"] = "getinitCsa CSA!";
            addToLog("getinitCsa","Success");
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x40   function setKeepProg                       
        includeFlag 1-is for add, 0 - is for delete
      */
    /*****************************************************************************/
    void  setKeepProg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,programNumbers;
        Json::FastWriter fastWriter;
        Json::Reader reader;
        std::string para[] = {"programNumbers","input","output","includeFlag","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setKeepProg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            
            std::string service_list = getParameter(request.body(),"programNumbers"); 
            std::string serviceList = UriDecode(service_list);
            bool parsedSuccess = reader.parse(serviceList,programNumbers,false);
            if (parsedSuccess)
            {
                std::string rmx_no =getParameter(request.body(),"rmx_no") ;
                // std::string programNumbers = getParameter(request.body(),"programNumbers"); 
                std::string input = getParameter(request.body(),"input"); 
                std::string output = getParameter(request.body(),"output"); 
                std::string includeFlag = getParameter(request.body(),"includeFlag"); 
                error[0] = verifyJsonArray(programNumbers,"programNumber",1);
                error[1] = verifyInteger(input,1,1);
                error[2] = verifyInteger(output,1,1);
                error[3] = verifyInteger(includeFlag,1,1);
                error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
                for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
                {
                   if(error[i]!=0){
                        continue;
                    }
                    all_para_valid=false;
                    json["error"]= true;
                    json[para[i]]= "Require Integer!";
                }
                if(all_para_valid){
                    int iFlag = std::stoi(includeFlag);
                    json = callSetKeepProg(programNumbers["programNumber"],input,output,std::stoi(rmx_no),iFlag);
                    if(json["error"] == false && iFlag != 1)
                    {
                        for (int service_index = 0; service_index < programNumbers["programNumber"].size(); ++service_index)
                        {
                            std::string pNumber = programNumbers["programNumber"][service_index].asString();
                            if(db->isServiceActivated(pNumber,rmx_no,input) == 0){
                                std::cout<<"_---------------------------RESET SERVICE DETAILS---------------"<<endl;
                                db->removeServiceIdName(pNumber,rmx_no,input);
                                updateServiceIDs(rmx_no,input);
                                resetServiceName(pNumber);
                                resetServiceProviderName(pNumber);
                            }
                        }
                    }
                }else{
                    json["error"]= true;
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalide json input!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetKeepProg(Json::Value programNumbers,std::string input,std::string output,int rmx_no, int includeFlag = 1){
        unsigned char RxBuffer[261]={0};
        int uLen;
        Json::Value json,jsonMsg,root;
        std::string prog_nos_str;
        Json::Value iojson = callSetInputOutput(input,output,rmx_no); 
        for(int i = 0; i<programNumbers.size();i++){
            prog_nos_str= prog_nos_str+','+programNumbers[i].asString();
            if(includeFlag)
                root.append(programNumbers[i].asString());
        }
        prog_nos_str = prog_nos_str.substr(1);
        if(iojson["error"]==false){
            Json::Value active_prog = db->getActivePrograms(prog_nos_str,input,output,std::to_string(rmx_no));
            if(active_prog["error"]==false){
                for (int i = 0; i < active_prog["list"].size(); ++i)
                {
                    root.append(active_prog["list"][i].asString());
                    // std::cout<<"-> "<<active_prog["list"][i].asString()<<" ";
                }
            }
            jsonMsg["programNumbers"] = root; 
            uLen = c1.callCommand(40,RxBuffer,6,4090,jsonMsg,1);
            if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_FILT_PROG_STATE || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ){
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
            }            
            else{
                uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
                if (uLen != 1 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                }else{
                    // Json::Value pnameList;
                    // if(includeFlag){
                    //     for(int i = 0; i<programNumbers.size();i++){
                    //         Json::Value jpnames = callGetProgramOriginalName(programNumbers[i].asString(),rmx_no);
                    //         if(jpnames["error"] == false){
                    //             pnameList[programNumbers[i].asString()] = jpnames["name"];
                    //         }else{
                    //             pnameList[programNumbers[i].asString()] = -1;
                    //         }
                    //     }
                    // }
                    // json["names"] = pnameList;
                    json["error"]= false;
                    addToLog("setKeepProg","Success");
                    db->addActivatedPrograms(input,output,programNumbers,rmx_no,includeFlag,prog_nos_str);
                    json["message"]= "Set keep programs!";    
                }
            }
        }
        return json;
    }

    Json::Value updateServiceIDs(std::string rmx_no,std::string input){
        Json::Value json, NewService_ids, new_service_ids,old_service_ids;
        NewService_ids = db->getServiceIds(std::stoi(rmx_no),std::stoi(input));
        // std::cout<<NewService_ids<<endl;
        if(NewService_ids["error"]==false){
            for (int i = 0; i < NewService_ids["list"].size(); ++i)
            {
                new_service_ids[i] = NewService_ids["list"][i]["service_id"].asString();
                old_service_ids[i] = NewService_ids["list"][i]["channel_number"].asString();
            }   
            json = callSetServiceID(old_service_ids,new_service_ids,std::stoi(rmx_no));
            if(json["error"]==false){
                std::cout<<"------------------ Service ID's restored successfully ------------------"<<std::endl;
            } 
        }   
        return json; 
    }
    Json::Value resetServiceName(std::string progNumber){
        Json::Value jsonMsg,json;
        unsigned char RxBuffer[15]={0};
        int uLen;

        jsonMsg["progNumber"] = progNumber;
        uLen = c1.callCommand(19,RxBuffer,10,8,jsonMsg,2);
        if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != 0x19 || uLen != 6 || RxBuffer[4] != 1 ||  RxBuffer[5] != ETX){
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!1"+getDecToHex((int)RxBuffer[4]);
        }            
        else{
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("setServiceName","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "Set new name!";                                
                addToLog("setServiceName","Success");
            }
        }
        return json;
    }
    Json::Value resetServiceProviderName( std::string serviceNumber){
        Json::Value jsonMsg,json;
        unsigned char RxBuffer[15]={0};
        int uLen;

        jsonMsg["serviceNumber"] = serviceNumber;
        uLen = c1.callCommand(26,RxBuffer,10,8,jsonMsg,2);
        if (!uLen|| RxBuffer[0] != STX || RxBuffer[3] != CMD_CHANGE_PROV || uLen != 6 || RxBuffer[4] != 1 || RxBuffer[5] != ETX ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
        }            
        else{
            addToLog("callSetServiceProvider",serviceNumber);
            uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
            if (uLen != 1 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("callSetServiceProvider","Error");
            }else{
                json["status"] = RxBuffer[4];
                json["error"]= false;
                json["message"]= "set provider name";  
                addToLog("callSetServiceProvider","Success");
            }
        }
        return json;

    }
    /*****************************************************************************/
    /*  Commande 0x55(85)   function setKeepProg                         */
    /*****************************************************************************/
    void  downloadTables(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[255 * 1024]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        
        std::string para[]={"type","table","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        int error_range[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("downloadTables",request.body());
        std::string res=validateRequiredParameter(request.body(), para,2);
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string type = getParameter(request.body(),"type");
            jsonMsg["type"] = type;
            std::string table = getParameter(request.body(),"table"); 
            jsonMsg["table"] = table;
            std::string output = getParameter(request.body(),"output"); 

            error[0] = verifyInteger(type,1,1,5);
            error[1] = verifyInteger(table);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 1-6!" :((i == 2)?"Require Integer between 1 to 6!":"Require Integer between 0 to 65535!");
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                	callSetInputOutput("0",output,std::stoi(rmx_no));
                    uLen = c1.callCommand(85,RxBuffer,255 * 1024,11,jsonMsg,1);
                    if (!uLen|| RxBuffer[0] != STX || std::stoi(getDecToHex((int)RxBuffer[3])) != 55){
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!1";
                    }            
                    else{
                        uLen = ((RxBuffer[1]<<8) | RxBuffer[2]); 
                        int num_prog = uLen;
                        std::stringstream ss;
                        std::string res;
                        
                        if (uLen == 0 ) {
                            json["error"]= uLen;
                            json["message"]= "STATUS COMMAND ERROR!";
                            addToLog("downloadTables","Error");
                        }else{
                            for(int i=0; i<uLen; i++) {
                                std::string byte;
                                byte = getDecToHex((int)RxBuffer[i+4]);
                                if(byte.length()<2)
                                    byte='0'+byte;
                                res+=byte;
                            }
                            json["data"] = res;
                            json["error"]=false;
                            json["message"]= "download Done";
                            addToLog("downloadTables","Success");          
                        }
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

     /*****************************************************************************/
    /*  Commande 0x10   function setTDT_TOT                           */
    /*****************************************************************************/
    void  setTDT_TOT(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[6]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        

        // std::string para[] = {"mode","output","rmx_no"};
        // int error[ sizeof(para) / sizeof(para[0])];
        // bool all_para_valid=true;
        // addToLog("setTDT_TOT",request.body());
        // std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        // if(res=="0"){
        //     std::string mode = getParameter(request.body(),"mode"); 
        //     std::string output = getParameter(request.body(),"output");
        //     std::string rmx_no =getParameter(request.body(),"rmx_no") ; 
            
        //     error[0] = verifyInteger(mode,1,1);
        //     error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
        //     error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
        //     for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
        //     {
        //        if(error[i]!=0){
        //             continue;
        //         }
        //         all_para_valid=false;
        //         json["error"]= true;
        //         json[para[i]]= (i==0)? "Require Integer between 1-6!" :(i==1) ? "Require Integer" : "Require Integer between 0 - "+std::to_string(INPUT_COUNT)+"!" ;
        //     }
        //     if(all_para_valid){
        //         Json::Value iojson=callSetInputOutput("0",output,std::stoi(rmx_no));
        //         if(iojson["error"]==false){
        //             json=callSetNITmode(mode,output,std::stoi(rmx_no));
        //             if(json["error"] == false)
        //                 db->addNitMode(mode,output,std::stoi(rmx_no)); 
        //         }else{
        //             json["error"]= true;
        //             json["message"]= "Error while selecting output!";     
        //         }
        //     }
        // }else{
        //     json["error"]= true;
        //     json["message"]= res;
        // }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    // Json::Value callSetNITmode(std::string mode,std::string output,int rmx_no){
    //     unsigned char RxBuffer[6]={0};
    //     int uLen;
    //     Json::Value json,jsonMsg;
    //     jsonMsg["Mode"] = mode;
    //     json["error"]= false;
    //     uLen = c1.callCommand(10,RxBuffer,6,6,jsonMsg,1);
                     
    //     if (!uLen || RxBuffer[0] != STX || RxBuffer[3] != CMD_INIT_NIT_MODE || uLen != 6 || RxBuffer[5] != ETX){
    //         json["error"]= true;
    //         json["message"]= "STATUS COMMAND ERROR!";
    //     }            
    //     else{
    //         uLen = ((RxBuffer[1]<<8) | RxBuffer[2]);
    //         if (uLen != 1 ) {
    //             // json["error"]= true;
    //             json["message"]= "STATUS COMMAND ERROR!";
    //             addToLog("setNitMode","Error");
    //         }else{
    //             json["status"] = RxBuffer[4];
    //             json["error"]= false;
    //             json["message"]= "set Nit Mode!!";    
                     
    //             addToLog("setTDT_TOT","Success");
    //         }
    //     }
    //     return json;
    // }

    // UDP STACK DEFINITIONS

    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x08   function getVersionofcore                          */
    /*****************************************************************************/
    void getVersionofcore(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(8,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getVersionofcore","Error");
        }else{
            json["verion_0"] = RxBuffer[8];
            json["verion_1"] = RxBuffer[9];
            json["verion_2"] = RxBuffer[10];
            json["verion_3"] = RxBuffer[11];
            json["error"]= false;
            json["message"]= "verion of the core!";
            addToLog("getVersionofcore","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x24,0x28,0x2C,0x30   function setEthernetOut                      */
    /*****************************************************************************/
    void  setEthernetOut(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address","port","channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetOut",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            std::string port = getParameter(request.body(),"port"); 
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = isValidIpAddress(ip_address.c_str());
            error[1] = verifyInteger(port,0,0,65535,1000);
            error[2] = verifyInteger(channel_no,0,0,16,1);
            error[3] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Invalid IP address!" :((i == 2)?"Require Integer between 1 to 9!":"Require Integer between 1000 to 65535!");
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    std::string hex_ip_part1,hex_ip_part2,hex_ip_part3,hex_ip_part4,hex_ip; 
                    std::stringstream split_str(ip_address);
                    unsigned int ip_part1,ip_part2,ip_part3,ip_part4; //to store the 4 ints
                    unsigned char ch; //to temporarily store the '.'
                    split_str >> ip_part1 >> ch >> ip_part2 >> ch >> ip_part3 >> ch >> ip_part4;
                    hex_ip_part1 = getDecToHex(ip_part1);
                    hex_ip_part1 = (hex_ip_part1.length() ==2)? hex_ip_part1 : "0"+hex_ip_part1;
                    hex_ip_part2 = getDecToHex(ip_part2);
                    hex_ip_part2 = (hex_ip_part2.length() ==2)? hex_ip_part2 : "0"+hex_ip_part2;
                    hex_ip_part3 = getDecToHex(ip_part3);
                    hex_ip_part3 = (hex_ip_part3.length() ==2)? hex_ip_part3 : "0"+hex_ip_part3;
                    hex_ip_part4 = getDecToHex(ip_part4);
                    hex_ip_part4 = (hex_ip_part4.length() ==2)? hex_ip_part4 : "0"+hex_ip_part4;
                    hex_ip = hex_ip_part1 +""+hex_ip_part2+""+hex_ip_part3+""+hex_ip_part4;
                    unsigned long int ip_addr = getHexToLongDec(hex_ip);
                    int rmx_no_of_controller = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((rmx_no_of_controller)*8)+std::stoi(channel_no);
                    json = callSetEthernetOut(ip_addr,std::stoi(port),ch_no);
                    if(json["error"] == false){
                        db->addIPOutputChannels(rmx_no,std::stoi(channel_no),std::to_string(ip_addr),std::stoi(port));
                    }
                    json["sreadport"] = read32bI2C(2,40);
                     usleep(1000);
                    json["dreadport"] = read32bI2C(2,44);
                     usleep(1000);
                    json["readch"] = read32bI2C(2,48);
                     usleep(1000);
                    json["readip"] = (read32bI2C(2,36)&0xFFFFFFFF);
                     usleep(1000);
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetEthernetOut(unsigned long int ip_addr, int port,int channel_no ){
        Json::Value json;
        json["message"]= "successfully assigned IP OUT!";
        if (write32bI2C (2,36,ip_addr) == -1) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("setEthernetOut IP","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOut","Success");
        }
        usleep(1000);
        //Source Port
        if (write32bI2C(2,40, port) == -1) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("setEthernetOut IP source port","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOut","Success");
        }
        usleep(1000);
        // Destination Port
        if ( write32bI2C(2,44,port) == -1) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("setEthernetOut IP destination port","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOut","Success");
        }
        usleep(1000);
        //Validation
        if (write32bI2C(2,48,channel_no) == -1) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("setEthernetOut IP channel","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOut","Success");
        }
        return json;
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command    function getEthernetOut                      */
    /*****************************************************************************/
    void  getEthernetOut(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("getEthernetOut",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            
           std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(channel_no,0,0,16,1);
            error[1] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 1 to 8!" :"Require Integer between 1 to 6!";
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int rmx_no_of_controller = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((rmx_no_of_controller)*8)+std::stoi(channel_no);
                    json["error"]= false;
                    //Validation
                    if (write32bI2C(3,0,ch_no) == -1) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("setEthernetOut IP channel","Error");
                    }else{
                        json["error"]= false;
                        addToLog("setEthernetOut","Success");
                    }
                    unsigned long int ip_addr= (read32bI2C(3,4)&0xFFFFFFFF);
                    if (ip_addr == -1) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("getEthernetOut IP","Error");
                    }else{
                        json["ip"]=std::to_string(ip_addr);
                        addToLog("setEthernetOut","Success");
                    }
                    unsigned int src_port = read32bI2C(3,8);
                    if (src_port == -1) {
                        json["error"]= true;
                        json["message"]= "STATUS COMMAND ERROR!";
                        addToLog("getEthernetOut","Error");
                    }else{
                        std::string ports = getDecToHex(src_port);
                        json["src_port"]= ports.substr(0,4);
                        json["dst_port"]= ports.substr(4);
                        addToLog("setEthernetOut","Success");
                    }
                    
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command    function setEthernetOutOff                      */
    /*****************************************************************************/
    void  setEthernetOutOff(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetOutOff",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            // connectI2Clines(14);
            std::string channel_no = getParameter(request.body(),"channel_no"); 
            std::string str_rmx_no = getParameter(request.body(),"rmx_no"); 
            if(verifyInteger(channel_no,1,1,9,1) && verifyInteger(str_rmx_no,1,1,RMX_COUNT,1)){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int rmx_no_of_controller = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((rmx_no_of_controller)*8)+std::stoi(channel_no);
                    json = callSetEthernetOutOff(ch_no);
                    if(json["error"] == false){
                        db->removeIPOutputChannels(rmx_no,std::stoi(channel_no));
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!"; 
                }
            }else{
                json["error"]= true;
                json["message"]= "channel_no & rmx_no: Required Integer!";    
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetEthernetOutOff(int channel_no ){
        Json::Value json;
        json["message"]= "Ethernet Out torned off!";
        if(write32bI2C (2,36,0) == -1){
            json["error"]= true;
            json["message"]= "Error while torning off ethernet out!";
            addToLog("setEthernetOutOff ChannelNumber","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOutOff ChannelNumber","Success");
        } 
        usleep(1000);
        //Validation
        if(write32bI2C(2,48,channel_no) == -1){
            json["error"]= true;
            json["message"]= "Error while torning off ethernet out!";
            addToLog("setEthernetOutOff","Error");
        }else{
            json["error"]= false;
            addToLog("setEthernetOutOff","Success");
        }
        return json;
    }
     /*****************************************************************************/
    /*  UDP Ip Stack Command    function setEthernetIn                      */
    /*****************************************************************************/
    void  setEthernetIn(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address","port","channel_no","rmx_no","type"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetIn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            std::string port = getParameter(request.body(),"port"); 
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            std::string str_type = getParameter(request.body(),"type");
            error[0] = isValidIpAddress(ip_address.c_str());
            error[1] = verifyInteger(port,0,0,65535,1000);
            error[2] = verifyInteger(channel_no,0,0,16,1);
            error[3] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[4] = verifyInteger(str_type,1,1,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Invalid IP address!" :((i == 2)?"Require Integer between 1 to 9!":((i==1)? "Require Integer between 1000 to 65535!":"Required Integer between 0-1"));
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    std::string hex_ip_part1,hex_ip_part2,hex_ip_part3,hex_ip_part4,hex_ip; 
                    std::stringstream split_str(ip_address);
                    unsigned int ip_part1,ip_part2,ip_part3,ip_part4; //to store the 4 ints
                    unsigned char ch; //to temporarily store the '.'
                    split_str >> ip_part1 >> ch >> ip_part2 >> ch >> ip_part3 >> ch >> ip_part4;
                    hex_ip_part1 = getDecToHex(ip_part1);
                    hex_ip_part1 = (hex_ip_part1.length() ==2)? hex_ip_part1 : "0"+hex_ip_part1;
                    hex_ip_part2 = getDecToHex(ip_part2);
                    hex_ip_part2 = (hex_ip_part2.length() ==2)? hex_ip_part2 : "0"+hex_ip_part2;
                    hex_ip_part3 = getDecToHex(ip_part3);
                    hex_ip_part3 = (hex_ip_part3.length() ==2)? hex_ip_part3 : "0"+hex_ip_part3;
                    hex_ip_part4 = getDecToHex(ip_part4);
                    hex_ip_part4 = (hex_ip_part4.length() ==2)? hex_ip_part4 : "0"+hex_ip_part4;
                    hex_ip = hex_ip_part1 +""+hex_ip_part2+""+hex_ip_part3+""+hex_ip_part4;
                    unsigned long int ip_addr = getHexToLongDec(hex_ip);
                    json["dec"] =std::to_string(ip_addr);
                    //Multicast IP
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    int rmx_no1,rmx_no2;
                    int tuner_ch= db->getTunerChannelType(control_fpga,rmx_no,std::stoi(channel_no)-1,1);
                    (control_fpga == 1)? (rmx_no1 = 1,rmx_no2 = 2) : ((control_fpga == 2)? (rmx_no1 = 3,rmx_no2 = 4) : (rmx_no1 = 5,rmx_no2 = 6));
                    json["rmx_no1"] = rmx_no1;
                    json["rmx_no2"] = rmx_no2;
                    int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((controller_of_rmx)*8)+std::stoi(channel_no);
                    json = callSetEthernetIn(ch_no,ip_addr,std::stoi(port),std::stoi(str_type));
                    usleep(1000);
                    if(write32bI2C(4,0,tuner_ch) == -1){
                        json["error"]= true;
                        json["message"]= "Failed IGMP Channel Number!";
                    }
                    if(json["error"] == false){
                        db->addIPInputChannels(rmx_no,std::stoi(channel_no),std::to_string(ip_addr),std::stoi(port),std::stoi(str_type));
                    }
                    long int mux_out =  0; 
                    Json::Value json_mux_out = db->getMuxOutValue(std::to_string(control_fpga));
                    if(json_mux_out["error"] == false){
                        int is_enable = std::stoi(json_mux_out["is_enable"].asString());
                        long int custom_line = std::stol(json_mux_out["custom_line"].asString());
                        mux_out = (is_enable == 0)?  custom_line : (custom_line | 65536 );
                    }
                    if(write32bI2C(5,0,mux_out) == -1){
                        json["error"]= true;
                        json["message"]= "Failed MUX OUT!";
                    }
                    json["tuner_ch"] = tuner_ch;
                    json["target"] = target;
                    json["control_fpga"] = control_fpga;
                    // json["channel_no"] = read32bI2C(6,4);
                    // json["ip"] = (read32bI2C(6,8))&0xFFFFFFFF;
                    // json["port"] = read32bI2C(6,12);
                    // json["channel"] = read32bI2C(6,16);
                    // usleep(1000);
                    // json["igmpip"] = (read32bI2C(2,28))&0xFFFFFFFF;
                    // json["igmpch"] = read32bI2C(2,32);
                    // json["input"] = read32bI2C(4,0);
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetEthernetIn(int channel_no,unsigned long int ip_addr,int port, int type,int tuner_ch=0){
        Json::Value json;
        json["error"]= false;
        json["message"]= "successfully assigned IP OUT!";
        if(write32bI2C(6, 4,channel_no) == -1){
            json["error"]= true;
            json["message"]= "Fail to set channel_no!";
        }
        usleep(1000);
        if(write32bI2C(6,8,ip_addr) == -1){
            json["error"]= true;
            json["message"]= "Fail to set IP address!";
        }
        usleep(1000);
        if(write32bI2C(6,12,port) == -1){
            json["error"]= true;
            json["message"]= "Fail to set port!";
        }
        usleep(1000);
        if(write32bI2C(6,16, 1) == -1){
            json["error"]= true;
            json["message"]= "Ethernet out failed!";
        }
        usleep(1000);
        if(type == 1)//If Multicat 
        {
            // IGMP multicast ip
            if(write32bI2C(2,28,ip_addr) == -1){
                json["error"]= true;
                json["message"]= "Failed IGMP multicast ip!";
            }
            usleep(1000);
            // IGMP Channel Number
            if(write32bI2C(2,32,channel_no) == -1){
                json["error"]= true;
                json["message"]= "Failed IGMP Channel Number!";
            }
        }
        // usleep(1000);
        // if(write32bI2C(4,0,tuner_ch) == -1){
     //     json["error"]= true;
     //     json["message"]= "Failed IGMP Channel Number!";
     //    }
        return json;
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command    function setSPTSEthernetIn                      */
    /*****************************************************************************/
    void  setSPTSEthernetIn(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address","port","channel_no","rmx_no","type"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetIn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            std::string port = getParameter(request.body(),"port"); 
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            std::string str_type = getParameter(request.body(),"type");
            error[0] = isValidIpAddress(ip_address.c_str());
            error[1] = verifyInteger(port,0,0,65535,1000);
            error[2] = verifyInteger(channel_no,0,0,31,17);
            error[3] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[4] = verifyInteger(str_type,1,1,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Invalid IP address!" :((i == 2)?"Require Integer between 17 to 31!":((i==1)? "Require Integer between 1000 to 65535!":"Required Integer between 0-1"));
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    std::string hex_ip_part1,hex_ip_part2,hex_ip_part3,hex_ip_part4,hex_ip; 
                    std::stringstream split_str(ip_address);
                    unsigned int ip_part1,ip_part2,ip_part3,ip_part4; //to store the 4 ints
                    unsigned char ch; //to temporarily store the '.'
                    split_str >> ip_part1 >> ch >> ip_part2 >> ch >> ip_part3 >> ch >> ip_part4;
                    hex_ip_part1 = getDecToHex(ip_part1);
                    hex_ip_part1 = (hex_ip_part1.length() ==2)? hex_ip_part1 : "0"+hex_ip_part1;
                    hex_ip_part2 = getDecToHex(ip_part2);
                    hex_ip_part2 = (hex_ip_part2.length() ==2)? hex_ip_part2 : "0"+hex_ip_part2;
                    hex_ip_part3 = getDecToHex(ip_part3);
                    hex_ip_part3 = (hex_ip_part3.length() ==2)? hex_ip_part3 : "0"+hex_ip_part3;
                    hex_ip_part4 = getDecToHex(ip_part4);
                    hex_ip_part4 = (hex_ip_part4.length() ==2)? hex_ip_part4 : "0"+hex_ip_part4;
                    hex_ip = hex_ip_part1 +""+hex_ip_part2+""+hex_ip_part3+""+hex_ip_part4;
                    unsigned long int ip_addr = getHexToLongDec(hex_ip);
                    json["dec"] =std::to_string(ip_addr);
                    //Multicast IP
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    int rmx_no1,rmx_no2;
                    int tuner_ch=65536; //db->getTunerChannelType(control_fpga,rmx_no,std::stoi(channel_no)-1,1);
                    (control_fpga == 1)? (rmx_no1 = 1,rmx_no2 = 2) : ((control_fpga == 2)? (rmx_no1 = 3,rmx_no2 = 4) : (rmx_no1 = 5,rmx_no2 = 6));
                    json["rmx_no1"] = rmx_no1;
                    json["rmx_no2"] = rmx_no2;
                    int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((controller_of_rmx)*8)+std::stoi(channel_no);
                    json = callSetEthernetIn(ch_no,ip_addr,std::stoi(port),std::stoi(str_type));
                    usleep(1000);
                    long int mux_out =  65536; 
                    mux_out = (control_fpga == 1)? 65536 : ((control_fpga == 2)? (1048576 | 65536) : (2097152 | 65536));
                    if(write32bI2C(5,0,mux_out) == -1){
                        json["error"]= true;
                        json["message"]= "Failed MUX OUT!";
                    }
                    // json["mux_out"] =std::to_string(mux_out);
                    if(json["error"] == false){
                        db->addSPTSIPInputChannels(str_rmx_no,channel_no,std::to_string(ip_addr),port,str_type);
                    }
                    // json["tuner_ch"] = tuner_ch;
                    // json["target"] = target;
                    // json["control_fpga"] = control_fpga;
                    // json["channel_no"] = read32bI2C(6,4);
                    // json["ip"] = (read32bI2C(6,8))&0xFFFFFFFF;
                    // json["port"] = read32bI2C(6,12);
                    // json["channel"] = read32bI2C(6,16);
                    // usleep(1000);
                    // json["igmpip"] = (read32bI2C(2,28))&0xFFFFFFFF;
                    // json["igmpch"] = read32bI2C(2,32);
                    // json["input"] = read32bI2C(4,0);
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x05   function getInputChannelTunerType              */
    /*****************************************************************************/
    void getInputChannelTunerType(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"control_fpga"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getInputChannelTunerType",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string control_fpga = getParameter(request.body(),"control_fpga"); 
            if(verifyInteger(control_fpga,1,1,2)){
                Json::Value json_input_type;
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((std::stoi(control_fpga)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int type = read32bI2C(4,0);
                    if(type != -1){
                        unsigned pos = 1;
                        for (int i = 1; i <= 16; ++i)
                        {
                            json_input_type[i-1] = ((pos & type) > 0 )? 1 : 0;
                            pos = pos<<1;
                        }
                        json["input_type"] = json_input_type;
                        json["error"]= false;
                        json["message"]= "!";
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }else{
                json["error"]= true;
                json["control_fpga"]= "Require Integer between 0 to 2!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command    function setSPTSEthernetIn                      */
    /*****************************************************************************/
    void  setPCRBypass(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"input_channel_no","bitrate","control_fpga","bypass_bit"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetIn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string input_channel_no= getParameter(request.body(),"input_channel_no");
            std::string str_control_fpga = getParameter(request.body(),"control_fpga");
            std::string str_bitrate = getParameter(request.body(),"bitrate");
            std::string bypass_bit = getParameter(request.body(),"bypass_bit");
            error[0] = verifyInteger(input_channel_no,0,0,15,1);
            error[1] = verifyInteger(str_bitrate);
            error[2] = verifyInteger(str_control_fpga,1,1,3,1);
            error[3] = verifyInteger(bypass_bit);
            
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 1 to 15!" :((i == 1)?"Require Integer!":"Require Integer 1-3!");
            }
            if(all_para_valid){
                int control_fpga =std::stoi(str_control_fpga)+2;
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((control_fpga&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int input = stoi(input_channel_no)-1;
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    int tuner_ch=65536; //db->getTunerChannelType(control_fpga,rmx_no,std::stoi(channel_no)-1,1);
                    int ch = (1<<14);
                    json["input_ch"] = 1<<(input-1);
                    json["control_fpga"] = control_fpga;
                    write32bI2C(4,0,(std::stoi(bypass_bit)));
                    json["rate"] = setInputConf(input,std::stoi(str_bitrate));

                    // long int mux_out =  65536; 
                    // if(write32bI2C(5,0,mux_out) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Failed MUX OUT!";
                    // }
                    // json["mux_out"] =std::to_string(mux_out);
                    // if(json["error"] == false){
                    //     db->addIPInputChannels(rmx_no,std::stoi(channel_no),std::to_string(ip_addr),std::stoi(port),std::stoi(str_type));
                    // }
                    // json["tuner_ch"] = tuner_ch;
                    // json["target"] = target;
                    // json["control_fpga"] = control_fpga;
                    // json["channel_no"] = read32bI2C(6,4);
                    // json["ip"] = (read32bI2C(6,8))&0xFFFFFFFF;
                    // json["port"] = read32bI2C(6,12);
                    // json["channel"] = read32bI2C(6,16);
                    // usleep(1000);
                    // json["igmpip"] = (read32bI2C(2,28))&0xFFFFFFFF;
                    // json["igmpch"] = read32bI2C(2,32);
                    // json["input"] = read32bI2C(4,0);
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    int setInputConf(int input,int bitrate) {
        int RISE_NFALL = 0, MASTER_NSLAVE = 0;
        // int input_ch = 1<<(input-1); 
        // // write32bI2C(4,0,input_ch);
        // usleep(100);
        write32bI2C(4,0,input);
        int rate =int(pow(2.0,29.0) * bitrate/(FboardClk*1000000));
        int value = (RISE_NFALL<<31)|(MASTER_NSLAVE<<30)|rate;
        write32bI2C(4,4,value);
        return rate;
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command    function getEthernetIn                      */
    /*****************************************************************************/
    void  getEthernetIn(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("getEthernetIn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(channel_no,0,0,31,1);
            error[1] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 1 to 8!" :"Required Integer between 1 to 6";
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                   
                    //Multicast IP
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((controller_of_rmx)*8)+std::stoi(channel_no);
                    usleep(1000);
                    if(write32bI2C(6, 4,ch_no) == -1){
                        json["error"]= true;
                        json["message"]= "Fail to set channel_no!";
                    }
                    usleep(1000);
                    unsigned long int ip_addr = (read32bI2C(6,8)&0xFFFFFFFF);
                    if(ip_addr == -1){
                        json["error"]= true;
                        json["message"]= "Failed IP address!";
                    }else{
                        json["ip_addr"] = std::to_string(ip_addr); 
                    }
                    usleep(1000);
                    unsigned int port = read32bI2C(6,12);
                    if(port == -1){
                        json["error"]= true;
                        json["message"]= "Failed IP address!";
                    }else{
                        json["port"] =std::to_string(port); 
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     /*****************************************************************************/
    /*  UDP Ip Stack Command    function setEthernetInOff                      */
    /*****************************************************************************/
    void  setEthernetInOff(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setEthernetInOff",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(channel_no,0,0,8,1);
            error[1] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 1 to 8!" :"Required Integer between 1 to 6";
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                   
                    //Multicast IP
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((controller_of_rmx)*8)+std::stoi(channel_no);
                    json = callSetEthernetInOff(ch_no);
                    // if(write32bI2C(6, 4,ch_no) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Fail to set channel_no!";
                    // }
                    // if(write32bI2C(6,8,0) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Fail to set IP address!";
                    // }
                    // if(write32bI2C(6,16, 1) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Ethernet out failed!";
                    // }
                    // // IGMP Channel Number
                    // if(write32bI2C(2,32,ch_no) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Failed IGMP Channel Number!";
                    // }
                    // // IGMP multicast ip
                    // if(write32bI2C(2,28,0) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Failed IGMP multicast ip!";
                    // }
                    
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
                if(json["error"] == false){
                    db->removeIPInputChannels(rmx_no,std::stoi(channel_no));
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetEthernetInOff(int ch_no){
        Json::Value json;
        json["error"] = false;
        json["message"] = "IP input disabled!";
        if(write32bI2C(6, 4,ch_no) == -1){
            json["error"]= true;
            json["message"]= "Fail to set channel_no!";
        }
        if(write32bI2C(6,8,0) == -1){
            json["error"]= true;
            json["message"]= "Fail to set IP address!";
        }
        if(write32bI2C(6,16, 1) == -1){
            json["error"]= true;
            json["message"]= "Ethernet out failed!";
        }
        // IGMP Channel Number
        if(write32bI2C(2,32,ch_no) == -1){
            json["error"]= true;
            json["message"]= "Failed IGMP Channel Number!";
        }
        // IGMP multicast ip
        if(write32bI2C(2,28,0) == -1){
            json["error"]= true;
            json["message"]= "Failed IGMP multicast ip!";
        }
        return json;
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command    function setSPTSEthernetInOff                      */
    /*****************************************************************************/
    void  setSPTSEthernetInOff(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json,jsonMsg;
        Json::FastWriter fastWriter;        
        std::string para[] = {"channel_no","rmx_no"};  
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;      
        addToLog("setSPTSEthernetInOff",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
                
            std::string channel_no= getParameter(request.body(),"channel_no");
            std::string str_rmx_no = getParameter(request.body(),"rmx_no");
            error[0] = verifyInteger(channel_no,0,0,31,17);
            error[1] = verifyInteger(str_rmx_no,1,1,3,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i == 0)? "Require Integer between 17 to 31!" :"Required Integer between 1 to 3";
            }
            if(all_para_valid){
                int rmx_no = std::stoi(str_rmx_no);
               
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((rmx_no-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                   
                    //Multicast IP
                    json["error"]= false;
                    json["message"]= "successfully assigned IP OUT!";
                    // int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no =std::stoi(channel_no);
                    json = callSetEthernetInOff(ch_no);
                    json["ch_no"] = ch_no;
                    if(json["error"] == false){
                        db->removeSPTSIPInputChannels(str_rmx_no,channel_no);
                    }
                    // if(write32bI2C(6, 4,ch_no) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Fail to set channel_no!";
                    // }
                    // if(write32bI2C(6,8,0) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Fail to set IP address!";
                    // }
                    // if(write32bI2C(6,16, 1) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Ethernet out failed!";
                    // }
                    // // IGMP Channel Number
                    // if(write32bI2C(2,32,ch_no) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Failed IGMP Channel Number!";
                    // }
                    // // IGMP multicast ip
                    // if(write32bI2C(2,28,0) == -1){
                    //     json["error"]= true;
                    //     json["message"]= "Failed IGMP multicast ip!";
                    // }
                    
                }else{
                    json["error"]= true;
                    json["message"]= "Error while connection!";     
                }
                if(json["error"] == false){
                    db->removeIPInputChannels(rmx_no,std::stoi(channel_no));
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x24   function setIpdestination                      */
    /*****************************************************************************/
    void  setIpdestination(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address","rmx_no"};
        addToLog("setIpdestination",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string str_rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            if(isValidIpAddress(ip_address.c_str()) && verifyInteger(str_rmx_no,1,1,RMX_COUNT,1)){
                std::string hex_ip_part1,hex_ip_part2,hex_ip_part3,hex_ip_part4,hex_ip; 
                std::stringstream split_str(ip_address);
                unsigned int ip_part1,ip_part2,ip_part3,ip_part4; //to store the 4 ints
                unsigned char ch; //to temporarily store the '.'
                split_str >> ip_part1 >> ch >> ip_part2 >> ch >> ip_part3 >> ch >> ip_part4;
                hex_ip_part1 = getDecToHex(ip_part1);
                hex_ip_part1 = (hex_ip_part1.length() ==2)? hex_ip_part1 : "0"+hex_ip_part1;
                hex_ip_part2 = getDecToHex(ip_part2);
                hex_ip_part2 = (hex_ip_part2.length() ==2)? hex_ip_part2 : "0"+hex_ip_part2;
                hex_ip_part3 = getDecToHex(ip_part3);
                hex_ip_part3 = (hex_ip_part3.length() ==2)? hex_ip_part3 : "0"+hex_ip_part3;
                hex_ip_part4 = getDecToHex(ip_part4);
                hex_ip_part4 = (hex_ip_part4.length() ==2)? hex_ip_part4 : "0"+hex_ip_part4;
                hex_ip = hex_ip_part1 +""+hex_ip_part2+""+hex_ip_part3+""+hex_ip_part4;
                long long int ip_addr = getHexToLongDec(hex_ip);
                int rmx_no = std::stoi(str_rmx_no);
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if (write32bI2C (2,36,(ip_addr-127)) == -1) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setEthernetOut IP","Error");
                }else{
                    json["error"]= false;
                    addToLog("setEthernetOut","Success");
                    json["message"]= "successfully assigned IP OUT!";
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid IP address!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x24   function getIpdestination                    */
    /*****************************************************************************/
    void getIpdestination(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        // int target =((0&0x3)<<8) | (((0)&0x7)<<5) | (((7)&0xF)<<1) | (0&0x1);     
        // write32bCPU(0,0,target);
        // json["ch_number"] = "1";
        // uLen = c2.callCommand(49,RxBuffer,20,20,json,1);//address 4 and chennel_no[1-12]
        uLen = c2.callCommand(24,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getIpdestination","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getIpdestination!";
            addToLog("getIpdestination","Success");
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }


    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x3C (60)   function setSubnetmask                      */
    /*****************************************************************************/
    void  setSubnetmask(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address"};
        addToLog("setSubnetmask",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            if(isValidIpAddress(ip_address.c_str())){
                json["ip_address"] = ip_address;
                uLen = c2.callCommand(60,RxBuffer,20,20,json,1);                             
                if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setSubnetmask","Error");
                }else{
                    json["setSubnetmask"] = RxBuffer[6];
                    json["error"]= false;
                    json["message"]= "Set Subnet Mask!";
                    addToLog("setSubnetmask","Success");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid Subnet Mask!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }



    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x3C(60)  function getSubnetmask                   */
    /*****************************************************************************/
    void getSubnetmask(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(60,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getSubnetmask","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getSubnetmask!";
            addToLog("getSubnetmask","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x38   function setIpgateway                     */
    /*****************************************************************************/
    void  setIpgateway(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;  
        std::string para[] = {"ip_address"};
        addToLog("setIpgateway",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            if(isValidIpAddress(ip_address.c_str())){
                json["ip_address"] = ip_address;
                uLen = c2.callCommand(38,RxBuffer,20,20,json,1);
                if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setIpgateway","Error");
                }else{    
                    json["setIpgateway done"] = RxBuffer[6];
                    json["error"]= false;
                    json["message"]= "Set Ip Gateway!";
                    addToLog("setIpgateway","Success");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid IP Gateway!";    
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }



    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x38   function getIpgateway                   */
    /*****************************************************************************/
    void getIpgateway(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(38,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getIpgateway","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getIpgateway!";
            addToLog("getIpgateway","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }


     /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x40   function getDhcpIpgateway                   */
    /*****************************************************************************/
    void getDhcpIpgateway(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(40,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getDhcpIpgateway","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getDhcpIpgateway!";
            addToLog("getDhcpIpgateway","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x44   function getDhcpSubnetmask                   */
    /*****************************************************************************/
    void getDhcpSubnetmask(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(44,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getDhcpSubnetmask","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getDhcpSubnetmask!";
            addToLog("getDhcpSubnetmask","Success");
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x1C   function setIpmulticast                     */
    /*****************************************************************************/
    void  setIpmulticast(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string para[] = {"ip_address"};
        addToLog("setIpmulticast",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){        
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            if(isValidIpAddress(ip_address.c_str())){
                json["ip_address"] = ip_address;
                uLen = c2.callCommand(29,RxBuffer,20,20,json,1);
                             
                if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setIpmulticast","Error");
                }else{        
                    json["setIpmulticast"] = RxBuffer[6];
                    json["error"]= false;
                    json["message"]= "Set IP Multicast!";
                    addToLog("setIpmulticast","Success");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid IP Multicast!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }



    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x1C  function getIpmulticast                   */
    /*****************************************************************************/
    void getIpmulticast(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(29,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getIpmulticast","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getIpmulticast!";
            addToLog("getIpmulticast","Success");
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x1C  function getIpmulticast                   */
    /*****************************************************************************/
    void getFpgaIp(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(14,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getFpgaIp","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getFpgaIp!";
            addToLog("getFpgaIp","Success");
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x18   function setFpgaipDhcp                     */
    /*****************************************************************************/
    void  setFpgaipDhcp(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;      
        std::string para[] = {"ip_address"};
        addToLog("setFpgaipDhcp",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){          
            std::string ip_address = getParameter(request.body(),"ip_address"); 
            if(isValidIpAddress(ip_address.c_str())){
                json["ip_address"] = ip_address;
                uLen = c2.callCommand(18,RxBuffer,20,20,json,1);
                             
                if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
                    json["error"]= true;
                    json["message"]= "STATUS COMMAND ERROR!";
                    addToLog("setFpgaipDhcp","Error");
                }else{
                    json["setIpmulticast done"] = RxBuffer[6];
                    json["error"]= false;
                    json["message"]= "setFpgaipDhcp!";
                    addToLog("setFpgaipDhcp","Success");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid FPGA IP !";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }



    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x18  function getFpgaipDhcp                 */
    /*****************************************************************************/
    void getFpgaipDhcp(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(18,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getFpgaipDhcp","Error");
        }else{
            json["M_MSB"] = RxBuffer[11];
            json["M_LSB"] = RxBuffer[10];
            json["L_MSB"] = RxBuffer[9];
            json["L_LSB"] = RxBuffer[8];
            json["error"]= false;
            json["message"]= "getFpgaipDhcp!";
            addToLog("getFpgaipDhcp","Success");
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Command 0x0C(12) & 0x10(16)   function setFpgaMACAddress                    */
    /*****************************************************************************/
    void  setFpgaMACAddress(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0},RxBuffer1[12]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;    
        std::string para[] = {"mac_address"};
        addToLog("setFpgaMACAddress",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){             
            std::string mac_address = getParameter(request.body(),"mac_address");
            mac_address = UriDecode(mac_address); 
            if(verifyMACAddress(mac_address,17)){
                mac_address.erase(std::remove(mac_address.begin(), mac_address.end(), ':'), mac_address.end());
                std::string mac_lsb = mac_address.substr(0,8);
                std::string mac_msb = mac_address.substr(8);
                json["mac_lsb"] = mac_lsb;
                json["mac_msb"] = mac_msb;
                uLen = c2.callCommand(12,RxBuffer,20,12,json,1);                             
                if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
                    json["error"]= true;
                    json["MSB_LSB"]= "STATUS COMMAND ERROR!";
                }else{
                    json["setFpgamacLSB"] = RxBuffer[6];
                    uLen = c2.callCommand(16,RxBuffer1,20,12,json,1);
                    json["error"]= false;
                    json["MSB_LSB"]= "FPGA MAC assigned successfully!";
                    if (RxBuffer1[0] != STX1 || RxBuffer1[1] != STX2 || RxBuffer1[2] != STX3 || uLen != 12 ) {
                        json["error"]= true;
                        json["MAC_MSB"]= "STATUS COMMAND ERROR!";
                        addToLog("setFpgaMACAddress","Error");
                    }else{
                        json["error"]= false;
                        json["MAC_MSB"]= "FPGA MAC assigned successfully!";
                        addToLog("setFpgaMACAddress","Success");
                    }
                }
            }else{
                json["error"]= true;
                json["mac_address"]= "Invalid MAC address!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }



    /*****************************************************************************/
    /*  UDP Ip Stack Commande 0x0C(12) & 0x10(16)  function getFpgaMACAddress                */
    /*****************************************************************************/
    void getFpgaMACAddress(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0},RxBuffer1[20]={0};
        
        //unsigned char MAJOR,MINOR,STANDARD,CUST_OPT_ID,CUST_OPT_VER;
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        uLen = c2.callCommand(12,RxBuffer,20,20,json,0);
        if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
            json["error"]= true;
            json["message"]= "STATUS COMMAND ERROR!";
            addToLog("getFpgaMACAddress","Error");
        }else{
            std::string mac_add=hexStr(&RxBuffer[8],1)+":"+hexStr(&RxBuffer[9],1)+":"+hexStr(&RxBuffer[10],1)+":"+hexStr(&RxBuffer[11],1);
            json["error"]= false;
            json["message"]= "Get Fpga mac address!";
            uLen = c2.callCommand(16,RxBuffer1,20,20,json,0);
        
            if (RxBuffer1[0] != STX1 || RxBuffer1[1] != STX2 || RxBuffer1[2] != STX3 || uLen != 12 ) {
                json["error"]= true;
                json["message"]= "STATUS COMMAND ERROR!";
                addToLog("getFpgaMACAddress","Error");
            }else{
                json["MAC_ADD"] = mac_add+":"+hexStr(&RxBuffer1[10],1)+":"+hexStr(&RxBuffer1[11],1);
                addToLog("getFpgaMACAddress","Success");
            }
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x28   function getUDPportSource                          */
    /*****************************************************************************/
    void getUDPportSource(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;

        uLen = c2.callCommand(28,RxBuffer,1024,10,jsonMsg,0);
         if(RxBuffer[0]!=STX1 || RxBuffer[1]!=STX2 || RxBuffer[2]!=STX3)
        {
            json["error"]= true;
            json["message"]= "Invalid response header";
            addToLog("getUDPportSource","Error");
        }else{
            json["error"]= false;
            json["message"]= "get UDP port source";
            json["name"] = (RxBuffer[10] << 8 | RxBuffer[11]);   
            addToLog("getUDPportSource","Success");  
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x28   function setUDPportSource                          */
    /*****************************************************************************/
    void setUDPportSource(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string para[] = {"port"};
        addToLog("setUDPportSource",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string port = getParameter(request.body(),"port"); 
            jsonMsg["address"] = port;
            if(verifyPort(port)){
                uLen = c2.callCommand(28,RxBuffer,1024,10,jsonMsg,1);
                if(getDecToHex((int)RxBuffer[6]) != "f8"){
                    json["error"]= true;
                    json["message"]= RxBuffer[6];
                    addToLog("setUDPportSource","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Set UDP port source !";
                    addToLog("setUDPportSource","Success");
                }
            }else{
                json["error"] = true;
                json["port"] = "Invalid port number!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x2C(44)   function getUDPportDestination                          */
    /*****************************************************************************/
    void getUDPportDestination(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        uLen = c2.callCommand(44,RxBuffer,1024,10,jsonMsg,0);
        if(RxBuffer[0]!=STX1 || RxBuffer[1]!=STX2 || RxBuffer[2]!=STX3)
        {
            json["error"]= true;
            json["message"]= "Invalid response header";
            addToLog("getUDPportDestination","Error");
        }else{
            json["error"]= false;
            json["message"]= "get UDP port destination";
            json["name"] = (RxBuffer[10] << 8 | RxBuffer[11]);   
            addToLog("getUDPportDestination","Success");  
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x2C(44)   function setUDPportDestination                          */
    /*****************************************************************************/
    void setUDPportDestination(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string para[] = {"port"};
        addToLog("setUDPportDestination",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string port = getParameter(request.body(),"port");
            jsonMsg["address"] = port;
            if(verifyPort(port)){ 
                uLen = c2.callCommand(44,RxBuffer,1024,10,jsonMsg,1);
                if(getDecToHex((int)RxBuffer[6]) != "f8"){
                    json["error"]= true;
                    json["message"]= "Error:";
                    addToLog("setUDPportDestination","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Set UDP port destination !";
                    addToLog("setUDPportDestination","Success");
                }
            }else{
                json["error"]= true;
                json["port"]= "Invalid port number!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x30(48)   function getUDPChannelNumber                          */
    /*****************************************************************************/
    void getUDPChannelNumber(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        uLen = c2.callCommand(48,RxBuffer,1024,10,jsonMsg,0);
        if(RxBuffer[0]!=STX1 || RxBuffer[1]!=STX2 || RxBuffer[2]!=STX3)
        {
            json["error"]= true;
            json["message"]= "Invalid response header";
            addToLog("getUDPChannelNumber","Error");
        }else{
            json["error"]= false;
            json["message"]= "get UDP Channel Number";
            json["port"] = getDecToHex((int)RxBuffer[11]);
            addToLog("getUDPChannelNumber","Success");     
        }
       
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x30(48)   function setUDPChannelNumber                          */
    /*****************************************************************************/
    void setUDPChannelNumber(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string para[] = {"ch_number"};
        addToLog("setUDPChannelNumber",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string ch_number= getParameter(request.body(),"ch_number"); 
            if(verifyInteger(ch_number)){
                jsonMsg["ch_number"] = ch_number; 
                uLen = c2.callCommand(48,RxBuffer,1024,10,jsonMsg,1);
                   
                if(getDecToHex((int)RxBuffer[6]) != "f8"){
                    json["error"]= true;
                    json["message"]= "Error:";
                    addToLog("setUDPChannelNumber","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Set UDP channel number !";
                    addToLog("setUDPChannelNumber","Success");
                }      
            }else{
                json["error"]= true;
                json["message"]= "Invalid channle number!"; 
            }
        }else{
            json["error"]= true;
            json["message"]= res; 
        }  
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    
    /*****************************************************************************/
    /*  Commande 0x20(32)   function getIGMPChannelNumber                          */
    /*****************************************************************************/
    void getIGMPChannelNumber(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        uLen = c2.callCommand(32,RxBuffer,1024,10,jsonMsg,0);
        if(RxBuffer[0]!=STX1 || RxBuffer[1]!=STX2 || RxBuffer[2]!=STX3)
        {
            json["error"]= true;
            json["message"]= "Invalid response header";
            addToLog("getIGMPChannelNumber","Error");  
        }else{
            json["error"]= false;
            json["message"]= "get IGMP Channel Number";
            json["channel"] = getDecToHex((int)RxBuffer[11]);  
            addToLog("getIGMPChannelNumber","Success");     
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     /*****************************************************************************/
    /*  Commande 0x20(32)   function setIGMPChannelNumber                          */
    /*****************************************************************************/
    void setIGMPChannelNumber(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[1024]= {0};
        unsigned char* ss;
        char *name;
        int uLen;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;
        std::string para[] = {"ch_number"};
        addToLog("setIGMPChannelNumber",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string ch_number= getParameter(request.body(),"ch_number"); 
            jsonMsg["ch_number"] = ch_number;
            if(verifyInteger(ch_number)){
                uLen = c2.callCommand(32,RxBuffer,1024,10,jsonMsg,1);
               if(getDecToHex((int)RxBuffer[6]) != "f8")   {
                    json["error"]= true;
                    json["message"]= "Error:";
                    addToLog("setIGMPChannelNumber","Error");
                }else{
                    json["error"]= false;
                    json["message"]= "Set IGMP Channel number !";
                    addToLog("setIGMPChannelNumber","Success");
                }
            }else{
                json["error"]= true;
                json["ch_number"]= "Require integer!"; 
            }
        }else{
            json["error"]= true;
            json["message"]= res; 
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  UDP Ip Stack Commande (0x34)52   function getUdpchannels                          */
    /*****************************************************************************/
    void getUdpchannels(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[10]={0};
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        // uLen = c2.callCommand(52,RxBuffer,20,20,json,0);
        // if (RxBuffer[0] != STX1 || RxBuffer[1] != STX2 || RxBuffer[2] != STX3 || uLen != 12 ) {
        //     json["error"]= true;
        //     json["message"]= "STATUS COMMAND ERROR!";
        //     addToLog("getUdpchannels","Error");
        // }else{
        //     json["total_number_of_udp_channels"] = RxBuffer[11];
        //     json["error"]= false;
        //     json["message"]= "Total UDP Channels!";
        //     addToLog("getUdpchannels","Success");
        // }
        write32bCPU(0,0,0);
        json["message"]= read32bI2C(2,52);
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    

//DVBC CORE
    /*****************************************************************************/
    /*  Commande 0x05   function setQAM                        
    0x0 : 16-QAM    0x1 : 32-QAM    0x2 : 64-QAM    0x3 : 128-QAM    0x4 : 256-QAM  */
    /*****************************************************************************/
    void  setQAM(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"qam_no","rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setQAM",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string qam_no = getParameter(request.body(),"qam_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(qam_no);
            error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==1)? "Require Integer between 1 to 6!" :(i==2)?"Require Integer between 1 to 8!" : "Require Integer between 1 to 128!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    int value = ((std::stoi(qam_no)&0xF)<<1) | (0&0x0);
                    json = callSetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(QAM_ADDR),std::to_string(value),std::stoi(rmx_no));
                    if(json["error"] == false){
                    	db->addQAM(rmx_no,output,qam_no);
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getCore             
    0x0 : 16-QAM    0x1 : 32-QAM    0x2 : 64-QAM    0x3 : 128-QAM    0x4 : 256-QAM */
    /*****************************************************************************/
    void getQAM(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getQAM",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(QAM_ADDR));
                    if(getcorejson["error"]==false){
                        json["qam"] =std::to_string(((std::stoul(getcorejson["data"].asString())&0xF)>>1));
                        json["error"] = false;
                        json["message"] ="get QAM";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"] ="Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Commande 0x05   function getDVBCCoreVersion(getCore)             
     */
    /*****************************************************************************/
    void getDVBCCoreVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDVBCCoreVersion",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBC_VER_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["core_id"] = std::to_string(((data >> 8)&0xFF));
                        json["minor_ver"] = std::to_string(((data >> 16)&0xFF));
                        json["major_ver"] = std::to_string(((data >> 24)&0xFF));
                        json["error"]= false;
                        json["message"] ="DVBC core version!";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"] ="Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getSPIStatusReg(getCore)             
     */
    /*****************************************************************************/
    void getSPIStatusReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getSPIStatusReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(SPI_STATUS_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["frame_status"] =std::to_string((data >> 29)&0x7);
                        json["stream"] = std::to_string((data >> 28)&0x1);
                        json["bitstream"] = std::to_string((data)&0xFFFFFFF);
                        json["error"]= false;
                        json["message"] ="SPI status register!";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"] ="Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setSPIConfigReg(getCore)             
     */
    /*****************************************************************************/
    void setSPIConfigReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output","fpga_clock","spi_edge_input","bypass_pcr","null_packets","lost_frames"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setSPIConfigReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            std::string fpga_clock = getParameter(request.body(),"fpga_clock"); 
            std::string spi_edge_input = getParameter(request.body(),"spi_edge_input"); 
            std::string bypass_pcr = getParameter(request.body(),"bypass_pcr"); 
            std::string null_packets = getParameter(request.body(),"null_packets"); 
            std::string lost_frames = getParameter(request.body(),"lost_frames"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(fpga_clock);
            error[3] = verifyInteger(spi_edge_input,1,1,1);
            error[4] = verifyInteger(bypass_pcr,1,1,1);
            error[5] = verifyInteger(null_packets,1,1,1);
            error[6] = verifyInteger(lost_frames,1,1,1);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" :((i==1)? "Require Integer between 1 to 8!" : "Require Integer 0 or 1 !");
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    long unsigned int value =  (std::stoul(lost_frames)&0x1)<<31 | (std::stoul(null_packets)&0x1)<<21 | (std::stoul(bypass_pcr)&0x1)<<20 | (std::stoul(spi_edge_input)&0x1)<<19 | (std::stoul(fpga_clock)&0x7FFFF);
                     json =callSetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(SPI_CONF_ADDR),std::to_string(value),std::stoi(rmx_no));
                    if(json["error"]==false){
                        json["message"] ="SPI configuration register!";
                    }
                }else{
                    json["error"]= true;
                    json["message"] ="Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getSPIConfigReg(getCore)             
     */
    /*****************************************************************************/
    void getSPIConfigReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getSPIConfigReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){   
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBC_OUTPUT_CS[std::stoi(output)]),std::to_string(SPI_CONF_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["lost_frames"] =std::to_string((data >> 31)&0x1);
                        json["null_packets"] = std::to_string((data >> 21)&0x1);
                        json["bypass_pcr"] = std::to_string((data >> 20)&0x1);
                        json["spi_edge_input"] = std::to_string((data >> 19)&0x1);
                        json["fpga_clock"] = std::to_string((data)&0x7FFFF);
                        json["error"]= false;
                        json["message"] ="SPI configuration register!";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"] ="Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }


//UPSAMPLER COMMANDS
    /*****************************************************************************/
    /*  Commande 0x05   function getUpsamplerCoreVersion(getCore)              */
    /*****************************************************************************/
    void getUpsamplerCoreVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getGain",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(UPSAMPLER_VER_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["core_id"] = std::to_string(((data >> 8)&0xFF));
                        json["minor_ver"] = std::to_string(((data >> 16)&0xFF));
                        json["major_ver"] = std::to_string(((data >> 24)&0xFF));
                        json["error"]= false;
                        json["message"] ="get Upsampler core version!";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setStatusReg                        
    */
    /*****************************************************************************/
    void  setStatusReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"gain_satu","if_satu","rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setFDAC",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string gain_satu = getParameter(request.body(),"gain_satu"); 
            std::string if_satu = getParameter(request.body(),"if_satu"); 
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(gain_satu,1,1,1);
            error[1] = verifyInteger(if_satu,1,1,1);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[3] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==2)? "Require Integer between 1 to 6!" :(i==3)?"Require Integer between 1 to 8!" : "Require Integer 0 or 1!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) 
                {
                    int value = ((std::stoi(if_satu)&0x1)<<8) | ((0&0x7F)<<1) | (std::stoi(gain_satu)&0x1);
                    json = callSetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(STATUS_REG_ADDR),std::to_string(value),std::stoi(rmx_no));
                    if(json["error"]==false){
                        json["message"] ="Status register configuration!";
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getStatusReg(getCore)              */
    /*****************************************************************************/
    void getStatusReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getStatusReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(STATUS_REG_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["gain_satu"] = std::to_string((data&0x1));
                        json["if_satu"] = std::to_string(((data >> 8)&0x1));
                        json["message"] ="Upsampler core status register!";
                        json["error"]= false;
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setFSymbolRate                        
    */
    /*****************************************************************************/
    void  setFSymbolRate(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"fsymbol_rate","rolloff","rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setFSymbolRate",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string fsymbol_rate = getParameter(request.body(),"fsymbol_rate"); 
            std::string rolloff = getParameter(request.body(),"rolloff"); 
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(fsymbol_rate);
            error[1] = verifyInteger(rolloff,1,1,3);
            error[2] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[3] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==2)? "Require Integer between 1 to 6!" :(i==3)?"Require Integer between 1 to 8!" : "Require Integer!";
            }
            if(all_para_valid){
            	json = callSetFSymbolRate(rmx_no,output,rolloff,fsymbol_rate);
                // int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                // if(write32bCPU(0,0,target) != -1) {
                //     int value = (0&0x3F)<<26 | ((std::stoi(rolloff)&0x3)<<24) | (std::stoi(fsymbol_rate)&0xFFFFFF);
                //     json = callSetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(FSYMBOLRATE_ADDR),std::to_string(value),std::stoi(rmx_no));
                //     if(json["error"]==false){
                //         json["message"] ="FSymbol rate configuration!";
                //         db->addSymbolRate(fsymbol_rate,rmx_no,output,rolloff);
                //     }
                // }else{
                //     json["error"]= true;
                //     json["message"]= "Connection error!";
                // }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callSetFSymbolRate(std::string rmx_no,std::string output,std::string rolloff,std::string fsymbol_rate){
    	Json::Value json;
    	int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
        if(write32bCPU(0,0,target) != -1) {
            int value = (0&0x3F)<<26 | ((std::stoi(rolloff)&0x3)<<24) | (std::stoi(fsymbol_rate)&0xFFFFFF);
            json = callSetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(FSYMBOLRATE_ADDR),std::to_string(value),std::stoi(rmx_no));
            if(json["error"]==false){
                json["message"] ="FSymbol rate configuration!";
                db->addSymbolRate(fsymbol_rate,rmx_no,output,rolloff);
            }
        }else{
            json["error"]= true;
            json["message"]= "Connection error!";
        }
        return json;
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getFSymbolRate(getCore)              */
    /*****************************************************************************/
    void getFSymbolRate(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getFSymbolRate",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    Json::Value getcorejson = callGetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(FSYMBOLRATE_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["fsymbol_rate"] = std::to_string((data&0xFFFFFF));
                        json["rolloff"] = std::to_string((data >> 24)&0x3);
                        json["error"] = false;
                        json["message"] ="FSymbol rate!";
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getFDAC(getCore)              */
    /*****************************************************************************/
    void getFDAC(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getFDAC",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(FDAC_ADDR));
                    if(getcorejson["error"]==false){
                        json["fdac"] = getcorejson["data"].asString();
                        json["message"] ="get FDAC!";
                        json["error"]= false;
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setFDAC                        
    */
    /*****************************************************************************/
    void  setFDAC(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"fdac","rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setFDAC",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string fdac = getParameter(request.body(),"fdac"); 
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(fdac);
            error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==1)? "Require Integer between 1 to 6!" :(i==2)?"Require Integer between 1 to 8!" : "Require Integer!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                    int value = std::stoi(fdac);
                    json = callSetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(FDAC_ADDR),std::to_string(value),std::stoi(rmx_no));
                    if(json["error"]==false){
                        json["message"] ="FDAC configuration!";
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function confGain                        
    */
    /*****************************************************************************/
    void  confGain(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;     

        std::string para[] = {"gain","invert_IQ","sin","mute","rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("confGain",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string gain = getParameter(request.body(),"gain");
            std::string invert_IQ = getParameter(request.body(),"invert_IQ");
            std::string sin = getParameter(request.body(),"sin");
            std::string mute = getParameter(request.body(),"mute"); 
            std::string rmx_no =getParameter(request.body(),"rmx_no") ;
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(gain);
            error[1] = verifyInteger(invert_IQ,1,1,1,0);
            error[2] = verifyInteger(sin,1,1,1,0);
            error[3] = verifyInteger(mute,1,1,1,0);
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[5] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==4)? "Require Integer between 1 to 6!" :((i==5)?"Require Integer between 1 to 8!" : ((i==0)? "Invalid GAIN (should be in 0xFF ,0x80 or 0x40)":"Require Integer between 0 and 1!"));
            }
            if(all_para_valid){
                if(std::stoi(gain)==64 || std::stoi(gain)==128 || std::stoi(gain)==255){
                    int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                    if(write32bCPU(0,0,target) != -1) {
                        int value = ((std::stoi(mute)&0x1)<<10) | ((std::stoi(sin)&0x1)<<9) | ((std::stoi(invert_IQ)&0x1)<<8) | (std::stoi(gain)&0xFF);
                        json = callSetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(GAIN_ADDR),std::to_string(value),std::stoi(rmx_no));
                        if(json["error"]==false){
                            json["message"] ="GAIN configuration!";
                        }
                    }else{
                        json["error"]= true;
                        json["message"]= "Connection error!";
                    }
                }else{
                     json["error"]= true;
                     json["message"]= "Invalid GAIN (should be in 0xFF ,0x80 or 0x40)";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getGain(getCore)              */
    /*****************************************************************************/
    void getGain(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getGain",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(UPSAMPLER_OUTPUT_CS[std::stoi(output)]),std::to_string(GAIN_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["gain"] = std::to_string((data&0xFF));
                        json["invert_IQ"] = std::to_string(((data >> 8)&0x1));
                        json["sin"] = std::to_string(((data >> 9)&0x1));
                        json["mute"] = std::to_string(((data >> 10)&0x1));
                        json["message"] ="get GAIN!";
                        json["error"]= false;
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  I2C controller Command function setCenterFrequency                         */
    /*****************************************************************************/
    void  setCenterFrequency(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;        
        std::string para[] = {"center_frequency","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setCenterFrequency",request.body());
        unsigned int uiFrequencies[8];
        unsigned int* puiFrequencies;
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string center_frequency = getParameter(request.body(),"center_frequency"); 
            std::string str_rmx_no = getParameter(request.body(),"rmx_no"); 
            error[0] = verifyInteger(str_rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(center_frequency);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between!";
            }
            if(all_para_valid){
                json = callsetCenterFrequency(center_frequency,str_rmx_no);
                if(json["error"] == false){
                	unsigned int firstFreq = std::stoi(center_frequency)-28;
                	for (int i = 0; i < 8; ++i)
                	{
                		uiFrequencies[i] = firstFreq + (i*8);
                	}
                	puiFrequencies = uiFrequencies; 
                	db->addFrequency(center_frequency,str_rmx_no,puiFrequencies);
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res; 
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callsetCenterFrequency(std::string center_frequency,std::string str_rmx_no){

        unsigned char RxBuffer[20]={0};
        int uLen;
        Json::Value json;
        int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((6&0xF)<<1) | (0&0x1);
        if(write32bCPU(0,0,target) != -1) {
            // double value = read32bI2C(8,0);
            //4294967296 (2^32)
            

            int frequency = std::stoi(center_frequency);
            int rmx_no = std::stoi(str_rmx_no) -1;
            int reg_addr = rmx_no * 4;//(4 is no of outputs)
            long long int reg_value = round(4294967296 * frequency/FDAC_VALUE);
            json["frequency"]= reg_value;
            if(write32bI2C(8,reg_addr,reg_value) != -1){
                json["error"]= false;
                json["message"]= "Center frequency has changaed successfully!";
                addToLog("setCenterFrequency","Success");
            }else{
                json["error"]= true;
                json["message"]= "Error while writing register!";
                addToLog("setCenterFrequency","Error");
            }
        }else{
                json["error"]= true;
                json["message"]= "Connection error!";
        }
        return json;
    }
    /*****************************************************************************/
    /*  I2C controller Commande function getCenterFrequency               */
    /*****************************************************************************/
    void getCenterFrequency(const Rest::Request& request, Net::Http::ResponseWriter response){
        unsigned char RxBuffer[20]={0};
        
        int uLen;
        Json::Value json;
        Json::FastWriter fastWriter;   
        std::string str_rmx_no; 
        str_rmx_no = request.param(":rmx_no").as<std::string>();
        double value;
        if(verifyInteger(str_rmx_no,1,1,RMX_COUNT,1)){
            int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((6&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
                int rmx_no = std::stoi(str_rmx_no) -1;
                int reg_addr = rmx_no * 4;//(4 is no of outputs)
                int center_frequency =197;
                while(center_frequency == 197){
                    usleep(1000);
                    value = read32bI2C(8,reg_addr);
                    center_frequency = round((value*FDAC_VALUE)/4294967296);  
                }
                json["center_frequency"] =center_frequency;
                json["value"] = value;
            }else{
                json["error"]= true;
                json["message"]= "Connection error!";
            }
        }else{
            json["error"]= true;
            json["message"]= "Connection error!";
        }
       
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

//DVBCSA Algorithms
    /*****************************************************************************/
    /*  Commande 0x05   function getDVBCSAVersion(getCore)              */
    /*****************************************************************************/
    void getDVBCSAVersion(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDVBCSAVersion",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_VER_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["minor_ver"] = std::to_string(((data >> 16)&0xFF));
                        json["major_ver"] = std::to_string(((data >> 24)&0xFF));
                        json["message"] ="DVB-CSA version!";
                        json["error"] = false;
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"] = true;
                    json["message"] = "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getDVBCSACoreControlReg(getCore)              */
    /*****************************************************************************/
    void getDVBCSACoreControlReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDVBCSACoreControlReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value getcorejson = callGetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_CORE_CONTR_ADDR));
                    if(getcorejson["error"]==false){
                        long unsigned int data = std::stoul(getcorejson["data"].asString());
                        json["enable_encryption"] = std::to_string((data&0x1));
                        json["parity"] = std::to_string(((data&0x2)>>1));
                        json["key_id"] = std::to_string(((data&0x7F)>>2));
                        json["pid_id"] = std::to_string(((data&0x3F00)>>8));
                        json["pid_value"] = std::to_string(((data&0x1FFF0000)>>16));
                        json["write_enable"] = std::to_string(((data&0x40000000)>>30));
                        json["message"] ="DVB-CSA control register!";
                        json["error"] = false;
                    }else{
                        json = getcorejson;
                    }
                }else{
                    json["error"] = true;
                    json["message"] = "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setDVBCSACoreControlReg(getCore)              */
    /*****************************************************************************/
    void setDVBCSACoreControlReg(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output","enable_encryption","parity","key_id","pid_id","pid_value","write_enable"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setDVBCSACoreControlReg",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            std::string enable_encryption = getParameter(request.body(),"enable_encryption"); 
            std::string parity = getParameter(request.body(),"parity"); 
            std::string key_id = getParameter(request.body(),"key_id"); 
            std::string pid_id = getParameter(request.body(),"pid_id"); 
            std::string pid_value = getParameter(request.body(),"pid_value"); 
            std::string write_enable = getParameter(request.body(),"write_enable"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(enable_encryption,1,1,1);
            error[3] = verifyInteger(parity,1,1,1);
            error[4] = verifyInteger(write_enable,1,1,1);
            error[5] = verifyInteger(key_id,0,0,31);
            error[6] = verifyInteger(pid_id,0,0,63);
            error[7] = verifyInteger(pid_value);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" :((i == 1)? "Require Integer between 1 to 8 !":((i==5)? "Require integer between 0 to 31!": ((i==6)? "Require integer between 0 to 63!" : ((i==7)? "Require integer!" : "Require Integer 0 or 1!"))));
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int value =((std::stoi(write_enable)&0x1)<<30) | ((std::stoi(pid_value)&0x1FFF)<<16) | ((std::stoi(pid_id)&0x3F)<<8) | ((std::stoi(key_id)&0x1F)<<2) | ((std::stoi(parity)&0x1)<<1) | (std::stoi(enable_encryption)&0x1);
                    json = callSetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_CORE_CONTR_ADDR),std::to_string(value),std::stoi(rmx_no));
                    if(json["error"]==false){
                        json["message"] ="DVB-CSA control register!";
                    }
                }else{
                    json["error"] = true;
                    json["message"] = "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function getDVBCSAKey(getCore)              */
    /*****************************************************************************/
    void getDVBCSAKey(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("getDVBCSAKey",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" : "Require Integer between 1 to 8!";
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    Json::Value lsbjson = callGetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_LSB_KEY_ADDR));
                    Json::Value msbjson = callGetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_MSB_KEY_ADDR));
                    Json::Value keyjson = callGetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_KEY_CONTR_ADDR));
                    if(lsbjson["error"]==false && msbjson["error"]==false && keyjson["error"]==false){
                        json["key"] =getlongDecToHex(std::stoul(msbjson["data"].asString())) +""+ getlongDecToHex(std::stoul(lsbjson["data"].asString()));
                        // json["key_msb"] =getlongDecToHex(std::stoul(msbjson["data"].asString())); 
                        json["key_id"] = std::to_string(std::stoul(keyjson["data"].asString())&0x1F);
                        json["message"] ="DVB-CSA get key!";
                        json["error"] = false;
                    }else{
                        json["error"] = true;
                        json["lsb"] = lsbjson["message"];
                        json["msb"] = msbjson["message"];
                        json["key"] = keyjson["message"];

                    }
                }else{
                    json["error"] = true;
                    json["message"] = "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Commande 0x05   function setDVBCSAKey(setCore)              */
    /*****************************************************************************/
    void setDVBCSAKey(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json,jsonInput;
        Json::FastWriter fastWriter;
        std::string para[] = {"rmx_no","output","key","key_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("setDVBCSAKey",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string rmx_no = getParameter(request.body(),"rmx_no"); 
            std::string output = getParameter(request.body(),"output"); 
            std::string key = getParameter(request.body(),"key"); 
            std::string key_id = getParameter(request.body(),"key_id"); 
            error[0] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);
            error[1] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[2] = verifyInteger(key_id,0,0,31);
            error[3] = verifyIsHex(key,18);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= (i==0)? "Require Integer between 1 to 6!" :((i == 1)? "Require Integer between 1 to 8 !":((i==2)? "Require integer between 0 to 31!": "Require hex string len 16!"));
            }
            if(all_para_valid){
                int target =((0&0x3)<<8) | (((std::stoi(rmx_no)-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    std::string msb_key = std::to_string(getHexToLongDec(key.substr(2,8)));
                    std::string lsb_key = std::to_string(getHexToLongDec(key.substr(10))); 
                    json = callSetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_LSB_KEY_ADDR),lsb_key,std::stoi(rmx_no));
                    if(json["error"]==false){
                        json = callSetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_MSB_KEY_ADDR),msb_key,std::stoi(rmx_no));
                        if(json["error"]==false){
                            json = callSetCore(std::to_string(DVBCSA_OUTPUT_CS[std::stoi(output)]),std::to_string(DVBCSA_KEY_CONTR_ADDR),key_id,std::stoi(rmx_no));
                            if(json["error"]==false){
                                json["message"] = "DVB-CSA encrypt key!";
                            }
                        }
                    }
                }else{
                    json["error"] = true;
                    json["message"] = "Connection error!";
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     /*****************************************************************************/
    /*  function upconverterRW                                                     */
    /*****************************************************************************/
    void  upconverterRW(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;  
        int mainInterface =std::stoi(getParameter(request.body(),"mainInterface"));
        int value =std::stoi(getParameter(request.body(),"value"));
        int cs =std::stoi(getParameter(request.body(),"cs"));
        int address =std::stoi(getParameter(request.body(),"address"));
        write32bCPU(0,0,(mainInterface<<1));
        // write32bI2C(cs, address ,value);
        json["VLAUE"] = read32bI2C(cs,address);
        std::cout<<value<<"\n";
        json["message"] = "upconverterRW!";
        json["mainInterface<<1"] = mainInterface<<1;
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
//CAS commands

    /*****************************************************************************/
    /*  Command    function addECMChannelSetup                          */
    /*****************************************************************************/
    void addECMChannelSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        int channel_id,ecm_id,ecm_port;
        std::string supercas_id,ecm_ip,str_channel_id,str_ecm_port;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","supercas_id","ip","port"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("addECMChannelSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            supercas_id = getParameter(request.body(),"supercas_id"); 
            ecm_ip = getParameter(request.body(),"ip");
            str_ecm_port = getParameter(request.body(),"port");

            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyIsHex(supercas_id);
            error[2] = isValidIpAddress(ecm_ip.c_str());
            error[3] = verifyPort(str_ecm_port);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id = std::stoi(str_channel_id);
                ecm_port = std::stoi(str_ecm_port);
                if(db->addECMChannelSetup(channel_id,supercas_id,ecm_ip,ecm_port)){
                    ecmChannelSetup(channel_id,supercas_id,ecm_port,ecm_ip,channel_id);
                    json["error"]= false;
                    json["message"]= "ECM added!";
                    addToLog("addECMChannelSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Mysql error while sdding ECM channel!";
                    addToLog("addECMChannelSetup","Mysql error while adding ECM channel");
                }
            }
        }else{
             json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void ecmChannelSetup(int channel_id,std::string supercas_id,int ecm_port,std::string ecm_ip,int old_channel_id,int isAdd=1){
        pthread_t old_thid;
        std::string currtime=getCurrentTime();
        reply = (redisReply *)redisCommand(context,"SELECT 5");
        // std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
        if(isAdd==1){
            this->channel_ids = channel_id;
            this->supercas_id = supercas_id;
            this->ecm_port = ecm_port;
            this->ecm_ip = ecm_ip;
            int err = pthread_create(&thid, 0,&StatsEndpoint::spawnChannel, this);
            if (err != 0){
                printf("\ncan't create thread :[%s]", strerror(err));
            }else{  
                printf("\n Thread created successfully\n");
                reply = (redisReply *)redisCommand(context,("SET channel_counter:ch_"+std::to_string(channel_id)+" 1").c_str());
                reply = (redisReply *)redisCommand(context,("SET stream_counter:ch_"+std::to_string(channel_id)+" 1").c_str());
            }  
        }else{
            if(channel_id != old_channel_id){
                reply = (redisReply *)redisCommand(context,("RENAME Channel_list:"+std::to_string(old_channel_id)+" Channel_list:"+std::to_string(channel_id)).c_str());
                reply = (redisReply *)redisCommand(context,("RENAME channel_counter:ch_"+std::to_string(old_channel_id)+" channel_counter:ch_"+std::to_string(channel_id)).c_str());
               
                reply = (redisReply *)redisCommand(context,("KEYS stream:ch_"+std::to_string(old_channel_id)+"*").c_str());
                int i=0;
                for (i = 0; i <reply->elements; ++i)
                {
                    std::string old_key = reply->element[i]->str;
                    std::string key = old_key;
                    std::size_t ch_pos = key.find("_");
                    if (ch_pos!=std::string::npos){
                        std::size_t ch_id = key.find(":",ch_pos);
                        if (ch_id!=std::string::npos){
                            int ch_p=static_cast<unsigned int>( ch_pos );
                            int ch_i=static_cast<unsigned int>( ch_id );
                            key.replace(ch_pos+1,ch_i - ch_p  - 1,std::to_string(channel_id));
                            stream_reply = (redisReply *)redisCommand(context,("RENAME "+old_key+" "+key).c_str());
                            std::cout<<"Replaced---------->>"<<old_key<<" TO "<<key<<std::endl;
                        }
                    }
                }
            }
            printf("\n Thread already exists\n");
            reply = (redisReply *)redisCommand(context,("INCR channel_counter:ch_"+std::to_string(channel_id)).c_str());
        }
        std::string query ="HMSET Channel_list:"+std::to_string(channel_id)+" channel_id "+std::to_string(channel_id)+" supercas_id "+supercas_id+" ip "+ecm_ip+" port "+std::to_string(ecm_port)+" is_deleted 0";
        reply = (redisReply *)redisCommand(context,query.c_str());
        reply = (redisReply *)redisCommand(context,("DEL deleted_ecm:ch_"+std::to_string(channel_id)).c_str());
        std::cout<<"Is deleted -->"<<reply->str<<std::endl;
        freeReplyObject(reply);
    }
    /*****************************************************************************/
    /*  Command    function updateECMChannelSetup                          */
    /*****************************************************************************/
    void updateECMChannelSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        int channel_id,ecm_id,ecm_port,old_channel_id;
        std::string supercas_id,ecm_ip,str_channel_id,str_ecm_port,str_old_channel_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","supercas_id","ip","port","old_channel_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("updateECMChannelSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_old_channel_id =getParameter(request.body(),"old_channel_id");
            supercas_id = getParameter(request.body(),"supercas_id"); 
            ecm_ip = getParameter(request.body(),"ip");
            str_ecm_port = getParameter(request.body(),"port");

            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyIsHex(supercas_id);
            error[2] = isValidIpAddress(ecm_ip.c_str());
            error[3] = verifyPort(str_ecm_port);
            error[4] = verifyInteger(str_old_channel_id);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id = std::stoi(str_channel_id);
                old_channel_id = std::stoi(str_old_channel_id);
                ecm_port = std::stoi(str_ecm_port);
               if(db->updateECMChannelSetup(channel_id,supercas_id,ecm_ip,ecm_port,old_channel_id) > 0){
                    ecmChannelSetup(channel_id,supercas_id,ecm_port,ecm_ip,old_channel_id,0);
                    json["error"]= false;
                    json["message"]= "ECM Updated!";
                    addToLog("updateECMChannelSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Mysql error while updating ECM channel!";
                    addToLog("updateECMChannelSetup","Mysql error while updating ECM channel");
                }
            }
        }else{
             json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Command    function deleteECMChannelSetup                          */
    /*****************************************************************************/
    void deleteECMChannelSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        int channel_id;
        std::string str_channel_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("deleteECMChannelSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            
            if(verifyInteger(str_channel_id)){
                channel_id = std::stoi(str_channel_id);
                if(db->isECMExists(channel_id) && deleteECMChannel(channel_id)){
                    json["error"]= false;
                    json["message"]= "ECM deleted!";
                    addToLog("deleteECMChannelSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "ECM does't exists or already deleted!";
                    addToLog("deleteECMChannelSetup","Error");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid channel id!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    int deleteECMChannel(int channel_id){
        int is_deleted=0;
        reply = (redisReply *)redisCommand(context,"SELECT 5");
        reply = (redisReply *)redisCommand(context,("HGETALL Channel_list:"+std::to_string(channel_id)).c_str());
        if(reply->elements>0){
            std::string query ="HMSET Channel_list:"+std::to_string(channel_id)+" is_deleted 1";
            reply = (redisReply *)redisCommand(context,query.c_str());
            if(db->deleteECM(channel_id)){
                is_deleted=1;
                std::cout<<"ECM DELETED"<<std::endl;
                streams_json=db->getScrambledServices();
                reply = (redisReply *)redisCommand(context,("INCR channel_counter:ch_"+std::to_string(channel_id)).c_str());
                reply = (redisReply *)redisCommand(context,("SET deleted_ecm:ch_"+std::to_string(channel_id)+" 1").c_str());
            }
        }
        freeReplyObject(reply);
        return is_deleted;
    }

    /*****************************************************************************/
    /*  Command    function /addECMStreamSetup                          */
    /*****************************************************************************/
    void addECMStreamSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        int channel_id,stream_id,access_criteria,cp_number,ecm_id;
        std::string str_channel_id,str_stream_id,str_access_criteria,str_cp_number,str_ecm_id,str_ecm_pid;
        std::string timestamp;
        Json::Value json;
        Json::FastWriter fastWriter;
        std::string para[] = {"channel_id","stream_id","access_criteria","cp_number","ecm_id","ecm_pid"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("addECMStreamSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_stream_id = getParameter(request.body(),"stream_id"); 
            str_access_criteria = getParameter(request.body(),"access_criteria");
            str_cp_number = getParameter(request.body(),"cp_number");
            str_ecm_id = getParameter(request.body(),"ecm_id"); 
            str_ecm_pid = getParameter(request.body(),"ecm_pid"); 
            // timestamp = getParameter(request.body(),"timestamp");
            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyInteger(str_stream_id);
            error[2] = verifyInteger(str_access_criteria,0,0,65535,1);
            error[3] = verifyInteger(str_cp_number);
            error[4] = verifyInteger(str_ecm_id);
            error[5] = verifyInteger(str_ecm_pid);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id =std::stoi(getParameter(request.body(),"channel_id"));
                stream_id = std::stoi(getParameter(request.body(),"stream_id")); 
                //access_criteria = std::stoi(getParameter(request.body(),"access_criteria"));
                cp_number = std::stoi(getParameter(request.body(),"cp_number"));
                ecm_id = std::stoi(getParameter(request.body(),"ecm_id")); 
                if(db->isECMExists(channel_id)){
                    std::string currtime=getCurrentTime();
                    if(db->addECMStreamSetup(stream_id,ecm_id,channel_id,str_access_criteria,cp_number,str_ecm_pid,currtime)){
                        ecmStreamSetup(channel_id,stream_id,ecm_id,str_access_criteria,cp_number,currtime,str_ecm_pid);
                        int err;
                        // if(CW_THREAD_CREATED==0){
                            
                        //     err = pthread_create(&tid, 0,&StatsEndpoint::cwProvision, this);
                        //     if (err != 0){
                        //         printf("\ncan't create thread :[%s]", strerror(err));
                        //     }else{
                        //         CW_THREAD_CREATED=1;
                        //         printf("\n Thread created successfully\n");
                        //     }
                        // }
                        json["error"]= false;
                        json["message"]= "Stream Added!";
                        addToLog("addECMStreamSetup","Success");
                    }else{
                        json["error"]= false;
                        json["message"]= "Mysql error while updating ECM channel!";
                        addToLog("addECMStreamSetup","Success");
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "channel_id does't exists or disabled!";
                    addToLog("addECMStreamSetup","Error");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void ecmStreamSetup(int channel_id,int stream_id,int ecm_id, std::string access_criteria,int cp_number,std::string currtime,std::string ecm_pid){
        redisReply *reply1;
        reply = (redisReply *)redisCommand(context,"SELECT 5");
        reply = (redisReply *)redisCommand(context,("KEYS stream:ch_"+std::to_string(channel_id)+":stm_"+std::to_string(stream_id)+"*").c_str());
        if(reply->elements>0){
            for(unsigned int i=0; i<reply->elements;i++){
                reply1 = (redisReply *)redisCommand(context,("DEL "+(std::string)reply->element[i]->str).c_str());
            }
            freeReplyObject(reply1);
        }
        std::string query="HMSET stream:ch_"+std::to_string(channel_id)+":stm_"+std::to_string(stream_id)+" stream_id "+std::to_string(stream_id)+" ecm_id "+std::to_string(ecm_id)+" access_criteria "+access_criteria+" cp_number "+std::to_string(cp_number)+" ecm_pid "+ecm_pid+" read_flag 0";        
        reply = (redisReply *)redisCommand(context,query.c_str());
        reply = (redisReply *)redisCommand(context,("INCR stream_counter:ch_"+std::to_string(channel_id)+"").c_str());
        //streams_json=db->getStreams();
        freeReplyObject(reply);
    }

    /*****************************************************************************/
    /*  Command    function /addECMStreamSetup                          */
    /*****************************************************************************/
    void updateECMStreamSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        
        int channel_id,stream_id,access_criteria,cp_number,ecm_id;
        std::string str_channel_id,str_stream_id,str_access_criteria,str_cp_number,str_ecm_id,str_ecm_pid;
        std::string timestamp;
        Json::Value json;
        Json::FastWriter fastWriter;
        std::string para[] = {"channel_id","stream_id","access_criteria","cp_number","ecm_id","ecm_pid"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("updateECMStreamSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_stream_id = getParameter(request.body(),"stream_id"); 
            str_access_criteria = getParameter(request.body(),"access_criteria");
            str_cp_number = getParameter(request.body(),"cp_number");
            str_ecm_id = getParameter(request.body(),"ecm_id"); 
            str_ecm_pid = getParameter(request.body(),"ecm_pid"); 

            // timestamp = getParameter(request.body(),"timestamp");
            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyInteger(str_stream_id);
            error[2] = verifyInteger(str_access_criteria,0,0,65535,1);
            error[3] = verifyInteger(str_cp_number);
            error[4] = verifyInteger(str_ecm_id);
            error[5] = verifyInteger(str_ecm_pid);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id =std::stoi(getParameter(request.body(),"channel_id"));
                stream_id = std::stoi(getParameter(request.body(),"stream_id")); 
                //access_criteria = std::stoi(getParameter(request.body(),"access_criteria"));
                cp_number = std::stoi(getParameter(request.body(),"cp_number"));
                ecm_id = std::stoi(getParameter(request.body(),"ecm_id")); 
                if(db->isECMExists(channel_id)){
                    std::string currtime=getCurrentTime();
                    if(db->updateECMStreamSetup(stream_id,ecm_id,channel_id,str_access_criteria,cp_number,str_ecm_pid,currtime)){
                        ecmStreamSetup(channel_id,stream_id,ecm_id,str_access_criteria,cp_number,currtime,str_ecm_pid);
                        int err;
                        json["error"]= false;
                        json["message"]= "Stream Added!";
                        addToLog("updateECMStreamSetup","Success");
                    }else{
                        json["error"]= false;
                        json["message"]= "Mysql error while updating ECM channel!";
                        addToLog("updateECMStreamSetup","Success");
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "channel_id does't exists or disabled!";
                    addToLog("updateECMStreamSetup","Error");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Command    function /deleteECMStreamSetup                          */
    /*****************************************************************************/
    void deleteECMStreamSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        int channel_id,stream_id;
        std::string str_channel_id,str_stream_id;
        Json::Value json;
        Json::FastWriter fastWriter;
        
        std::string para[] = {"channel_id","stream_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("deleteECMStreamSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_stream_id = getParameter(request.body(),"stream_id"); 
            
            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyInteger(str_stream_id);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id =std::stoi(getParameter(request.body(),"channel_id"));
                stream_id = std::stoi(getParameter(request.body(),"stream_id")); 
                if(db->isECMStreamExists(channel_id,stream_id)){
                	json["desc"] = disableECMStreams(str_channel_id,str_stream_id);
                	if(deleteECMStream(channel_id,stream_id)){
	                    json["error"]= false;
	                    json["message"]= "Stream deleted!";
	                    addToLog("deleteECMStreamSetup","Success");
	                }else{
	                	json["error"]= true;
                    	json["message"]= "Stream already deleted!";	
	                }
                }else{
                    json["error"]= true;
                    json["message"]= "Stream does't exists!";
                    addToLog("deleteECMStreamSetup","Error");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value disableECMStreams(std::string channel_id,std::string stream_id){
    	Json::Value json,private_data_list;
    	Json::Value JSON_ecm_desc = db->getEnabledEMCStreams(channel_id,stream_id);
    	int flag = 0;
    	if(JSON_ecm_desc["error"] == false){
			for (int i = 0; i < JSON_ecm_desc["rmx_nos"].size(); ++i)
        	{

                std::string rmx_no = JSON_ecm_desc["rmx_nos"][i].asString();
                std::string output = JSON_ecm_desc["output_channel"][i].asString();
                std::string programNumber = JSON_ecm_desc["access_criteria"][i].asString();
                std::string service_pid = JSON_ecm_desc["service_pids"][i].asString();
                std::string input = JSON_ecm_desc["input_channel"][i].asString();
                // std::string programNumber = JSON_ecm_desc["access_criteria"][i].asString();
                Json::Value auth_outputs,ecm_pids,elementryPid,service_pids,JSON_CA_System_id;
		    	db->enableECM(channel_id,stream_id,service_pid,programNumber,rmx_no,output,input,flag);
		    	Json::Value JSON_ecm_desc = db->getECMDescriptors(rmx_no,input);
		    	// json["ecms"] = JSON_ecm_desc;
		        std::cout<<"-------------------------------------------------------------------------------";
		        if (JSON_ecm_desc["error"] == false)
		        {
		        	for (int i = 0; i < JSON_ecm_desc["ca_system_ids"].size(); ++i)
		        	{

		                std::string system_id = JSON_ecm_desc["ca_system_ids"][i].asString();

		                system_id = system_id.substr(2,system_id.length()-6);
		                // std::cout<<system_id;
		        		JSON_CA_System_id.append(std::to_string(getHexToLongDec(system_id)));
		        		auth_outputs.append("255");
		        		service_pids.append(JSON_ecm_desc["service_pids"][i].asString());

		        		ecm_pids.append(JSON_ecm_desc["ecm_pids"][i].asString());

		        	}
		        	json = callSetPMTCADescriptor(service_pids,JSON_CA_System_id,ecm_pids,private_data_list,std::stoi(rmx_no),output,input);
		        	// Json::Value customPid = callsetCustomPids(ecm_pids,auth_outputs,std::stoi(rmx_no),output);
		         //   	if(customPid["error"] == false){
		         //   		json["customPid"] = "Successfully enabled ECM PID!";
		         //   	}
		         //   	else{
		         //   		json["customPid"] = "Failed enabled ECM PID!";
		         //   	}
		        }
		        else{
		        	//Empty the CA descriptor
		        	json = callSetPMTCADescriptor(service_pids,JSON_CA_System_id,ecm_pids,private_data_list,std::stoi(rmx_no),output,input);
		        }
		        // if(json["error"] == false){
		        // 	unsigned int index;
		        // 	unsigned int indexValue =  db->getScramblingIndex(rmx_no,output);
		        // 	if(flag){
		        // 		index = db->getCWKeyIndex(channel_id,stream_id,programNumber);
		        // 		if(index == -1)
		        // 			index = allocateScramblingIndex(indexValue);
		        // 	}else{
		        // 		index = db->getCWKeyIndex(channel_id,stream_id,programNumber);
		        // 	}
		        // 	int rmx_id =std::stoi(rmx_no);
		        // 	std::string port =(rmx_id == 1 || rmx_id == 2)? "5000" : ((rmx_id == 3 || rmx_id == 4)? "5001" : "5002");
		        // 	updateCWIndex(channel_id,stream_id,std::to_string(index),port,flag);
		        // 	db->updateCWIndex(rmx_no,output,channel_id,stream_id,programNumber,index,indexValue,flag);
		        // }

        	}
    	}
    	return JSON_ecm_desc;
    }
    int deleteECMStream(int channel_id,int stream_id){
        int is_deleted=0;
        reply = (redisReply *)redisCommand(context,"SELECT 5");
        reply = (redisReply *)redisCommand(context,("DEL stream:ch_"+std::to_string(channel_id)+":stm_"+std::to_string(stream_id)).c_str());
        
        if(db->deleteECMStream(channel_id,stream_id)){
            is_deleted=1;
            std::cout<<"DELETED"<<std::endl;
            streams_json=db->getScrambledServices();
            reply = (redisReply *)redisCommand(context,("INCR stream_counter:ch_"+std::to_string(channel_id)).c_str());
            reply = (redisReply *)redisCommand(context,("SET deleted_ecm_stream:ch_"+std::to_string(channel_id)+":stm_"+std::to_string(stream_id)+" 1").c_str());
        }
        freeReplyObject(reply);
        return is_deleted;
    }

    /*****************************************************************************/
    /*  Command    function addEmmgSetup                          */
    /*****************************************************************************/
    void addEmmgSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        int data_id,port,bw,channel_id,stream_id,emm_pid;
        std::string client_id,str_data_id,str_port,str_bw,str_channel_id,str_stream_id,str_emm_pid;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","client_id","data_id","bw","port","stream_id","emm_pid"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("addEmmgSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            client_id = getParameter(request.body(),"client_id");
            str_data_id =getParameter(request.body(),"data_id");
            str_bw = getParameter(request.body(),"bw");
            str_port =getParameter(request.body(),"port");
            str_stream_id = getParameter(request.body(),"stream_id");
            str_emm_pid = getParameter(request.body(),"emm_pid");
            error[0] = verifyInteger(str_channel_id);
            error[1] = 1;
            error[2] = verifyInteger(str_data_id);
            error[3] = verifyInteger(str_bw);
            error[4] = verifyInteger(str_port);
            error[5] = verifyInteger(str_stream_id);
            error[6] = verifyInteger(str_emm_pid);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id =std::stoi(str_channel_id);
                data_id =std::stoi(str_data_id);
                bw = std::stoi(str_bw);
                port = std::stoi(str_port);
                stream_id = std::stoi(str_stream_id);
                emm_pid = std::stoi(str_emm_pid);
               if(db->addEMMchannelSetup(channel_id,client_id,data_id,bw,port,stream_id,emm_pid)){
                std::string currtime=getCurrentTime();
                    emmgSetup(channel_id,stream_id,data_id,client_id,bw,port,currtime,emm_pid);
                    updateCATCADescriptor(str_channel_id);
                    json["error"]= false;
                    json["message"]= "EMM added!";
                    addToLog("addEmmgSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Mysql error while adding EMM channel!!";
                    addToLog("addEmmgSetup","Mysql error while adding EMM channel!");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
     void emmgSetup(int channel_id,int stream_id, int data_id,std::string client_id,int bw,int port,std::string currtime,int emm_pid){
        std::string CA_system_id = client_id.substr(2);
        reply = (redisReply *)redisCommand(context,"SELECT 6");
        std::string query ="HMSET UI_Input channel_id "+std::to_string(channel_id)+" client_id "+CA_system_id+" data_id "+std::to_string(data_id)+" port "+std::to_string(port)+" stream_id "+std::to_string(stream_id)+" PID "+std::to_string(emm_pid)+" bandwidth "+std::to_string(bw)+" is_deleted 0";
        reply = (redisReply *)redisCommand(context,query.c_str());
        // reply = (redisReply *)redisCommand(context,("GET emm_channel_counter:ch_"+std::to_string(channel_id)).c_str());
        // if(isAdd == 1){
        //     this->channel_ids = channel_id;
        //     this->client_id = client_id;
        //     this->emm_port = port;
        //     this->data_id = data_id;
        //     this->emm_bw = emm_bw;
        //     this->stream_id = stream_id;
        //     int err = pthread_create(&thid, 0,&StatsEndpoint::spawnEMMChannel, this);
        //     if (err != 0){
        //         printf("\ncan't create thread :[%s]", strerror(err));
        //     }else{  
        //         printf("\n Thread created successfully\n");
        //         reply = (redisReply *)redisCommand(context,("SET emm_channel_counter:ch_"+std::to_string(channel_id)+" 1").c_str());
        //     }  
        // }else{
        //     printf("\n Thread already exists\n");
        //     reply = (redisReply *)redisCommand(context,("INCR emm_channel_counter:ch_"+std::to_string(channel_id)).c_str());
        // }
        freeReplyObject(reply);
    }
    /*****************************************************************************/
    /*  Command    function updateEmmgSetup                          */
    /*****************************************************************************/
    void updateEmmgSetup(const Rest::Request& request, Net::Http::ResponseWriter response){
        int data_id,port,bw,channel_id,stream_id,emm_pid;
        std::string client_id,str_data_id,str_port,str_bw,str_channel_id,str_stream_id,str_emm_pid;
        Json::Value json;
        Json::Value jsonMsg;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","client_id","data_id","bw","port","stream_id","emm_pid"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("updateEmmgSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            client_id = getParameter(request.body(),"client_id");
            str_data_id =getParameter(request.body(),"data_id");
            str_bw = getParameter(request.body(),"bw");
            str_port =getParameter(request.body(),"port");
            str_stream_id = getParameter(request.body(),"stream_id");
            str_emm_pid = getParameter(request.body(),"emm_pid");
            error[0] = verifyInteger(str_channel_id);
            error[1] = 1;
            error[2] = verifyInteger(str_data_id);
            error[3] = verifyInteger(str_bw);
            error[4] = verifyInteger(str_port);
            error[5] = verifyInteger(str_stream_id);
            error[6] = verifyInteger(str_emm_pid);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
                channel_id =std::stoi(str_channel_id);
                data_id =std::stoi(str_data_id);
                bw = std::stoi(str_bw);
                port = std::stoi(str_port);
                stream_id = std::stoi(str_stream_id);
                emm_pid = std::stoi(str_emm_pid);
                if(db->updateEMMchannelSetup(channel_id,client_id,data_id,bw,port,stream_id,str_emm_pid)){
                    std::string currtime=getCurrentTime();
                    emmgSetup(channel_id,stream_id,data_id,client_id,bw,port,currtime,emm_pid);
                    json["error"]= false;
                    json["message"]= "EMM updated!";
                    addToLog("updateEmmgSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Mysql error while adding EMM channel!!";
                    addToLog("updateEmmgSetup","Mysql error while adding EMM channel!");
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Command    function deleteEMMSetup                          */
    /*****************************************************************************/
    void deleteEMMSetup(const Rest::Request& request, Net::Http::ResponseWriter response)
    {
        int channel_id;
        std::string str_channel_id;
        Json::Value json;
        Json::FastWriter fastWriter;
        std::string para[] = {"channel_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("deleteEMMSetup",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            if(verifyInteger(str_channel_id)){
                channel_id = std::stoi(str_channel_id);
                if(db->isEMMExists(channel_id)){ //&& deleteEMMChannel(channel_id)
                    updateCATCADescriptor(str_channel_id);
                    json["error"]= false;
                    json["message"]= "EMM deleted!";
                    addToLog("deleteEMMSetup","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "EMM does't exists or already deleted!";
                    addToLog("deleteEMMSetup","Error");
                }
            }else{
                json["error"]= true;
                json["message"]= "Invalid channel id!";
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    int deleteEMMChannel(int channel_id){
        int is_deleted = 0;
        reply = (redisReply *)redisCommand(context,"SELECT 6");
        // reply = (redisReply *)redisCommand(context,("HGETALL UI_Input").c_str());
        // if(reply->elements>0){
            std::string query ="HMSET UI_Input is_deleted 1";
            reply = (redisReply *)redisCommand(context,query.c_str());
            if(db->deleteEMM(channel_id)){
                is_deleted= 1;
                std::cout<<"EMM DELETED"<<std::endl;
                // reply = (redisReply *)redisCommand(context,("INCR emm_channel_counter:ch_"+std::to_string(channel_id)).c_str());
                // reply = (redisReply *)redisCommand(context,("SET deleted_emm:ch_"+std::to_string(channel_id)+" 1").c_str());
            // }
        }
        freeReplyObject(reply);
        return is_deleted;
    }

    void killThread(int channel_id)
    {
        std::cout<<"main(): sending cancellation request\n"<<std::endl;
          int s = pthread_cancel(thid);
           if (s != 0)
                std::cout<<"pthread_cancel"<<std::endl;    
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<s<<std::endl;
    }
 
    // void deleteECMStream(int channel_id){
    //     
    //     reply = (redisReply *)redisCommand(context,"SELECT 5");
    //     reply = (redisReply *)redisCommand(context,("HGETALL Channel_list:"+std::to_string(channel_id)).c_str());
    //     if(reply->elements>0){
    //         std::string supercas_id =reply->element[3]->str;
    //         std::string ip =reply->element[5]->str;
    //         int port = std::stoi(reply->element[7]->str);
    //         std::string query ="HMSET Channel_list:"+std::to_string(channel_id)+" channel_id "+std::to_string(channel_id)+" supercas_id "+supercas_id+" ip "+ip+" port "+std::to_string(port)+" is_deleted 1";
    //         reply = (redisReply *)redisCommand(context,query.c_str());
    //         if(db->deleteECM(channel_id)){
    //             std::cout<<"DELETED"<<std::endl;
    //             streams_json=db->getStreams();
    //             reply = (redisReply *)redisCommand(context,("INCR channel_counter:ch_"+std::to_string(channel_id)).c_str());
    //         }
    //     }
    //     freeReplyObject(reply);
    // }
    
    void forkChannels(){
        int err;
        Json::Value json,channel_json;
        json=db->getChannels();
        channel_json=json["list"];
       // std::cout<<channel_json.size()<<std::endl;
        for(int i=0;i<channel_json.size();i++){
            if(std::stoi((channel_json[i]["is_enable"]).asString())){
                int channel_id = std::stoi(channel_json[i]["channel_id"].asString());
                std::string supercas_id = channel_json[i]["supercas_id"].asString();
                int ecm_port = std::stoi(channel_json[i]["port"].asString());
                std::string ecm_ip = channel_json[i]["ip"].asString();

                reply = (redisReply *)redisCommand(context,"SELECT 5");
                // reply = (redisReply *)redisCommand(context,("DEL Channel_list:"+std::to_string(channel_id)).c_str());
                std::string query ="HMSET Channel_list:"+std::to_string(channel_id)+" channel_id "+std::to_string(channel_id)+" supercas_id "+supercas_id+" ip "+ecm_ip+" port "+std::to_string(ecm_port)+"";
                reply = (redisReply *)redisCommand(context,query.c_str());
                this->channel_ids=channel_id;
                this->supercas_id = supercas_id;
                this->ecm_port = ecm_port;
                this->ecm_ip = ecm_ip;

                err = pthread_create(&thid, 0,&StatsEndpoint::spawnChannel, this);
                if (err != 0){
                    printf("\ncan't create thread :[%s]", strerror(err));
                }else{
                    printf("\n Thread already exists\n");
                    reply = (redisReply *)redisCommand(context,("INCR channel_counter:ch_"+std::to_string(this->channel_ids)).c_str());
                }    
                usleep(1000000);
            }
        }
    }
    void forkEMMChannels(){
        int err;
        Json::Value json,channel_json;
        json=db->getEMMChannels();
        channel_json=json["list"];
        std::cout<<channel_json.size()<<std::endl; 
        for(int i=0;i<channel_json.size();i++){
            if(std::stoi((channel_json[i]["is_enable"]).asString())){
                int channel_id = std::stoi(channel_json[i]["channel_id"].asString());
                std::string client_id = channel_json[i]["client_id"].asString();
                int emm_port = std::stoi(channel_json[i]["port"].asString());
               int data_id = std::stoi(channel_json[i]["data_id"].asString());
                int emm_bw = std::stoi(channel_json[i]["bw"].asString());
                int stream_id = std::stoi(channel_json[i]["stream_id"].asString());
                
                reply = (redisReply *)redisCommand(context,"SELECT 3");
                std::string query ="HMSET emm_channel_list:"+std::to_string(channel_id)+" channel_id "+std::to_string(channel_id)+" client_id "+client_id+" data_id "+std::to_string(data_id)+" port "+std::to_string(emm_port)+" bw "+std::to_string(emm_bw)+" stream_id "+std::to_string(stream_id)+" is_deleted 0";
                reply = (redisReply *)redisCommand(context,query.c_str());
                if(reply->str){
                    this->channel_ids=channel_id;
                    this->client_id = client_id;
                    this->emm_port = emm_port;
                    this->data_id = data_id;
                    this->emm_bw = emm_bw;
                    this->stream_id = stream_id;

                    err = pthread_create(&thid, 0,&StatsEndpoint::spawnEMMChannel, this);
                    if (err != 0){
                        printf("\ncan't create thread :[%s]", strerror(err));
                    }else{
                        printf("\n Thread already exists\n");
                        reply = (redisReply *)redisCommand(context,("INCR emm_channel_counter:ch_"+std::to_string(this->channel_ids)).c_str());
                    }    
                }
            }
        }
    }


    /*****************************************************************************/
    /*  Command    function scrambleService                          */
    /*****************************************************************************/
    void scrambleService(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_channel_id,str_service_id,str_stream_id;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","service_id","stream_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("scrambleService",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_service_id =getParameter(request.body(),"service_id");
            str_stream_id = getParameter(request.body(),"stream_id");
            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyInteger(str_service_id);
            error[2] = verifyInteger(str_stream_id);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
             if(all_para_valid){
            // if(verifyInteger(str_channel_id)){
                channel_id = std::stoi(str_channel_id);
                service_id = std::stoi(str_service_id);
                stream_id = std::stoi(str_stream_id);
                if(db->isECMExists(channel_id) && db->isECMStreamExists(channel_id,stream_id) && !(db->isStreamAlreadUsed(channel_id,stream_id))){
                    db->scrambleService(channel_id,stream_id,service_id);
                    streams_json = db->getScrambledServices();
                    if(CW_THREAD_CREATED==0){
                        err = pthread_create(&tid, 0,&StatsEndpoint::cwProvision, this);
                        if (err != 0){
                            printf("\ncan't create thread :[%s]", strerror(err));
                        }else{
                            CW_THREAD_CREATED=1;
                            printf("\n Thread created successfully\n");
                        }
                    }
                    json["error"]= false;
                    json["message"]= "Service scrambled!";
                    // json["channel_id"] = channel_id;
                    // json["serv_id"] = str_service_id;
                    // json["stream_id"] = str_stream_id;
                    addToLog("scrambleService","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Invalid channel_id,stream_id or stream alread mapped!"; 
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Command    function deScrambleService                          */
    /*****************************************************************************/
    void deScrambleService(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_channel_id,str_service_id,str_stream_id;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"channel_id","service_id","stream_id"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true;
        addToLog("deScrambleService",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_channel_id =getParameter(request.body(),"channel_id");
            str_service_id =getParameter(request.body(),"service_id");
            str_stream_id = getParameter(request.body(),"stream_id");
            error[0] = verifyInteger(str_channel_id);
            error[1] = verifyInteger(str_service_id);
            error[2] = verifyInteger(str_stream_id);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
             if(all_para_valid){
            // if(verifyInteger(str_channel_id)){
                channel_id = std::stoi(str_channel_id);
                service_id = std::stoi(str_service_id);
                stream_id = std::stoi(str_stream_id);
                if(db->isECMExists(channel_id) && db->isECMStreamExists(channel_id,stream_id) && (db->isStreamAlreadUsed(channel_id,stream_id))){
                    db->deScrambleService(channel_id,stream_id,service_id);
                    streams_json = db->getScrambledServices();
                    json["error"]= false;
                    json["message"]= "Service de-scrambled!";
                    // json["channel_id"] = channel_id;
                    // json["serv_id"] = str_service_id;
                    // json["stream_id"] = str_stream_id;
                    addToLog("deScrambleService","Success");
                }else{
                    json["error"]= true;
                    json["message"]= "Invalid channel_id,stream_id !"; 
                }
            }
        }else{
            json["error"]= true;
            json["message"]= res;
        }
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void forkCWthread(){

        int err;
        Json::Value streams=db->getStreams();
        if(streams["error"]==false){
            for (int i = 0; i < streams["list"].size(); ++i)
            {
                // std::cout<<streams["list"][i]["access_criteria"].asString()<<std::endl;
                std::string currtime=getCurrentTime();
                ecmStreamSetup(std::stoi(streams["list"][i]["channel_id"].asString()),std::stoi(streams["list"][i]["stream_id"].asString()),std::stoi(streams["list"][i]["ecm_id"].asString()),streams["list"][i]["access_criteria"].asString(),std::stoi(streams["list"][i]["cp_number"].asString()),currtime,streams["list"][i]["ecm_pid"].asString());
            }
        }
        streams_json = db->getScrambledServices();
        if(streams_json["list"].size()>0){
            err = pthread_create(&tid, 0,&StatsEndpoint::cwProvision, this);
            if (err != 0){
                printf("\ncan't create thread :[%s]", strerror(err));
            }else{
                CW_THREAD_CREATED=1;
                printf("\n Thread created successfully\n");
            }    
        }
    }
    
    /*****************************************************************************/
    /*  Command    function setHostNITLcn                          */
    /*****************************************************************************/
    void setHostNITLcn(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_service_id,str_lcn_id,input,output,rmx_no;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"service_id","lcn_id","input","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,nit_insert_error = true;
        addToLog("setHostNITLcn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_service_id =getParameter(request.body(),"service_id");
            str_lcn_id =getParameter(request.body(),"lcn_id");
            input =getParameter(request.body(),"input");
            output =getParameter(request.body(),"output");
            rmx_no =getParameter(request.body(),"rmx_no");
            
            error[0] = verifyInteger(str_service_id);
            error[1] = verifyInteger(str_lcn_id,0,0,1023,1);
            error[2] = verifyInteger(input,1,1,INPUT_COUNT);
            error[3] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[4] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);

            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
            	if(db->isLcnIdExists(str_lcn_id) == 0){
		            if(db->addLcnIds(str_service_id,str_lcn_id,input,output,rmx_no) > 0){
		            	json["error"]= false;
	                	json["message"]= "LCN updated successfully!";	
		            }else{
		            	json["error"]= true;
	                	json["message"]= "Failed to updated LCN!";	
		            }          		
		        }else{
		        	json["error"]= true;
	               	json["message"]= "Duplicate LCN!";	
		        }

            }else{
                json["error"]= true;
                json["message"]= "Invalid input!"; 
            }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Command    function unsetHostNITLcn                          */
    /*****************************************************************************/
    void unsetHostNITLcn(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_service_id,str_lcn_id,input,output,rmx_no;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"service_id","input","output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,nit_insert_error = true;
        addToLog("unsetHostNITLcn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_service_id =getParameter(request.body(),"service_id");
            input =getParameter(request.body(),"input");
            output =getParameter(request.body(),"output");
            rmx_no =getParameter(request.body(),"rmx_no");
            
            error[0] = verifyInteger(str_service_id);
            error[1] = verifyInteger(input,1,1,INPUT_COUNT);
            error[2] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[3] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);

            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
                if(db->unsetLcnId(str_service_id,input,output,rmx_no) > 0){
                    json["error"]= false;
                    json["message"]= "LCN updated successfully!";   
                }else{
                    json["error"]= true;
                    json["message"]= "Failed to updated LCN!";  
                }               

            }else{
                json["error"]= true;
                json["message"]= "Invalid input!"; 
            }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    /*****************************************************************************/
    /*  Command    function getHostNITLcn                          */
    /*****************************************************************************/
    void getHostNITLcn(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_service_id,str_lcn_id,input,output,rmx_no;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"output","rmx_no"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,nit_insert_error = true;
        addToLog("getHostNITLcn",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            output =getParameter(request.body(),"output");
            rmx_no =getParameter(request.body(),"rmx_no");
           
            error[0] = verifyInteger(output,1,1,OUTPUT_COUNT);
            error[1] = verifyInteger(rmx_no,1,1,RMX_COUNT,1);

            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
            	Json::Value json_lcn_list = db->getLcnIds(output,rmx_no);
	            if( json_lcn_list["error"] == false){
	            	json["lcn_list"] = json_lcn_list["list"];
	            	json["error"]= false;
                	json["message"]= "LCN list!";	
	            }else{
	            	json["error"]= true;
                	json["message"]= "Empty LCN list!";	
	            }          		
            }else{
                json["error"]= true;
                json["message"]= "Invalid input!"; 
            }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Command    function setPrivateData                          */
    /*****************************************************************************/
    void setPrivateData(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string sPrivateData;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"private_data_id","private_data","addFlag"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,nit_insert_error = true;
        addToLog("setPrivateData",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            std::string private_data_id =getParameter(request.body(),"private_data_id");
            sPrivateData =getParameter(request.body(),"private_data");
            std::string addflag =getParameter(request.body(),"addFlag");
            json["mod"] =(int) sPrivateData.length()%2;
            if(verifyIsHex(sPrivateData)>0 && (sPrivateData.length()%2 == 0) && verifyInteger(addflag,1,1,1)){
                if((sPrivateData.substr(0,2) == "0x") && ((sPrivateData.substr(2,2) == "4a")||(sPrivateData.substr(2,2) == "4A") ) && (sPrivateData.length() > 20))
                {
                    long pDataLen = getHexToLongDec(sPrivateData.substr(4,2));
                    if((sPrivateData.substr(6,sPrivateData.length())).length() == (pDataLen*2)){
                        if(db->addPrivateData(sPrivateData.substr(2,sPrivateData.length()),private_data_id,std::stoi(addflag)) >0){
                            json["error"]= false;
                            json["message"]= "Private Data updated successfully!";   
                        }else{
                            json["error"]= true;
                            json["message"]= "Error while adding to database!";  
                        }
                    }else{
                        json["error"]= true;
                        json["message"]= "Is not valide private data(Length missmatch)!";  
                    }
                }else{
                    json["error"]= true;
                    json["message"]= "Private Data is not valid!"; 
                }

            }else{
                json["error"]= true;
                json["message"]= "Private Data is not valid (Even length) HEX OR addFlag is not valide integer!"; 
            }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }

    /*****************************************************************************/
    /*  Command    function getPrivateData                          */
    /*****************************************************************************/
    void getPrivateData(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;

        json = db->getPrivateData(1);
        if(json["error"] == true)
            json["message"] = "No records found!";
        else
            json["message"] = "Private data list!";

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    

    /*****************************************************************************/
    /*  Command    function    disableNITable                       */
    /*****************************************************************************/
    void disableNITable(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_network_id,network_name,str_version;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;
        bool error_disabling_NIT = false;
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
            for (int output = 0; output < OUTPUT_COUNT; ++output)
            {
                Json::Value json_io = callSetInputOutput("0",std::to_string(output),rmx_no);
                if(json_io["error"] == false){
                    Json::Value json_nitmode =  callSetNITmode("3",std::to_string(output),rmx_no);
                    usleep(100);
                    if(json_nitmode["error"] == true)
                        error_disabling_NIT = true;
                }else{
                    error_disabling_NIT = true;
                }
            }
        }
        if(error_disabling_NIT){
            json["message"] = "NIT disabled Partially, Please try again!"; 
            json["error"] = true; 
        }else{
            db->disableHostNIT();
            json["message"] = "NIT disabled successfully"; 
            json["error"] = false; 
        }
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    //NIT creation

    int NIT_VER = 31;
    /*****************************************************************************/
    /*  Command    function insertNITable                          */
    /*****************************************************************************/
    void insertNITable(const Rest::Request& request, Net::Http::ResponseWriter response){
        std::string str_network_id,network_name,str_version;
        int channel_id,stream_id,service_id;
        Json::Value json;
        Json::FastWriter fastWriter;

        std::string para[] = {"network_id","network_name","version"};
        int error[ sizeof(para) / sizeof(para[0])];
        bool all_para_valid=true,nit_insert_error = true;
        addToLog("insertNITable",request.body());
        std::string res=validateRequiredParameter(request.body(),para, sizeof(para) / sizeof(para[0]));
        if(res=="0"){
            str_network_id =getParameter(request.body(),"network_id");
            network_name =getParameter(request.body(),"network_name");
            str_version = getParameter(request.body(),"version");
            error[0] = verifyInteger(str_network_id);
            error[1] = verifyString(network_name);
            error[2] = verifyInteger(str_version);
            for (int i = 0; i < sizeof(error) / sizeof(error[0]); ++i)
            {
               if(error[i]!=0){
                    continue;
                }
                all_para_valid=false;
                json["error"]= true;
                json[para[i]]= "Please give valid input for "+para[i]+"!";
            }
            if(all_para_valid){
                json = callInsertNITable(str_network_id,network_name);
            	// NIT_VER = (NIT_VER == 31)? 0 : NIT_VER+1;
            	// unsigned char* ucNetworkName =(unsigned char*) network_name.c_str();
            	// Json::Value NIT_Host_modes = db->getNITHostMode();
            	
             //    int pusSectionL=  getCountSections();
             //    std::cout<<"\n-----------------------------LAST SECTION COUNT----------------------------"<<pusSectionL<<endl;
            	// if(NIT_Host_modes["error"] == false){
            	// 	for (int i = 0; i <NIT_Host_modes["list"].size() ; ++i)
            	// 	{
            	// 		int rmx_no = std::stoi(NIT_Host_modes["list"][i]["rmx_no"].asString());
            	// 		std::string output = NIT_Host_modes["list"][i]["output"].asString();
            	// 		Json::Value json_io = callSetInputOutput("0",output,rmx_no);
	            // 		if(json_io["error"] == false){
	            // 			int count=  TS_GenNITSection(std::stoi(str_network_id),NIT_VER,ucNetworkName,pusSectionL);
	            // 			usleep(10000);
	            // 			nit_insert_error = false;
	            // 			//set NIT MODE 
	            // 			Json::Value json_nitmode =  callSetNITmode("2",output,rmx_no);
	            // 			usleep(100);
	            // 			if(json_nitmode["error"] == true)
	            // 				json["nit_mode"] = "Setting NIT mode failed"; 
	            // 		}else{
	            // 			json = json_io;
	            // 		}
             //            break;
	            // 	}
	            // }
	            // if(nit_insert_error == false){
	            // 	db->addNITDetails(NIT_VER,network_name,str_network_id);
	            // 	json["error"]= false;
             //    	json["message"]= "NIT inserted!";
	            // }else{
	            // 	json["error"]= true;
             //    	json["message"]= "Connection error!";	
	            // }          		
                
                
            }else{
                json["error"]= true;
                json["message"]= "Invalid input!"; 
            }
            
        }else{
            json["error"]= true;
            json["message"]= res;
        }

        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    Json::Value callInsertNITable(std::string str_network_id,std::string network_name){
        Json::Value json;

        NIT_VER = (NIT_VER == 31)? 0 : NIT_VER+1;
        unsigned char* ucNetworkName =(unsigned char*) network_name.c_str();
        Json::Value NIT_Host_modes = db->getNITHostMode();
        
        bool nit_insert_error = true;
        int pusSectionL=  getCountSections();
        std::cout<<"\n-----------------------------LAST SECTION COUNT----------------------------"<<pusSectionL<<endl;
        if(NIT_Host_modes["error"] == false){
            for (int i = 0; i <NIT_Host_modes["list"].size() ; ++i)
            {
                int rmx_no = std::stoi(NIT_Host_modes["list"][i]["rmx_no"].asString());
                std::string output = NIT_Host_modes["list"][i]["output"].asString();
                Json::Value json_io = callSetInputOutput("0",output,rmx_no);
                if(json_io["error"] == false){
                    int count=  TS_GenNITSection(std::stoi(str_network_id),NIT_VER,ucNetworkName,pusSectionL);
                    usleep(10000);
                    nit_insert_error = false;
                    //set NIT MODE 
                    Json::Value json_nitmode =  callSetNITmode("2",output,rmx_no);
                    usleep(100);
                    if(json_nitmode["error"] == true)
                        json["nit_mode"] = "Setting NIT mode failed"; 
                }else{
                    json = json_io;
                }
            }
        }
        if(nit_insert_error == false){
            db->addNITDetails(NIT_VER,network_name,str_network_id);
            json["error"]= false;
            json["message"]= "NIT inserted!";
        }else{
            json["error"]= true;
            json["message"]= "Connection error!";   
        } 
        return json; 
    }
    unsigned long crc_table[256] = {
  	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};


  	unsigned long crc32 (unsigned char *data, int len) {
	  	register int i;
	  	unsigned long crc = 0xffffffff;

	  	for (i=0; i<len; i++){
	 		// printf(" %d --> %x\n",i,*data);
	   		crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
	  		}

	  	return crc;
	}

    unsigned short setCableDeliverySystemDescriptor(unsigned char *pucDescriptor, unsigned int uiFrequency, unsigned char ucFecOuter, unsigned char ucModulation, unsigned int uiSymbolRate, unsigned char ucFecInner) {
	   unsigned char *pucData = pucDescriptor;

	   *(pucData++) = 0x44; // Descriptor tag
	   *(pucData++) = 11; // length
	   uiFrequency = getHexToLongDec(std::to_string(uiFrequency));
	   uiFrequency = uiFrequency<<16;
	   // std::cout<<uiFrequency<<endl;
	   *(pucData++) = (unsigned char)(uiFrequency>>24); // frequency
	   *(pucData++) = (unsigned char)(uiFrequency>>16); // frequency
	   *(pucData++) = (unsigned char)(uiFrequency>>8);  // frequency
	   *(pucData++) = (unsigned char)(uiFrequency);  // frequency
	   *(pucData++) = 0xFF;
	   *(pucData++) = 0xF0 | ucFecOuter; // FEC_outer
	   *(pucData++) = ucModulation; // modulation
	   uiSymbolRate = getHexToLongDec(std::to_string(uiSymbolRate));
	   *(pucData++) = (unsigned char) ((uiSymbolRate)>>24)&0xFF; // symbol_rate
	   *(pucData++) = (unsigned char) ((uiSymbolRate)>>16)&0xFF; // symbol_rate
	   *(pucData++) = (unsigned char) ((uiSymbolRate)>>8)&0xFF; // symbol_rate
	   *(pucData++) = (unsigned char) ((uiSymbolRate ) & 0x0F) | ucFecInner; // symbol_rate | FEC_inner

	   return 13; // full length of descriptor.
	}
	unsigned short setNetworkNameDescriptor(unsigned char *pucDescriptor, unsigned char *pucName) {
	   unsigned char *pucData = pucDescriptor;
	   unsigned char length = strlen((char*)pucName);

	   *(pucData++) = 0x40; // Descriptor tag
	   *(pucData++) = length; // Descriptor tag
	   while(*pucName) {
	      *(pucData++) = *(pucName++); // Descriptor tag
	   }
	   return (unsigned short)length+2;
	}
    unsigned short setPrivateDataDescriptor(unsigned char *pucDescriptor) {
       unsigned char *pucData = pucDescriptor;
       unsigned char *pucPrivateData;
       unsigned short usDataSize;
       Json::Value json_private_data = db->getPrivateData();
       std::cout<<json_private_data<<endl;
       unsigned short usPDataLength=0;
       std::string sPrivateData;
       if(json_private_data["error"] == false){
           usDataSize = json_private_data["list"].size();
           for (int i = 0; i < usDataSize; ++i)
           {
                sPrivateData = json_private_data["list"][i].asString();
                unsigned short length = sPrivateData.length();
                if(length%2 == 0){
                    for (int i = 0; i < length ; i+=2)
                     {
                        *(pucData++) = getHexToLongDec(sPrivateData.substr(i,2)); 
                        usPDataLength++;
                    }
                }
           }
           return usPDataLength;
       }else{
            return 0;
       }
    }

	int lcn_id =1;
	unsigned short setLcnDescriptor(unsigned char *pucDescriptor, unsigned int uiNbservices,
                    unsigned short *pucServiceId, unsigned short *pucChannelNumber, unsigned short *pucHdChannelNumber) {
	   	unsigned char *pDataLen, *pDataLenDesc, *pDataLenDesc2, *pDataLenDesc3;
	   	unsigned short sec_len=0, sec_len_desc=0, list_desc_len=0,desc_len=0;
		unsigned int input=0;
		unsigned char *pData = pucDescriptor;

	    *(pData++) = 0x83;
		desc_len = 0;
		pDataLenDesc3 = pData;
		pData++;
		list_desc_len +=2;
		for(input=0; input<uiNbservices; input++) {
	        unsigned short ucServiceId = (unsigned short) pucServiceId[input];
	        unsigned short ucChannelNumber = (unsigned short) pucChannelNumber[input];
	        *(pData++) = (unsigned char)(ucServiceId>>8);
	        *(pData++) = (unsigned char)ucServiceId;
	        *(pData++) = (unsigned char)((ucChannelNumber >>8) | 0xC0);
			*(pData++) = (unsigned char) ucChannelNumber ;
			lcn_id++;
			desc_len+=4;
			}

		list_desc_len += desc_len;
		*pDataLenDesc3 = desc_len;
		//printf("\n list_desc_len %d",list_desc_len);
	   // Simulcast !!!!

	  //HD channel LCN
	 /*	*(pData++) = 0x88;
		desc_len = 0;
		pDataLenDesc3 = pData;
		pData++;
		list_desc_len +=2;
		for(input=0; input<uiNbservices; input++) {
	        unsigned char ucServiceId = (unsigned char) *(pucServiceId++);
	        unsigned char ucHdChannelNumber = (unsigned char) *(pucHdChannelNumber++);
	        *(pData++) = (ucServiceId>>8);
	        *(pData++) =  ucServiceId;
			*(pData++) = ((ucHdChannelNumber>>8) | 0xFC);
			*(pData++) =  ucHdChannelNumber;
	        desc_len+=4;
		}

		list_desc_len += desc_len;
		*pDataLenDesc3 = desc_len; */
		return list_desc_len;
	}

	unsigned short setServiceListDescriptor(unsigned char *pucDescriptor,unsigned int uiNbservices,
                    unsigned short *pucServiceId,unsigned short *pucServiceType){
	      unsigned short list_desc_len=0,desc_len=0;
	      unsigned int input=0;
	      unsigned char *pData = pucDescriptor;
	      unsigned char *pDataLenDesc3;

	    *(pData++) = 0x41;
		desc_len = 0;
		pDataLenDesc3 = pData;
		pData++;
		list_desc_len +=2;
		for(input=0; input<uiNbservices; input++) {
	        unsigned short ucServiceId = (unsigned short) pucServiceId[input];
	        unsigned short ucServiceType = (unsigned short) pucServiceType[input];
	        *(pData++) = (unsigned char)(ucServiceId>>8);
	        *(pData++) = (unsigned char)ucServiceId;
	        *(pData++) = 0x01;//(ucServiceType);
			desc_len+=3;
			// printf("\n ucServiceType ---------->%d",ucServiceType);
			}
		list_desc_len += desc_len;
		*pDataLenDesc3 = desc_len;
	    return list_desc_len;
	}

	// TS_GenNITSection section

int TS_GenNITSection(unsigned short usNetworkId,unsigned char ucVersion, unsigned char *pucNetworkName,int iSectionCount) {	// 30300
	   unsigned short *pucServiceId=NULL,*pucChannelNumber=NULL,*pucHdChannelNumber,*pucServiceType=NULL,*pucServiceLcnId=NULL;
	   unsigned short usOriginalNetworkId,ucCurrentTsId;
	   unsigned int uiNbServices,uiSymbol_rate;
	   unsigned char pucNewSection[1024];     // 1 section allocated array
	   unsigned char *pucData;                // byte pointer for NIT section composing
	   unsigned char *pucSectionLength;       // 2 keep section length pointer
	   unsigned char *pucDescriptorLength;    // 2 keep descriptor length pointer
	   unsigned char *pucTSLength;
	   unsigned char i = 0;                   // Current section increment
	   unsigned char pucDescriptor[256];       // descriptor data
	   unsigned short usFullDescriptorLength=0; // descriptor length
	   unsigned short usFullTSLength=0;
	   unsigned short usDescriptorLength;     // descriptor length
	   unsigned short usFullSectionLength;
	   unsigned long val_crc;
	   unsigned short sec_len, sec_len_desc, list_desc_len;
	   unsigned short NIT_TAG_SEC_LEN3 = 3,NIT_NID_VER_LASTSEC_DESC_LENB7 = 7,TS_SECTION_LENB2 = 2,CRC32B4=4;
	   unsigned int ts_size=0,uiFrequency;
	   unsigned char ucFec_Inner = 0,ucFec_Outer = 0,ucModulation;
	   // generate required sections according to descriptors.
	   	int ts_cout=0;
	   	int section_count=0;
	   	Json::Value json_network_details = db->getNetworkDetailsForNIT(); 
	   	ts_size = json_network_details["list"].size();
	      std::cout<<json_network_details<<endl;
	    if(json_network_details["error"]==false){
	      	while(1) {
	      		printf("\n----Fisrt Section TS Start------------>%d\n ",ts_cout);
	      		pucNewSection[1024] = {0};
			    // point to the first byte of the allocated array
			    pucData = pucNewSection;
			    // first byte of the section
			    *(pucData++) = 0x40;
			    //skip section length and keep pointer to
			    pucSectionLength = pucData;
			    pucData+=2;
			    // insert network id
			    *(pucData++) =  (unsigned char)(usNetworkId>>8);
			    *(pucData++) =  (unsigned char)(usNetworkId&0xFF);
			    // insert version number
			    *(pucData++) =  0xC1 | (ucVersion<<1);
			    // Insert section number
			    *(pucData++) = section_count;
			    // Jump last section number (don't know yet)
			    *(pucData++) = iSectionCount;
			    // Save descriptor length pointer and jump it
			    pucDescriptorLength = pucData;
			    pucData += 2;
			    // Add Network Name descriptor
			    usDescriptorLength = setNetworkNameDescriptor(pucData, pucNetworkName);
			    usFullDescriptorLength = usDescriptorLength;
			    pucData += usDescriptorLength;

			    //Add linkage descriptor here
                usDescriptorLength = setPrivateDataDescriptor(pucData);
                usFullDescriptorLength += usDescriptorLength;
                pucData += usDescriptorLength;
                // printf("\n----      setPrivateDataDescriptor   ------------>%d\n ",usDescriptorLength);
                
			    *(pucDescriptorLength++) =  0xF0 | (usFullDescriptorLength>>8);
			    *(pucDescriptorLength++) =  (usFullDescriptorLength&0xFF);

			    //to chech the length of full section
			    usFullSectionLength = usFullDescriptorLength + NIT_NID_VER_LASTSEC_DESC_LENB7;

			    //TS steams loop start here
			      
			    pucTSLength = pucData;
			    pucData += 2;
			    usFullTSLength = 0;
			   
	            for (; ts_cout < json_network_details["list"].size(); ++ts_cout)
	            {
	            	
	            	// std::cout<<"---------------------------TS_COUNT ---------------"<<json_network_details["list"][ts_cout]["ts_id"]<<endl;
	            	std::string rmx_no = json_network_details["list"][ts_cout]["rmx_no"].asString();
	            	std::string output = json_network_details["list"][ts_cout]["output"].asString();

	              	ucCurrentTsId = (unsigned short) std::stoi(json_network_details["list"][ts_cout]["ts_id"].asString());
	              	usOriginalNetworkId = (unsigned short) std::stoi(json_network_details["list"][ts_cout]["original_netw_id"].asString());
			      	//transport stream loop len
			      	*(pucData++) = (ucCurrentTsId>>8);
			      	*(pucData++) = (ucCurrentTsId&0xFF);
			      	*(pucData++) = (usOriginalNetworkId>>8);
			     	*(pucData++) = (usOriginalNetworkId&0xFF);

			       	usFullDescriptorLength = 0;
			       	pucDescriptorLength = pucData;
			       	pucData += 2;

			      	ucModulation = (unsigned char) std::stoi(json_network_details["list"][ts_cout]["modulation"].asString());
			      	ucModulation+=1;
	              	uiFrequency = (unsigned int) std::stoi(json_network_details["list"][ts_cout]["frequency"].asString());
	              	uiSymbol_rate = ((unsigned int) std::stoi(json_network_details["list"][ts_cout]["symbol_rate"].asString()) / 1000);
	              	uiSymbol_rate = uiSymbol_rate*100;
	              	// printf("--------TS Start HERE--------> %d ----->> %d ----------------- >> %d \n", ts_cout,ucCurrentTsId,uiFrequency);
			        //Add cable_delivery descriptor
			      	usDescriptorLength = setCableDeliverySystemDescriptor(pucData,uiFrequency,ucFec_Outer,ucModulation,uiSymbol_rate,ucFec_Inner);
			      	usFullDescriptorLength += usDescriptorLength;
			      	pucData += usDescriptorLength;
				      	//printf("-------CABLE DELE DESC LEN--------->%d\n", usDescriptorLength);

				      	Json::Value json_active_progs = db->getActivePrograms(output,rmx_no);
				      	// std::cout<<json_active_progs<<endl;
				      	if(json_active_progs["error"]==false){
				      		uiNbServices =(unsigned int) json_active_progs["list"].size();
				      		int iServiceCount = 0;
				      		pucServiceId =(short unsigned int*) new short[uiNbServices];
				      		pucServiceType =(short unsigned int*) new short[uiNbServices];
				      		for (int iServiceCount = 0; iServiceCount < uiNbServices; ++iServiceCount)
				      		{  
                                int service_id = std::stoi(json_active_progs["list"][iServiceCount]["service_id"].asString());
                                int orig_service_id =std::stoi(json_active_progs["list"][iServiceCount]["orig_service_id"].asString());
				      			pucServiceId[iServiceCount] = (unsigned short) (service_id != -1)? service_id : orig_service_id;	
				      			pucServiceType[iServiceCount] = 1;//Get the service type from getProg INFO
				      		}
				      		//Add Service List Descriptor
				      		usDescriptorLength = setServiceListDescriptor(pucData, uiNbServices, pucServiceId,pucServiceType);
					      	usFullDescriptorLength += usDescriptorLength;
					      	pucData += usDescriptorLength;
				  		}

				  		// // Add LCN Descriptor
				  		Json::Value json_prog_lcn_ids = db->getLcnIds(output,rmx_no);
				  		// std::cout<<json_prog_lcn_ids<<endl;
				  		if(json_prog_lcn_ids["error"]==false){
				  			uiNbServices =(unsigned int) json_prog_lcn_ids["list"].size();
				      		int iLcnCount = 0;
				      		pucServiceLcnId =(short unsigned int*) new short[uiNbServices];
				      		pucChannelNumber =(short unsigned int*) new short[uiNbServices];
					  		for (int iLcnCount = 0; iLcnCount < uiNbServices; ++iLcnCount)
				      		{
				      			pucServiceLcnId[iLcnCount] = (unsigned short)std::stoi(json_prog_lcn_ids["list"][iLcnCount]["service_id"].asString());	
				      			pucChannelNumber[iLcnCount] = (unsigned short)std::stoi(json_prog_lcn_ids["list"][iLcnCount]["lcn_id"].asString());
				      		}
					      	usDescriptorLength = setLcnDescriptor(pucData, uiNbServices, pucServiceId, pucChannelNumber, pucHdChannelNumber);
					      	usFullDescriptorLength += usDescriptorLength;
					      	pucData += usDescriptorLength;
				  		}

				  		//END OF Descriptors
				  		*(pucDescriptorLength++) =  0xF0 | (usFullDescriptorLength>>8);
					    *(pucDescriptorLength++) =  (usFullDescriptorLength&0xFF);
			      
			        // printf("--------usFullDescriptorLength-------->%d\n", usFullDescriptorLength+6);

			        usFullSectionLength += usFullDescriptorLength+6;
			        if(usFullSectionLength>900){
			        	// printf("%d---- SECTION Full------------>%d\n ", ts_cout,usFullSectionLength);
			            pucData = pucData-(usFullDescriptorLength+6);
			            usFullSectionLength = usFullSectionLength - (usFullDescriptorLength+6);
			            // ts_cout = ts_cout-1;
			            printf("%d---- SECTION Full------------>%d\n ", ts_cout,usFullSectionLength);
			            break;
			        }
			             //Full TS length
			       		 usFullTSLength += usFullDescriptorLength+6;
			       		 // printf("--------usFullDescriptorLength-------->%d\n", usFullDescriptorLength+6);
			    }
		        *(pucTSLength++) =  0xF0 | (usFullTSLength>>8);
		        *(pucTSLength++) =  (usFullTSLength&0xFF);

		        //add section len size
		        usFullSectionLength += TS_SECTION_LENB2;
		        usFullSectionLength += CRC32B4;

		        *(pucSectionLength++) =  0xF0 | ((usFullSectionLength)>>8);
		        *(pucSectionLength++) =  ((usFullSectionLength)&0xFF);
		        printf("----FULL SECTION LEN------------>%d\n", usFullSectionLength);

			    //usFullSectionLength+= NIT_TAG_SEC_LEN3;
			    int j = 0;
			    val_crc = crc32 (pucData-(usFullSectionLength+NIT_TAG_SEC_LEN3-CRC32B4), usFullSectionLength-CRC32B4+NIT_TAG_SEC_LEN3);
				*(pucData++) = val_crc>>24;
				*(pucData++) = val_crc>>16;
				*(pucData++) = val_crc>>8;
				*(pucData) = val_crc;
				usFullSectionLength +=NIT_TAG_SEC_LEN3;
				printf("\n");
			    unsigned char * ucSectionPayload = pucNewSection;
			    for(j = 0;j<usFullSectionLength;j++){
			        // printf(" %d --> %x \t\t",j,pucNewSection[j]);
			        printf("%x:",pucNewSection[j]);
			    }
			    int k = 0;
			    unsigned short usPointer=0;
			    while(usFullSectionLength)
			    {
		        	int size_of_payload = (usFullSectionLength>256)? 256 : usFullSectionLength;
		            c1.updateNITTable(ucSectionPayload+=k,size_of_payload,usPointer,section_count);
		            usPointer += size_of_payload;
		            usFullSectionLength -= size_of_payload;
		            // printf(")) %d --> %d\n",k,size_of_payload);
		            k =size_of_payload;
		        }
		        std::cout<<ts_cout<<"------NIT ------>>"<<ts_size;
		        if(ts_cout==ts_size)
					break;

			    section_count++;
		    }
				
		}
	    return 1;
	}

    int getCountSections() {   // 30300
        unsigned short usNetworkId = 1;
        unsigned char ucVersion = 1; 
        unsigned char *pucNetworkName =(unsigned char *) "test";
        unsigned short *pucServiceId=NULL,*pucChannelNumber=NULL,*pucHdChannelNumber,*pucServiceType=NULL,*pucServiceLcnId=NULL;
        unsigned short usOriginalNetworkId,ucCurrentTsId;
        unsigned int uiNbServices,uiSymbol_rate;
        unsigned char pucNewSection[1024];     // 1 section allocated array
       unsigned char *pucData;                // byte pointer for NIT section composing
       unsigned char *pucSectionLength;       // 2 keep section length pointer
       unsigned char *pucDescriptorLength;    // 2 keep descriptor length pointer
       unsigned char *pucTSLength;
       unsigned char i = 0;                   // Current section increment
       unsigned char pucDescriptor[256];       // descriptor data
       unsigned short usFullDescriptorLength=0; // descriptor length
       unsigned short usFullTSLength=0;
       unsigned short usDescriptorLength;     // descriptor length
       unsigned short usFullSectionLength;
       unsigned long val_crc;
       unsigned short sec_len, sec_len_desc, list_desc_len;
       unsigned short NIT_TAG_SEC_LEN3 = 3,NIT_NID_VER_LASTSEC_DESC_LENB7 = 7,TS_SECTION_LENB2 = 2,CRC32B4=4;
       unsigned int ts_size=0,uiFrequency;
       unsigned char ucFec_Inner = 0,ucFec_Outer = 0,ucModulation;
       // generate required sections according to descriptors.
        int ts_cout=0;
        int section_count=0;
        Json::Value json_network_details = db->getNetworkDetailsForNIT(); 
        ts_size = json_network_details["list"].size();
          std::cout<<json_network_details<<endl;
        if(json_network_details["error"]==false){
            while(1) {
                // printf("\n----Fisrt Section TS Start------------>%d\n ",ts_cout);
                pucNewSection[1024] = {0};
                // point to the first byte of the allocated array
                pucData = pucNewSection;
                // first byte of the section
                *(pucData++) = 0x40;
                //skip section length and keep pointer to
                pucSectionLength = pucData;
                pucData+=2;
                // insert network id
                *(pucData++) =  (unsigned char)(usNetworkId>>8);
                *(pucData++) =  (unsigned char)(usNetworkId&0xFF);
                // insert version number
                *(pucData++) =  0xC1 | (ucVersion<<1);
                // Insert section number
                *(pucData++) = section_count;
                // Jump last section number (don't know yet)
                *(pucData++) = 1;//(section_count == 0)? 0 : section_count-1;
                // Save descriptor length pointer and jump it
                pucDescriptorLength = pucData;
                pucData += 2;
                // Add Network Name descriptor
                usDescriptorLength = setNetworkNameDescriptor(pucData, pucNetworkName);
                usFullDescriptorLength = usDescriptorLength;
                pucData += usDescriptorLength;

                //Add linkage descriptor here
                usDescriptorLength = setPrivateDataDescriptor(pucData);
                usFullDescriptorLength += usDescriptorLength;
                pucData += usDescriptorLength;

                *(pucDescriptorLength++) =  0xF0 | (usFullDescriptorLength>>8);
                *(pucDescriptorLength++) =  (usFullDescriptorLength&0xFF);

                //to chech the length of full section
                usFullSectionLength = usFullDescriptorLength + NIT_NID_VER_LASTSEC_DESC_LENB7;

                //TS steams loop start here
                  
                pucTSLength = pucData;
                pucData += 2;
                usFullTSLength = 0;
               
                    for (; ts_cout < json_network_details["list"].size(); ++ts_cout)
                    {
                        
                        // std::cout<<"---------------------------TS_COUNT ---------------"<<json_network_details["list"][ts_cout]["ts_id"]<<endl;
                        std::string rmx_no = json_network_details["list"][ts_cout]["rmx_no"].asString();
                        std::string output = json_network_details["list"][ts_cout]["output"].asString();

                        ucCurrentTsId = (unsigned short) std::stoi(json_network_details["list"][ts_cout]["ts_id"].asString());
                        usOriginalNetworkId = (unsigned short) std::stoi(json_network_details["list"][ts_cout]["original_netw_id"].asString());
                        //transport stream loop len
                        *(pucData++) = (ucCurrentTsId>>8);
                        *(pucData++) = (ucCurrentTsId&0xFF);
                        *(pucData++) = (usOriginalNetworkId>>8);
                        *(pucData++) = (usOriginalNetworkId&0xFF);

                        usFullDescriptorLength = 0;
                        pucDescriptorLength = pucData;
                        pucData += 2;

                        ucModulation = (unsigned char) std::stoi(json_network_details["list"][ts_cout]["modulation"].asString());
                        ucModulation+=1;
                        uiFrequency = (unsigned int) std::stoi(json_network_details["list"][ts_cout]["frequency"].asString());
                        uiSymbol_rate = ((unsigned int) std::stoi(json_network_details["list"][ts_cout]["symbol_rate"].asString()) / 1000);
                        uiSymbol_rate = uiSymbol_rate*100;
                        // printf("--------TS Start HERE--------> %d ----->> %d ----------------- >> %d \n", ts_cout,ucCurrentTsId,uiFrequency);
                        //Add cable_delivery descriptor
                        usDescriptorLength = setCableDeliverySystemDescriptor(pucData,uiFrequency,ucFec_Outer,ucModulation,uiSymbol_rate,ucFec_Inner);
                        usFullDescriptorLength += usDescriptorLength;
                        pucData += usDescriptorLength;
                            //printf("-------CABLE DELE DESC LEN--------->%d\n", usDescriptorLength);

                            Json::Value json_active_progs = db->getActivePrograms(output,rmx_no);
                            // std::cout<<json_active_progs<<endl;
                            if(json_active_progs["error"]==false){
                                uiNbServices =(unsigned int) json_active_progs["list"].size();
                                int iServiceCount = 0;
                                pucServiceId =(short unsigned int*) new short[uiNbServices];
                                pucServiceType =(short unsigned int*) new short[uiNbServices];
                                for (int iServiceCount = 0; iServiceCount < uiNbServices; ++iServiceCount)
                                {
                                    pucServiceId[iServiceCount] = (unsigned short)std::stoi(json_active_progs["list"][iServiceCount]["service_id"].asString()); 
                                    pucServiceType[iServiceCount] = 1;//Get the service type from getProg INFO
                                }
                                //Add Service List Descriptor
                                usDescriptorLength = setServiceListDescriptor(pucData, uiNbServices, pucServiceId,pucServiceType);
                                usFullDescriptorLength += usDescriptorLength;
                                pucData += usDescriptorLength;
                            }

                            // // Add LCN Descriptor
                            Json::Value json_prog_lcn_ids = db->getLcnIds(output,rmx_no);
                            // std::cout<<json_prog_lcn_ids<<endl;
                            if(json_prog_lcn_ids["error"]==false){
                                uiNbServices =(unsigned int) json_prog_lcn_ids["list"].size();
                                int iLcnCount = 0;
                                pucServiceLcnId =(short unsigned int*) new short[uiNbServices];
                                pucChannelNumber =(short unsigned int*) new short[uiNbServices];
                                for (int iLcnCount = 0; iLcnCount < uiNbServices; ++iLcnCount)
                                {
                                    pucServiceLcnId[iLcnCount] = (unsigned short)std::stoi(json_prog_lcn_ids["list"][iLcnCount]["service_id"].asString());  
                                    pucChannelNumber[iLcnCount] = (unsigned short)std::stoi(json_prog_lcn_ids["list"][iLcnCount]["lcn_id"].asString());
                                }
                                usDescriptorLength = setLcnDescriptor(pucData, uiNbServices, pucServiceId, pucChannelNumber, pucHdChannelNumber);
                                usFullDescriptorLength += usDescriptorLength;
                                pucData += usDescriptorLength;
                            }

                            //END OF Descriptors
                            *(pucDescriptorLength++) =  0xF0 | (usFullDescriptorLength>>8);
                            *(pucDescriptorLength++) =  (usFullDescriptorLength&0xFF);
                      
                        usFullSectionLength += usFullDescriptorLength+6;
                        if(usFullSectionLength>900){
                            // printf("%d---- SECTION Full------------>%d\n ", ts_cout,usFullSectionLength);
                            pucData = pucData-(usFullDescriptorLength+6);
                            usFullSectionLength = usFullSectionLength - (usFullDescriptorLength+6);
                           
                            break;
                        }
                             usFullTSLength += usFullDescriptorLength+6;
                    }
                    *(pucTSLength++) =  0xF0 | (usFullTSLength>>8);
                    *(pucTSLength++) =  (usFullTSLength&0xFF);

                    //add section len size
                    usFullSectionLength += TS_SECTION_LENB2;
                    usFullSectionLength += CRC32B4;

                    *(pucSectionLength++) =  0xF0 | ((usFullSectionLength)>>8);
                    *(pucSectionLength++) =  ((usFullSectionLength)&0xFF);
                   
                    int j = 0;
                  
                    usFullSectionLength +=NIT_TAG_SEC_LEN3;
                    printf("\n");
                    unsigned char * ucSectionPayload = pucNewSection;
                   
                    if(ts_cout==ts_size)
                        break;

                    section_count++;
                }
                
            }
        return section_count;
    }
	/*****************************************************************************/
    /*  Command    function insertNITable                          */
    /*****************************************************************************/
    void reBootScript(const Rest::Request& request, Net::Http::ResponseWriter response){
        Json::Value json;
        Json::FastWriter fastWriter;
        runBootupscript();
        json["error"]= false;
        json["message"]= "Executed bootup script!";
        
        std::string resp = fastWriter.write(json);
        response.send(Http::Code::Ok, resp);
    }
    void runBootupscript(){
        Json::Value json,NewService_names,network_details,lcn_json,high_prior_ser,pmt_alarm_json,active_progs,locked_progs,freeca_progs,input_mode_json,fifo_flags,table_ver_json,table_timeout_json,dvb_output_json,psisi_interval,serv_provider_json,nit_mode;
        printf("\n\n Downloding Mxl 1 \n");
        downloadMxlFW(1,0);
        usleep(100);
        printf("\n\n Downloding Mxl 2 \n");
        downloadMxlFW(2,0);
        usleep(100);
        printf("\n\n Downloding Mxl 3 \n");
        downloadMxlFW(3,0);
        usleep(100);
        printf("\n\n Downloding Mxl 4 \n");
        downloadMxlFW(4,0);
        usleep(100);
        printf("\n\n Downloding Mxl 5 \n");
        downloadMxlFW(5,0);
        usleep(100);
        printf("\n\n Downloding Mxl 6 \n");
        downloadMxlFW(6,0);
        printf("\n\n MXL Downlod Completed! \n\n");
        usleep(100);
        Json::Value jsonAllegro = db->getConfAllegro();
        if(jsonAllegro["error"] == false){
            for(int i=0;i<jsonAllegro["list"].size();i++ ){
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | ((((std::stoi(jsonAllegro["list"][i]["mxl_id"].asString()))+6)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int address = std::stoi(jsonAllegro["list"][i]["address"].asString());
                    callReadAllegro(address);
                    usleep(500000);
                    confAllegro8297(address,std::stoi(jsonAllegro["list"][i]["enable1"].asString()),std::stoi(jsonAllegro["list"][i]["volt1"].asString()),std::stoi(jsonAllegro["list"][i]["enable2"].asString()),std::stoi(jsonAllegro["list"][i]["volt2"].asString()));
                    usleep(1000);
                }
                
            }   
        }
        
        Json::Value jsonTuner = db->getRFTunerDetails();
        if(jsonTuner["error"] == false){
            for(int i=0;i<jsonTuner["list"].size();i++ ){
                int rmx_no = std::stoi(jsonTuner["list"][i]["rmx_no"].asString());
                int mxl_id = std::stoi(jsonTuner["list"][i]["mxl_id"].asString());
                int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | (((mxl_id+6)&0xF)<<1) | (0&0x1);
                if(connectI2Clines(target)){
                    tuneMxl(std::stoi(jsonTuner["list"][i]["demod_id"].asString()),std::stoi(jsonTuner["list"][i]["lnb_id"].asString()),std::stoi(jsonTuner["list"][i]["dvb_standard"].asString()),std::stoi(jsonTuner["list"][i]["frequency"].asString()),std::stoi(jsonTuner["list"][i]["symbol_rate"].asString()),std::stoi(jsonTuner["list"][i]["modulation"].asString()),std::stoi(jsonTuner["list"][i]["fec"].asString()),std::stoi(jsonTuner["list"][i]["rolloff"].asString()),std::stoi(jsonTuner["list"][i]["pilots"].asString()),std::stoi(jsonTuner["list"][i]["spectrum"].asString()),std::stoi(jsonTuner["list"][i]["lo_frequency"].asString()),mxl_id,rmx_no,0);
                    usleep(1000);
                    setMpegMode(std::stoi(jsonTuner["list"][i]["demod_id"].asString()),1,MXL_HYDRA_MPEG_CLK_CONTINUOUS,MXL_HYDRA_MPEG_CLK_IN_PHASE,104,MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG,1,1,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_ACTIVE_HIGH,MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE,MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED);
                    usleep(1000);
                }
            }   
        }
        
        Json::Value jsonIPTuner = db->getIPTunerDetails();
        if(jsonIPTuner["error"] == false){
            for(int i=0;i<jsonIPTuner["list"].size();i++ ){
                int rmx_no = std::stoi(jsonIPTuner["list"][i]["rmx_no"].asString());
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    int channel_no = std::stoi(jsonIPTuner["list"][i]["input_channel"].asString());
                    int controller_of_rmx = (rmx_no % 2 == 0)?1:0;
                    int ch_no = ((controller_of_rmx)*8)+channel_no;
                    Json::Value json = callSetEthernetIn(ch_no,std::stoul(jsonIPTuner["list"][i]["ip_address"].asString()),std::stoi(jsonIPTuner["list"][i]["port"].asString()),std::stoi(jsonIPTuner["list"][i]["type"].asString()));
                    if(json["error"] == false)
                        std::cout<<"-------------------IP Input success------------------------------- "<<std::endl;
                    else
                        std::cout<<"--------------------IP Input FAILED-------------------------------- "<<std::endl;
                }
                usleep(1000);
            }   
        }
        usleep(1000);
        //Setting Input tuner type for each input channels (RF or IP IN)
        for (int control_fpga = 1; control_fpga <= 3; ++control_fpga)
        {
            int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1){
                int tuner_ch = db->getTunerChannelType(control_fpga);
                if(write32bI2C(4,0,tuner_ch) == -1){
                    std::cout<<"Failed to configure MUX IN!"<<std::endl;
                }else{
                    std::cout<<"-------------------Success MUX IN!--------------------------------------"<<std::endl;
                }
            }
            usleep(1000);
        }
        
        for (int mux_id = 1; mux_id <= 3; ++mux_id)
        {   
            Json::Value jsonIPTuner = db->getSPTSIPTuners(mux_id);
            int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((mux_id-1)&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1){
                if(jsonIPTuner["error"] == false){
                    for(int i=0;i<jsonIPTuner["list"].size();i++ ){
                         Json::Value json = callSetEthernetIn(std::stoi(jsonIPTuner["list"][i]["channel_no"].asString()),std::stoul(jsonIPTuner["list"][i]["ip_address"].asString()),std::stoi(jsonIPTuner["list"][i]["port"].asString()),std::stoi(jsonIPTuner["list"][i]["type"].asString()));
                        std::cout<<"-------------------SPTS IP IN!--------------------------------------"<<jsonIPTuner["list"][i]["ip_address"].asString()<<std::endl;
                    }
                }
            }
            usleep(1000);
        }

        Json::Value jsonSPTSControl = db->getSPTSControl();
        if(jsonSPTSControl["error"] == false){
            for(int i=0;i<jsonSPTSControl["list"].size();i++ ){
                int mux_id = std::stoi(jsonSPTSControl["list"][i]["mux_id"].asString());
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((mux_id-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    unsigned long custom_line =std::stoul(jsonSPTSControl["list"][i]["custom_line"].asString());
                    int is_enable =std::stoi(jsonSPTSControl["list"][i]["is_enable"].asString());
                    if(is_enable == 1)
                    {
                        custom_line = custom_line | 65536;
                    }
                    if(write32bI2C(5,0,custom_line) == -1){
                        std::cout<<"-------------------Failed MUX OUT!"<<std::endl;
                    }
                    std::cout<<"-------------------SPTS IP(getSPTSControl)!--------------------------------------"<<custom_line<<std::endl;
                }
            }
        }
        usleep(1000);
        Json::Value jsonIPOUT = db->getIPOutputChannels();
        if(jsonIPOUT["error"] == false){
            std::cout<<jsonIPOUT["list"].size()<<std::endl;
            for(int i=0;i<jsonIPOUT["list"].size();i++ ){
                int rmx_no = std::stoi(jsonIPOUT["list"][i]["rmx_no"].asString());
                int channel_no  = std::stoi(jsonIPOUT["list"][i]["output_channel"].asString());
                int port  = std::stoi(jsonIPOUT["list"][i]["port"].asString());
                int rmx_no_of_controller = (rmx_no % 2 == 0)?1:0;
                int ch_no = ((rmx_no_of_controller)*8)+channel_no;
                unsigned long int ip_address = std::stoul(jsonIPOUT["list"][i]["ip_address"].asString());
                int control_fpga =ceil(double(rmx_no)/2);
                int target =((0&0x3)<<8) | ((0&0x7)<<5) | (((control_fpga-1)&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1){
                    std::cout<<ch_no<<port<<ip_address;
                    Json::Value jsonipout = callSetEthernetOut(ip_address,port,ch_no);
                    if(jsonipout["error"] == false){
                         std::cout<<"IP OUT Success"<<std::endl;
                    }else{
                        std::cout<<"IP OUT FAILED"<<std::endl;
                    }
                }
            }
        }

        usleep(1000);
        // Json::Value jsonRfauth = db->getRFauthorizedRmx();
        // int rmx_no =0;
        // if(jsonRfauth["error"] == false){
        //     std::cout<<jsonRfauth["list"].size()<<std::endl;
        //     for(int i=0;i<jsonRfauth["list"].size();i++ ){
        //          rmx_no = std::stoi(jsonRfauth["list"][0].asString());
        //     }
        // }
        RFauthorization(1);
        usleep(100);
        RFauthorization(63);

        for (int rmx = 1; rmx <= RMX_COUNT; rmx++)
        {
            for(int input=0;input<4;input++){
                Json::Value iojson =callGetProgramList(input,rmx);
                if(iojson["error"] == false)
                    std::cout<<"-----------Services has been restored from channel RMX "<<rmx<<">---------------->CH "<<input<<std::endl;
            }
        }
        usleep(1000);
        for (int i = 0; i <= RMX_COUNT; ++i)
        {
            for (int j = 0; j <= INPUT_COUNT; ++j)
            {
                for (int k = 0; k <= OUTPUT_COUNT; ++k)
                {
                    active_progs = db->getActivePrograms("-1",std::to_string(j),std::to_string(k),std::to_string(i+1));                    
                    if(active_progs["error"]==false){
                        // std::cout<<active_progs["list"].size();
                        Json::Value json = callSetKeepProg(active_progs["list"],std::to_string(j),std::to_string(k),i+1);
                        if(json["error"]==false){
                            std::cout<<"------------------Active Prog has been restored--------------------- "<<std::endl;
                        }
                    }   
                }
            }
        }
        
        usleep(100);
        NewService_names = db->getServiceNewnames();
        if(NewService_names["error"]==false){
            for (int i = 0; i < NewService_names["list"].size(); ++i)
            {
                Json::Value json = callSetServiceName(NewService_names["list"][i]["channel_name"].asString(),NewService_names["list"][i]["channel_number"].asString(),NewService_names["list"][i]["input_channel"].asString(),std::stoi(NewService_names["list"][i]["rmx_no"].asString()));
                if(json["error"]==false){
                    std::cout<<"------------------ Service name's restored successfully ------------------"<<std::endl;
                }
            }    
        }
        usleep(100);
        for (int rmx = 1; rmx <= RMX_COUNT; ++rmx)
        {
            for (int input = 0; input < INPUT_COUNT; ++input)
            {

                Json::Value new_service_ids,old_service_ids,NewService_ids;
                NewService_ids = db->getServiceIds(rmx,input);
                // std::cout<<NewService_ids<<endl;
                if(NewService_ids["error"]==false){
                    Json::Value iojson=callSetInputOutput(std::to_string(input),"0",rmx);
                    if(iojson["error"] == false){
                        for (int i = 0; i < NewService_ids["list"].size(); ++i)
                        {
                            new_service_ids[i] = NewService_ids["list"][i]["service_id"].asString();
                            old_service_ids[i] = NewService_ids["list"][i]["channel_number"].asString();
                        }   
                        Json::Value json = callSetServiceID(old_service_ids,new_service_ids,rmx);
                        if(json["error"]==false){
                            std::cout<<"------------------ Service ID's restored successfully ------------------"<<std::endl;
                        } 
                    }else{
                        std::cout<<"------------------ "<<iojson["message"].asString()<<"------------------"<<std::endl;
                    }
                }    
            }
        }
        usleep(100);
        network_details = db->getNetworkDetails();
        if(network_details["error"]==false){
            for (int i = 0; i < network_details["list"].size(); ++i)
            {
                if(network_details["list"][i]["ts_id"].asString()!="-1"){
                    Json::Value tsJason;
                    tsJason = callSetTsId(network_details["list"][i]["ts_id"].asString(),network_details["list"][i]["network_id"].asString(),network_details["list"][i]["original_netw_id"].asString(),network_details["list"][i]["output"].asString(),std::stoi(network_details["list"][i]["rmx_no"].asString()));  
                    if(tsJason["error"]==false){
                        std::cout<<"------------------TS details has been restored--------------------- "<<std::endl;
                    }
                }
                if(network_details["list"][i]["network_name"].asString()!="-1"){
                    Json::Value setNetJson;
                    setNetJson = callSetNetworkName(network_details["list"][i]["network_name"].asString(),network_details["list"][i]["output"].asString(),std::stoi(network_details["list"][i]["rmx_no"].asString()));
                    if(setNetJson["error"]==false){
                        std::cout<<"------------------Network name has been restored--------------------- "<<std::endl;
                    }
                }

            }
        } 
        usleep(100);
        Json::Value json_cetnter_frequency = db->getCenterFrequency();
        if(json_cetnter_frequency["error"] == false){
            for (int i = 0; i < json_cetnter_frequency["list"].size(); ++i)
            {
                Json::Value json = callsetCenterFrequency(json_cetnter_frequency["list"][i]["center_frequency"].asString(),json_cetnter_frequency["list"][i]["rmx_no"].asString());
                if(json["error"]==false){
                    std::cout<<"------------------ Center Frequency has been restored --------------------- "<<std::endl;
                }   
            }
        }
        usleep(100);
        Json::Value json_sr = db->getSymbolRates();
        if(json_sr["error"] == false){
            for (int i = 0; i < json_sr["list"].size(); ++i)
            {
                Json::Value json = callSetFSymbolRate(json_sr["list"][i]["rmx_no"].asString(),json_sr["list"][i]["output"].asString(),json_sr["list"][i]["roll_off"].asString(),json_sr["list"][i]["symbol_rate"].asString());
                if(json["error"]==false){
                    std::cout<<"------------------ Symbol rate has been restored --------------------- "<<std::endl;
                }   
            }
        }
        usleep(100);
        lcn_json = db->getLcnNumbers();
        if(lcn_json["error"]==false){
            for (int i = 0; i < lcn_json["list"].size(); ++i)
            {
                Json::Value json = callSetLcn(lcn_json["list"][i]["program_number"].asString(),lcn_json["list"][i]["channel_number"].asString(),"0",std::stoi(lcn_json["list"][i]["rmx_no"].asString()),lcn_json["list"][i]["input_channel"].asString(),1);
                if(json["error"]==false){
                    std::cout<<"------------------LCN Numbers has been restored--------------------- "<<std::endl;
                }
            }
        }
        usleep(100);
        high_prior_ser = db->getHighPriorityServices();
        if(high_prior_ser["error"]==false){
            std::cout<<"------------------====="<<high_prior_ser["list"].size()<<std::endl;
            for (int i = 0; i < high_prior_ser["list"].size(); ++i)
            {
                //std::cout<<high_prior_ser["list"][i]["program_number"].asString()<<std::endl;
                Json::Value json = callSetHighPriorityServices(high_prior_ser["list"][i]["program_number"].asString(),high_prior_ser["list"][i]["input_channel"].asString(),1);
                if(json["error"]==false){
                    std::cout<<"------------------High priority services has been restored--------------------- "<<std::endl;
                }
            }
        }
        usleep(100);
        pmt_alarm_json = db->getPmtalarm();
        if(pmt_alarm_json["error"]==false){
            
            for (int i = 0; i < pmt_alarm_json["list"].size(); ++i)
            {
                std::cout<<pmt_alarm_json["list"].size()<<std::endl;
                Json::Value json = callSetPmtAlarm(pmt_alarm_json["list"][i]["program_number"].asString(),pmt_alarm_json["list"][i]["alarm_enable"].asString(),pmt_alarm_json["list"][i]["input"].asString(),std::stoi(pmt_alarm_json["list"][i]["rmx_no"].asString()));
                //std::cout<<"------------------PMT alarm call--------------------- "<<json["message"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------PMT alarm has been restored--------------------- "<<pmt_alarm_json["list"][i]["input"].asString()<<std::endl;
                }
            }
        }

        usleep(100);
        std::cout<<"-------------------------END PMT-------------------------------"<<std::endl;
        locked_progs = db->getLockedPids();
        if(locked_progs["error"]==false){
            for (int i = 0; i < locked_progs["list"].size(); ++i)
            {
                Json::Value json = callSetLockedPIDs(locked_progs["list"][i]["program_number"].asString(),locked_progs["list"][i]["input_channel"].asString(),std::stoi(locked_progs["list"][i]["rmx_no"].asString()));
                if(json["error"]==false){
                    std::cout<<"------------------Locked services has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------END Locking Pid-------------------------------"<<std::endl;

        usleep(100);
        freeca_progs = db->getFreeCAModePrograms();
        if(freeca_progs["error"]==false){

            for (int i = 0; i < freeca_progs["list"].size(); ++i)
            {
                Json::Value json = callSetEraseCAMod(freeca_progs["list"][i]["program_number"].asString(),freeca_progs["list"][i]["input_channel"].asString(),freeca_progs["list"][i]["output_channel"].asString(),std::stoi(freeca_progs["list"][i]["rmx_no"].asString()));
                std::cout<<"-------------------------"<<json["message"]<<freeca_progs["list"][i]["output_channel"].asString()<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------Free CA has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------Free CA END-------------------------------"<<std::endl; 

        usleep(100);
        input_mode_json = db->getInputMode();
        if(input_mode_json["error"]==false){

            for (int i = 0; i < input_mode_json["list"].size(); ++i)
            {
                Json::Value json = callSetInputMode(input_mode_json["list"][i]["is_spts"].asString(),input_mode_json["list"][i]["pmt"].asString(),input_mode_json["list"][i]["sid"].asString(),input_mode_json["list"][i]["rise"].asString(),input_mode_json["list"][i]["master"].asString(),input_mode_json["list"][i]["in_select"].asString(),input_mode_json["list"][i]["bitrate"].asString(),input_mode_json["list"][i]["input"].asString(),std::stoi(input_mode_json["list"][i]["rmx_no"].asString()));
                //std::cout<<"-------------------------"<<json["message"]<<input_mode_json["list"][i]["input"].asString()<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------INPUT mode has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------INPUT mode END-------------------------------"<<std::endl; 
        
        usleep(100);
        Json::Value lcn_provider = db->getLcnProviderId();
        if(lcn_provider["error"]==false){
            Json::Value json = callSetLcnProvider(lcn_provider["provider_id"].asString(),std::stoi(lcn_provider["rmx_no"].asString()));
            //std::cout<<"-------------------------"<<json["message"]<<std::endl;
            if(json["error"]==false){
                std::cout<<"------------------LCN provider id has been restored--------------------- "<<json["message"]<<std::endl;
            }
        }
        std::cout<<"-------------------------LCN provider id END-------------------------------"<<std::endl;
        
        usleep(100);
        fifo_flags = db->getAlarmFlags();
        if(fifo_flags["error"]==false){
            for (int i = 0; i < fifo_flags["list"].size(); ++i)
            {
                Json::Value json = callCreateAlarmFlags(fifo_flags["list"][i]["level1"].asString(),fifo_flags["list"][i]["level2"].asString(),fifo_flags["list"][i]["level3"].asString(),fifo_flags["list"][i]["level4"].asString(),fifo_flags["list"][i]["mode"].asString(),std::stoi(fifo_flags["list"][i]["mode"].asString()));
                //std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------Creating alarm flags has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------Creating alarm flags END-------------------------------"<<std::endl;

        usleep(100);
        table_ver_json = db->getTablesVersions();
        if(table_ver_json["error"]==false){
            for (int i = 0; i < table_ver_json["list"].size(); ++i)
            {
                Json::Value json = callSetTablesVersion(table_ver_json["list"][i]["pat_ver"].asString(),table_ver_json["list"][i]["pat_isenable"].asString(),table_ver_json["list"][i]["sdt_ver"].asString(),table_ver_json["list"][i]["sdt_isenable"].asString(),table_ver_json["list"][i]["nit_ver"].asString(),table_ver_json["list"][i]["nit_isenable"].asString(),table_ver_json["list"][i]["output"].asString(),std::stoi(table_ver_json["list"][i]["rmx_no"].asString()));
                //std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------Table versions has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------Table versions END-------------------------------"<<std::endl;
        
        usleep(100);
        nit_mode = db->getNITmode();
        if(nit_mode["error"]==false){
            for (int i = 0; i < nit_mode["list"].size(); ++i)
            {
            	 Json::Value iojson=callSetInputOutput("0",nit_mode["list"][i]["output"].asString(),std::stoi(nit_mode["list"][i]["rmx_no"].asString()));
                if(iojson["error"]==false){
                	if(std::stoi(nit_mode["list"][i]["mode"].asString()) != 1){
						Json::Value json = callSetNITmode(nit_mode["list"][i]["mode"].asString(),nit_mode["list"][i]["output"].asString(),std::stoi(nit_mode["list"][i]["rmx_no"].asString()));
		                //std::cout<<"-------------------------"<<json["message"]<<std::endl;
			            if(json["error"]==false){
		                    std::cout<<"------------------NIT mode has been restored for output--------------------- "<<nit_mode["list"][i]["output"]<<std::endl;
		                }
		            }
				}else{
                    json["error"]= true;
                    json["message"]= "Error while selecting output!";     
                }
                
            }
        }

        usleep(100);
        table_timeout_json = db->getNITtableTimeout();
        if(table_timeout_json["error"]==false){
            for (int i = 0; i < table_timeout_json["list"].size(); ++i)
            {
                Json::Value json = callSetTableTimeout(table_timeout_json["list"][i]["table_id"].asString(),table_timeout_json["list"][i]["timeout"].asString(),std::stoi(table_timeout_json["list"][i]["rmx_no"].asString()));
                //std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------NIT table timeout has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        std::cout<<"-------------------------NIT table timeout END-------------------------------"<<std::endl;

        usleep(100);
        dvb_output_json = db->getDVBspiOutputMode();
        if(dvb_output_json["error"]==false){
            for (int i = 0; i < dvb_output_json["list"].size(); ++i)
            {
                Json::Value json = callSetDvbSpiOutputMode(dvb_output_json["list"][i]["bit_rate"].asString(),dvb_output_json["list"][i]["falling"].asString(),dvb_output_json["list"][i]["mode"].asString(),dvb_output_json["list"][i]["output"].asString(),std::stoi(dvb_output_json["list"][i]["rmx_no"].asString()));
                //std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------DVB output mode has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }

        usleep(100);
        std::cout<<"-------------------------DVB output mode END-------------------------------"<<std::endl;
        psisi_interval = db->getPsiSiInterval();
        if(psisi_interval["error"]==false){
            for (int i = 0; i < psisi_interval["list"].size(); ++i)
            {
                Json::Value json = callSetPsiSiInterval(psisi_interval["list"][i]["pat_int"].asString(),psisi_interval["list"][i]["sdt_int"].asString(),psisi_interval["list"][i]["nit_int"].asString(),psisi_interval["list"][i]["output"].asString(),std::stoi(psisi_interval["list"][i]["rmx_no"].asString()));
                //std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------PSI SI Interval has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }

        std::cout<<"-------------------------PSI SI Interval END-------------------------------"<<std::endl;

        usleep(100);
        serv_provider_json = db->getNewProviderName();
        if(serv_provider_json["error"]==false){
            for (int i = 0; i < serv_provider_json["list"].size(); ++i)
            {
                Json::Value json = callSetServiceProvider(serv_provider_json["list"][i]["service_name"].asString(),serv_provider_json["list"][i]["service_number"].asString(),serv_provider_json["list"][i]["input_channel"].asString(),std::stoi(serv_provider_json["list"][i]["rmx_no"].asString()));
               // std::cout<<"-------------------------"<<json["error"]<<std::endl;
                if(json["error"]==false){
                    std::cout<<"------------------Provider names has been restored--------------------- "<<json["message"]<<std::endl;
                }
            }
        }
        updateCATCADescriptor("-1");
        
        usleep(100);
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
        	Json::Value json_qam_ids = db->getQAM(rmx_no);
        	std::cout<<json_qam_ids["error"].asString()<<std::endl;
	        if(json_qam_ids["error"] == false){
	    		int target =((0&0x3)<<8) | (((rmx_no-1)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
                if(write32bCPU(0,0,target) != -1) {
                	for (int i = 0; i < json_qam_ids["list"].size(); ++i)
                	{
                		int qam_id =std::stoi(json_qam_ids["list"][i]["qam_id"].asString());
                		if(qam_id != 2){
		                	int output_channel = std::stoi(json_qam_ids["list"][i]["output_channel"].asString());
		                    int value = ((qam_id&0xF)<<1) | (0&0x0);
		                    json = callSetCore(std::to_string(DVBC_OUTPUT_CS[output_channel]),std::to_string(QAM_ADDR),std::to_string(value),rmx_no);
		                    if(json["error"] == false)
		                		std::cout<<"----------------QAM ADDED! -----------------------"<<std::endl;    	
		                }

                	}
                	
                }else{
                    std::cout<<"----------------Error while connection! -----------------------"<<std::endl;
                }    	
	        }
        }
       
       usleep(100);
        // //Initializing DVBCSA to the core
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
        	callSetInputOutput("0","0",rmx_no);
        	// std::cout <<rmx_no<<std::endl;
        	callinitCsa(rmx_no);
        }

        usleep(100);
        // //Updating CA Descriptor to the PMT 
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
        	 for (int input = 0; input <= OUTPUT_COUNT; ++input)
	        {
	        	Json::Value auth_outputs,ecm_pids,JSON_CA_System_id,service_pids,private_data_list;
	        	Json::Value JSON_ecm_desc = db->getECMDescriptors(std::to_string(rmx_no),std::to_string(input));
		        // std::cout<<"-------------------------------------------------------------------------------";
		        if (JSON_ecm_desc["error"] == false)
		        {
		        	for (int i = 0; i < JSON_ecm_desc["ca_system_ids"].size(); ++i)
		        	{

		                std::string system_id = JSON_ecm_desc["ca_system_ids"][i].asString();

		                system_id = system_id.substr(2,system_id.length()-6);
		                // std::cout<<system_id;
		        		JSON_CA_System_id.append(std::to_string(getHexToLongDec(system_id)));
		        		auth_outputs.append("255");
		        		service_pids.append(JSON_ecm_desc["service_pids"][i].asString());

		        		ecm_pids.append(JSON_ecm_desc["ecm_pids"][i].asString());

		        	}
		        	json = callSetPMTCADescriptor(service_pids,JSON_CA_System_id,ecm_pids,private_data_list,rmx_no,std::to_string(input),std::to_string(input));
		        	if(json["error"] == false){
		           		std::cout<<"-----------------------Successfully Added ECM Descriptor!----------------------------";
		           	}
		           	else{
		           		std::cout<<"-----------------------Failed to add ECM Descriptor!----------------------------";
		           	}
		        	// Json::Value customPid = callsetCustomPids(ecm_pids,auth_outputs,rmx_no,std::to_string(output));
		         //   	if(customPid["error"] == false){
		         //   		std::cout<<"-----------------------Successfully enabled ECM PID!----------------------------";
		         //   	}
		           	// else{
		           	// 	std::cout<<"-----------------------Failed to enabled ECM PID!----------------------------";
		           	// }
		        }
	        }
        }

        usleep(100);
       //Initializing CUSTOM PIDS to the core
        for (int rmx_no = 1; rmx_no <= RMX_COUNT; ++rmx_no)
        {
            for (int output = 0; output < OUTPUT_COUNT; ++output)
            {
                Json::Value json_pids = db->getCustomPids(std::to_string(rmx_no));
                if(json_pids["error"] == false){
                    json = callsetCustomPids(json_pids["pids"],json_pids["output_auths"],rmx_no,std::to_string(output));
                }else{
                    json["error"]= true;
                    json["message"]= "No record found!";
                }
            }

        }

        usleep(100);
   //      //Insert NIT insertion 
        Json::Value json_nit = db->getNITDetails();
       	if(json_nit["error"] == false){
            if(json_nit["mode"] == "2"){
           		std::string networkid = json_nit["network_id"].asString();
    			std::string networkName =json_nit["network_name"].asString();
                Json::Value json_nit =  callInsertNITable(networkid,networkName);
                if(json_nit["error"] == false)
                    std::cout<<"-----------------------NIT Updated successfully!----------------------------";
                else
                    std::cout<<"-----------------------Error Updating NIT!       ----------------------------";
            }else{
                std::cout<<"----------------------NIT in disabled mode!----------------------------";
            }
       	}
       	else{
       		std::cout<<"-----------------------NO NIT!----------------------------";
       	}

        usleep(100);
       	//Intialize the RMX_FPGA's
        int remx_bit[]= {0,4,1,5,2,6};
        for (int rmx_no = 0; rmx_no < RMX_COUNT; ++rmx_no)
        {
            int target =((0&0x3)<<8) | (((rmx_no)&0x7)<<5) | ((0&0xF)<<1) | (0&0x1);
            if(write32bCPU(0,0,target) != -1) {
			Json::Value core_json = callSetCore("0","0",std::to_string(remx_bit[rmx_no]),rmx_no+1);
		    		if(core_json["status"] == 1)
						std::cout<<remx_bit[rmx_no]<<std::endl;
            }
		}
	
        std::cout<<"----------------END OF EMMG INITIALIZATION! -----------------------"<<std::endl;

        std::cout<<"-------------------------Provider names END-------------------------------"<<std::endl;

        std::cout<<"-------------------------END of Bootup Scrip-------------------------------------"<<std::endl;
    }
    
   
    std::string getCurrentTime()
    {
        std::time_t result = std::time(0);
        std::stringstream ss;
        ss << result;
        return ss.str();
    }

    int getReminderOfTime(int stream_time){
        int curr_time=std::stoi(getCurrentTime())+CW_MQ_TIME_DIFF;
        int diff = curr_time - stream_time;
        return(diff%10);
    }

    std::string generatorRandCW()
    {
        mt19937 mt_rand(time(0));
        std::string rnd = std::to_string(mt_rand());
        long in;
        while(1){
            // std::cout<<"---------------------------CONTINUE-------------------"<<std::endl;
            in=mt_rand()+(rand() % 100 + 1);
            if((16-rnd.length())>(std::to_string(in).length())) continue;
            break;
        }
        return rnd + (std::to_string(in)).substr((std::to_string(in).length()-(16-rnd.length())),std::to_string(in).length());
    }


    std::string getParameter(std::string postData,std::string key){
        if(postData.length() !=0 && key.length() != 0)
        {
            postData="&"+postData+"&";
            key="&"+key+"=";
            std::size_t found = postData.find(key);
            if (found!=std::string::npos){
                return (postData.substr(found+key.length(),postData.length())).substr(0,postData.substr(found+key.length(),postData.length()).find("&"));
            }
        }
        return "";
    } 
    bool is_numeric(const std::string& str){
        return !str.empty() && std::find_if(str.begin(),str.end(),[](char c){
            return !std::isdigit(c);
        })==str.end();
    }
    std::string validateRequiredParameter(std::string postData,std::string *para,int len){
         postData="&"+postData+"&";
         std::string req_fiels="Required fields ";
         bool error=false;
         for (int i = 0; i < len; ++i)
         {
            std::string value,key;
            key="&"+para[i]+"=";
            std::size_t found = postData.find(key);
            if (found==std::string::npos){
                error=true;
                req_fiels+=para[i]+",";
            }
         }
         if (error==true)
         {
            req_fiels =req_fiels.substr(0, req_fiels.size()-1);
            return req_fiels+" are missing";
         }
        return "0";
    }
    bool verifyInteger(const std::string& string,int len=0,int isExactLen=0,int range=0,int llimit=0){  
        if(is_numeric(string)){
            if(len!=0){
                if(isExactLen){
                    if(string.length() == len){
                        if(range!=0){
                            if(std::stoi(string)<=range && std::stoi(string)>=llimit)
                                return true;
                            else
                                return false;
                        }
                        else{
                            return true;
                        }
                    }else{
                        return false;
                    }
                }else{
                    if(string.length() <= len){
                        if(range!=0){
                            if(std::stoi(string)<=range && std::stoi(string)>=llimit)
                                return true;
                            else
                                return false;
                        }
                        else{
                            return true;
                        }
                    }else{
                        return false;
                    }
                }
            }else{
                if(range!=0){
                    if(std::stoi(string)<=range && std::stoi(string)>=llimit)
                        return true;
                    else
                        return false;
                }
                else{
                    return true;
                }
            }
        }else{
            return false;
        }
    }
    bool verifyString(std::string var,int len=0,int isExactLen=0){
        std::istringstream iss(var);
        int num=0;
        if((iss >> num).fail()){
            if(len!=0){
                if(isExactLen){
                    if(var.length() == len){
                        return true;
                    }else{
                        return false;
                    }
                }else{
                    if(var.length() <= len){
                        return true;
                    }else{
                        return false;
                    }
                }
            }
            return true;
        }else{
            return false;
        }
    }
    bool verifyPort(std::string port){
          if(is_numeric(port)){
            if(1024 < std::stoi(port) <= 65535){
                return true;
            }else{
                return false;
            }
          }else
            return false;
    }
    bool verifyBool(std::string var){
        if(var=="true" || var=="false"){
            return true;
        }else{
            return false;
        }   
    }
    bool isValidIpAddress(const char *ipAddress)
    {
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
        return result != 0;
    }
    //Function to verify json array
    bool verifyJsonArray(Json::Value jsonArray,std::string key,int data_type){
        bool valid_datatypes=true;
        if(data_type == 1){
            if(!jsonArray[key].isNull())
            {
                int len =jsonArray[key].size();
                if(data_type==1){//Integer datatypes
                    for(int i=0;i<len;i++){
                        if(!verifyInteger(jsonArray[key][i].asString()))
                            valid_datatypes=false;
                    }
                }else if(data_type==2){//string datatypes
                    for(int i=0;i<len;i++){
                        if(!verifyString(jsonArray[key][i].asString()))
                            valid_datatypes=false;
                    }
                }
            }else{
                valid_datatypes = false;
            }
        }
        // std::cout<<"===="<<valid_datatypes<<std::endl;
        return valid_datatypes;
    }

    bool verifyMACAddress(std::string mac_addr,int len) {
        char tempBuff[1024];
        strcpy(tempBuff, mac_addr.c_str());
        char *arr_mac=tempBuff  ;
        for(int i = 0; i < len; i++) {
            if(i % 3 != 2 && !isxdigit(arr_mac[i]))
                return false;
            if(i % 3 == 2 && arr_mac[i] != ':')
                return false;
        }
        if(arr_mac[len] != '\0')
            return false;
        return true;
    }

    bool verifyIsHex(std::string const& hex,int range=0)
    {
        if(range > 0){
            // std::cout<<"M here "<<hex.size()<<std::endl;
            return (hex.compare(0, 2, "0x") == 0
              && hex.size() > 2
              && hex.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos) && hex.size() <= range;
        }else{
            return (hex.compare(0, 2, "0x") == 0
              && hex.size() > 2
              && hex.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos);
        }
    }
    std::string UriDecode(const std::string & sSrc)
    {
       // Note from RFC1630: "Sequences which start with a percent
       // sign but are not followed by two hexadecimal characters
       // (0-9, A-F) are reserved for future extension"
      const char HEX2DEC[256] = 
      {
          /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
          /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
          
          /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          
          /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          
          /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
          /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
      }; 

       const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
       const int SRC_LEN = sSrc.length();
       const unsigned char * const SRC_END = pSrc + SRC_LEN;
       // last decodable '%' 
       const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

       char * const pStart = new char[SRC_LEN];
       char * pEnd = pStart;

       while (pSrc < SRC_LAST_DEC)
       {
          if (*pSrc == '%')
          {
             char dec1, dec2;
             if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
                && -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
             {
                *pEnd++ = (dec1 << 4) + dec2;
                pSrc += 3;
                continue;
             }
          }

          *pEnd++ = *pSrc++;
       }

       // the last 2- chars
       while (pSrc < SRC_END)
          *pEnd++ = *pSrc++;

       std::string sResult(pStart, pEnd);
       delete [] pStart;
       return sResult;
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

    std::string getDecToHex(int dec_num)
    {
        std::stringstream ss;
        ss<< std::hex << dec_num;
        std::string res (ss.str());
        return res;
    }
    std::string getlongDecToHex(long unsigned int dec_num)
    {
        std::stringstream ss;
        ss<< std::hex << dec_num;
        std::string res (ss.str());
        return res;
    }
    long unsigned int getHexToLongDec(std::string hex_str)
    {
        long unsigned int val;
        // std::stringstream ss;
        // ss<< std::hex << dec_num;
        // std::string res (ss.str());
        std::stringstream ss;
        ss << std::hex << hex_str;
        ss >> val;
        return val;
    }
    std::string hex_to_string(const std::string& input)
    {
        static const char* const lut = "0123456789abcdef";
        size_t len = input.length();
        if (len & 1) throw std::invalid_argument("odd length");

        std::string output;
        output.reserve(len / 2);
        for (size_t i = 0; i < len; i += 2)
        {
            char a = input[i];
            const char* p = std::lower_bound(lut, lut + 16, a);
            if (*p != a) throw std::invalid_argument("not a hex digit");

            char b = input[i + 1];
            const char* q = std::lower_bound(lut, lut + 16, b);
            if (*q != b) throw std::invalid_argument("not a hex digit");

            output.push_back(((p - lut) << 4) | (q - lut));
        }
        return output;
    }

    void addToLog(std::string fname,std::string msg){
        fname="\n-------------------------------------------------------------\nFunction Name:- "+fname+"\n";
        const char * char_fname = fname.c_str();
        const char * char_msg = msg.c_str();
        FILE * pFile;
        pFile = fopen ("RMX_LOG.txt","a+");
        if (pFile!=NULL)
        {
            fputs (char_fname,pFile);
            fputs (char_msg,pFile);
            fputs ("\n-------------------------------------------------------------\n",pFile);
            fclose (pFile);
        } 
    }
    class Metric {
    public:
        Metric(std::string name, int initialValue = 1)
            : name_(std::move(name))
            , value_(initialValue)
        { }

        int incr(int n = 1) {
            int old = value_;
            value_ += n;
            return old;
        }

        int value() const {
            return value_;
        }

        std::string name() const {
            return name_;
        }
    private:
        std::string name_;
        int value_;
    };

    typedef std::mutex Lock;
    typedef std::lock_guard<Lock> Guard;
    Lock metricsLock;
    std::vector<Metric> metrics;

    std::shared_ptr<Net::Http::Endpoint> httpEndpoint;
    Rest::Router router;
    
    static void* cwProvision(void *args)
    {
         StatsEndpoint *_this=static_cast<StatsEndpoint *>(args);
        _this->sendCWProvision();
    }
    static void* spawnChannel(void *args)
    {
        // struct arg_struct t = (struct arg_struct)args;
         StatsEndpoint *_this=static_cast<StatsEndpoint *>(args);
        _this->createChannel(_this->channel_ids,_this->supercas_id,_this->ecm_port,_this->ecm_ip);
       int s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
       if (s != 0)
        std::cout<<"THEAD CANCEL S ------------------------------------------ FAILED"<<std::endl;
    }
    static void* spawnEMMChannel(void *args)
    {
        // struct arg_struct t = (struct arg_struct)args;
         StatsEndpoint *_this=static_cast<StatsEndpoint *>(args);
        _this->createEMMChannel(_this->channel_ids,_this->client_id,_this->data_id,_this->emm_port,_this->emm_bw,_this->stream_id);
    }
    void sendCWProvision()
    {
        while(1){
            addCWQueue();
            usleep(1000000); 
        }
    }
    void createChannel(int channel_ids,std::string supercas_id,int ecm_port,std::string ecm_ip)
    {  
        std::cout<<"------------------channel- "+std::to_string(channel_ids)+" created---"+std::to_string(ecm_port)+"------------"<<std::endl;
        // int pid=fork();
        // if(pid==0)
        //     execl("test",std::to_string(channel_ids).c_str(),supercas_id.c_str(),std::to_string(ecm_port).c_str(),ecm_ip.c_str(),NULL);
    }
    void createEMMChannel(int channel_ids,std::string client_id,int data_id, int emm_port,int emm_bw,int stream_id)
    {  
        std::cout<<"------------------EMM channel- "+std::to_string(channel_ids)+"EMM CH created---"+std::to_string(emm_port)+"------------"<<std::endl;
        // int pid=fork();
        // if(pid==0)
        //     execl("test",std::to_string(channel_ids).c_str(),client_id.c_str(),std::to_string(data_id).c_str(),std::to_string(emm_port).c_str(),NULL);
    }

    
};
int StatsEndpoint::CW_THREAD_CREATED=0;



int main(int argc, char *argv[]) {

    std::string config_file_path;
    Net::Port port(9080);
    int thr = 2;
    if (argc >= 2) {
        config_file_path =(std::string)argv[1];
    }else{
        config_file_path = "config.conf";
    }
    // else if(argc){
    //     config_file_path =(std::string)argv[1];
        std::cout<<config_file_path<<"---------------------------------"<<std::endl; 
    // }
    Net::Address addr(Net::Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads=== "<< endl;

    
    StatsEndpoint stats(addr);
    
    stats.init(thr,config_file_path);
   
    stats.start();
    // cout << "M here \n"  << endl;
    stats.shutdown(); 
}
