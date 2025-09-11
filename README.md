c版本mapperframework


source ~/modbus-venv/bin/activate
ss -ltnp | grep ':1502' || nohup python /home/zhang/modbus_server.py > ~/modbus_server.log 2>&1 &


MYSQL_PWD=123456 mysql -h 127.0.0.1 -u mapper -D testdb


kubectl -n default patch device demo-1 --type='json' -p='[
  {"op":"replace","path":"/spec/properties/1/desired/value","value":"74"}
]'