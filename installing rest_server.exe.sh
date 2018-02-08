//cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..

sudo apt-get update
sudo apt-get install apache2
sudo apt-get install mysql-server-5.6
sudo apt-get install  php5-mysql
sudo apt-get install  phpmyadmin

sudo apt-get install libjsoncpp-dev
sudo apt-get install libmysqlclient-dev
sudo apt-get install libhiredis-dev
sudo apt-get install redis-server

edit apache config file.

add fallowing line to /etc/apache2/apache2.conf at the end of the file. 
Include /etc/phpmyadmin/apache.conf






