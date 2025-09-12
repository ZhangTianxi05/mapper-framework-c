c版本mapperframework


source ~/modbus-venv/bin/activate
ss -ltnp | grep ':1502' || nohup python /home/zhang/modbus_server.py > ~/modbus_server.log 2>&1 &


MYSQL_PWD=123456 mysql -h 127.0.0.1 -u mapper -D testdb


kubectl -n default patch device demo-1 --type='json' -p='[
  {"op":"replace","path":"/spec/properties/1/desired/value","value":"74"}
]'


http


export PUBLISH_METHOD=http
export PUBLISH_CONFIG='{"endpoint":"http://127.0.0.1:8080/ingest","method":"POST"}'


import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
class H(BaseHTTPRequestHandler):
    def do_POST(self):
        l = int(self.headers.get('Content-Length','0'))
        body = self.rfile.read(l) if l>0 else b''
        print("POST", self.path, body.decode('utf-8', 'ignore'))
        self.send_response(200); self.end_headers()
        self.wfile.write(b"ok")
    def do_PUT(self): self.do_POST()
addr = sys.argv[1] if len(sys.argv)>1 else '0.0.0.0'
port = int(sys.argv[2]) if len(sys.argv)>2 else 8080
print(f"Listening on http://{addr}:{port}")
HTTPServer((addr,port), H).serve_forever()

python3 /home/zhang/http_sink.py 0.0.0.0 8080

mqtt
mosquitto_sub -h 127.0.0.1 -p 1885 -t 'kubeedge/device/#' -v -d


export PUBLISH_METHOD=mqtt
export PUBLISH_CONFIG='{"brokerUrl":"127.0.0.1","port":1885,"clientId":"mapper_client","topicPrefix":"kubeedge/device","qos":1,"keepAlive":60}'
./build/main ./config.yaml


otel

export PUBLISH_METHOD=otel
export PUBLISH_CONFIG='{"endpoint":"http://127.0.0.1:4318/v1/metrics","serviceName":"mapper-c"}'

import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
class H(BaseHTTPRequestHandler):
    def do_POST(self):
        l = int(self.headers.get('Content-Length','0'))
        body = self.rfile.read(l) if l>0 else b''
        print("POST", self.path, body.decode('utf-8', 'ignore'))
        self.send_response(200); self.end_headers()
        self.wfile.write(b"ok")
    def do_PUT(self): self.do_POST()
addr = sys.argv[1] if len(sys.argv)>1 else '0.0.0.0'
port = int(sys.argv[2]) if len(sys.argv)>2 else 4318
print(f"Listening on http://{addr}:{port}")
HTTPServer((addr,port), H).serve_forever()

python3 /home/zhang/http_sink.py 0.0.0.0 4318