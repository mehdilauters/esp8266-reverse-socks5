import argparse
import socket
from threading import Thread
import select
import re

sockets = []
fwds = []
buffer_size = 4096

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("-r", "--remote", help="remote port")
  parser.add_argument("-l", "--local", help="local port")
  parser.add_argument("-t", "--target", help="host to connect CAUTION, port is mandatory (ie 192.168.1.1:80)")
  parser.add_argument("-p", "--proxy", action='store_true', help="proxy mode")
  return parser.parse_args()
  

class Forwarder(Thread):
  def __init__(self, args, sock1, sock2):
    self.args = args
    Thread.__init__(self)
    if not args.proxy:
      print "forwarding %s to %s"%(args.local, args.target)
      sock1.send(args.target)
    else:
      request = self.parse_request(sock2.recv(buffer_size))
      print "proxying %s"%request
      sock1.send(request)
    self.sock1 = sock1
    self.sock2 = sock2
    
  def parse_request(self, _data):
    data = ''
    res = re.match('(GET) http:\/\/([^:\/]+):?(\d*)?(.*)\sHTTP', _data)
    if res is not None:
      action, host, port, uri = res.groups()
      data = '%s'%host
      if port == '':
        port = 80
      data += ':%s'%port
      data += "\n"
      
      data += "%s %s HTTP/1.0\n\n"%(action,uri)
      # parse headers
      lines = _data.split("\r\n")
      for line in lines[1:]:
        res = re.match('.*Keep-Alive', line)
        if res is None:
          data += "%s\r\n"%line
    print data
    return data
      
    
  def run(self):
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
          f = Forwarder(self.args, sockets[-2], sockets[-1])
          f.start()
          fwds.append(f)

def main(args):
  
  # Create the server, binding to localhost on port 9999
  server = Server(args, ('0.0.0.0', args.remote))
  server.start()
  
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