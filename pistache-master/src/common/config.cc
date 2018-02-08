#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
	std::string Config :: trim(const std::string& str)
	{
	    size_t first = str.find_first_not_of(' ');
	    if (std::string::npos == first)
	    {
	        return str;
	    }
	    size_t last = str.find_last_not_of(' ');
	    return str.substr(first, (last - first + 1));
	}
	int Config :: readConfig(std::string config_file_path)
	{
	   	char config[10];
	  	std::ifstream in("config.conf");
	  	if(!in) {
		    std::cout << "Cannot open config file.\n";
		    return 1;
	  	}

	  	char str[255];
	  	int i=0;
	  	while(in) {
		    in.getline(str, 255);  // delim defaults to '\n'
		    std::string line(str);
		    if(in){
		      // config[i] ="s";
		      	i++;
		       	std::size_t found = line.find("=");
	            if (found!=std::string::npos){
		              std::string key;
		              key=trim(line.substr(0,found));
	              	  std::string value= trim(line.substr(found+1,line.length()));
		              if(key=="db_name")
		                DB_NAME = value;
		              else if(key=="db_host")
		                DB_HOST = value;
		              else if(key=="db_user")
		                DB_USER = value;
		              else if(key=="db_pass")
		                DB_PASS = value;
		              else if(key=="redis_host")
		                REDIS_IP = value;
		              else if(key=="redis_pass")
		                REDIS_PASS = value;
		              if(key=="redis_port")
		                REDIS_PORT = std::stoi(value);
	            }
		   	} 
	  	}
	  in.close();
	  return 0;
	}
