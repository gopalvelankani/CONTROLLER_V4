#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

class Config{
   	public:
   	std::string DB_NAME,DB_HOST,DB_USER,DB_PASS,REDIS_IP,REDIS_PASS;
	int REDIS_PORT;
	std::string trim(const std::string& str);
	int readConfig(std::string config_file_path);
};