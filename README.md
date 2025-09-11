c版本mapperframework


source ~/modbus-venv/bin/activate
ss -ltnp | grep ':1502' || nohup python /home/zhang/modbus_server.py > ~/modbus_server.log 2>&1 &


MYSQL_PWD=123456 mysql -h 127.0.0.1 -u mapper -D testdb