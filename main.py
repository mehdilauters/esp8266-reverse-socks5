import argparse
import socket
from threading import Thread
import select
import re
import socks5


socks5_server = None
sockets = []
fwds = []
buffer_size = 4096

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("-r", "--remote", help="remote port")
  parser.add_argument("-l", "--local", help="local port")
  parser.add_argument("-t", "--target", help="host to connect CAUTION, port is mandatory (ie 192.168.1.1:80)")
  parser.add_argument("-p", "--proxy", action='store_true', help="proxy mode")
  parser.add_argument("-s", "--socks5", action='store_true', help="socks5 proxy mode")
  return parser.parse_args()
  

class TcpForwarder(Thread):
  def __init__(self, sock1, sock2, host = '', port = ''):
    Thread.__init__(self)
    self.sock1 = sock1
    self.sock2 = sock2
    self.target_host = host
    self.target_port = port
    self.initial_data = ''
    
  def tunnelling(self, host, port, initial_data = ''):
    self.target_host = host
    self.target_port = port
    self.initial_data = initial_data
    
  def run(self):
    print "forwarding to %s:%s"%(self.target_host, self.target_port)
    self.sock1.send('%s:%s'%(self.target_host, self.target_port))
    self.sock1.send(self.initial_data)
    while True:
      inputready, outputready, exceptready = select.select([self.sock1, self.sock2],[],[])
      for socket in inputready:
        if socket == self.sock1:
          data = self.sock1.recv(buffer_size)
          if len(data) == 0:
            print "Closing"
            self.sock2.close()
            self.sock1.close()
            return
          
          self.sock2.send(data)
          
        if socket == self.sock2:
          data = self.sock2.recv(buffer_size)
          if len(data) == 0:
            print "Closing"
            self.sock2.close()
            self.sock1.close()
            return
          self.sock1.send(data)
  
class HttpForwarder(TcpForwarder):
  def __init__(self, sock1, sock2):
    TcpForwarder.__init__(self, sock1, sock2)
    host, port, data = self.parse_request(sock2.recv(buffer_size))
    self.tunnelling(host, port, data)
  
  def parse_request(self, _data):
    host = None
    port = None
    data = ''
    res = re.match('(GET) http:\/\/([^:\/]+):?(\d*)?(.*)\sHTTP', _data)
    if res is not None:
      action, host, port, uri = res.groups()
      if port == '':
        port = 80
      data += "%s %s HTTP/1.0\n"%(action,uri)
      # parse headers
      lines = _data.split("\r\n")
      for line in lines[1:]:
        res = re.match('.*Keep-Alive', line)
        if res is None:
          data += "%s\r\n"%line
    return (host, port, data)

class TcpServer(Thread):
  allow_reuse_address = True
  def __init__(self, args, address):
    Thread.__init__(self)
    self.args = args
    self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    print "Listening on %s:%s"%address
    self.server.bind(address)
    self.server.listen(200)

class Server(TcpServer):
  def run(self):
    while True:
      conn, addr = self.server.accept()
      print 'Connected by', addr
      if socks5_server is not None:
        socks5_server.set_proxy_socket(conn)
      else:
        sockets.append(conn)
        

class Proxy(TcpServer):
  def run(self):
    while True:
      conn, addr = self.server.accept()
      print 'Connected by', addr
      if len(sockets) % 2 == 0:
        conn.close()
        print 'refused'
      else:
        sockets.append(conn)
        if len(sockets) % 2 == 0:
          if self.args.proxy:
            f = HttpForwarder(sockets[-2], sockets[-1])
          else:
            host, port = args.target.split(':')
            f = TcpForwarder(sockets[-2], sockets[-1], host, port)
          f.start()
          fwds.append(f)

def main(args):
  global socks5_server
  # Create the server, binding to localhost on port 9999
  server = Server(args, ('0.0.0.0', args.remote))
  server.start()
  
  if args.socks5:
    print "starting socks5 server on %s"%args.local
    socks5.Socks5Server.allow_reuse_address = True
    auth = False
    allowed_ips = None
    user_manager = socks5.UserManager()
    socks5_server = socks5.Socks5Server(args.local, auth, user_manager, allowed=allowed_ips)
    socks5_server.serve_forever()
  else:
    proxy = Proxy(args, ('0.0.0.0', args.local))
    proxy.start()
  
  server.join()
  proxy.join()

if __name__ == "__main__":
  args = parse_args()
  if args.remote is not None:
    args.remote = int(args.remote)
  else:
    args.remote = 6668

  if args.local is not None:
    args.local = int(args.local)
  else:
    args.local = 6669
    
  main(args)