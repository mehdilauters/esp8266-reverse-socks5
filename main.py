import socket
from threading import Thread
import select

sockets = []
fwds = []
buffer_size = 4096

class Forwarder(Thread):
  def __init__(self, sock1, sock2):
    print "==forward=="
    Thread.__init__(self)
    sock1.send('192.168.1.104:22')
    self.sock1 = sock1
    self.sock2 = sock2
    
  def run(self):
    while True:
      inputready, outputready, exceptready = select.select([self.sock1, self.sock2],[],[])
      for socket in inputready:
        if socket == self.sock1:
          data = self.sock1.recv(buffer_size)
          if len(data) == 0:
            self.sock2.close()
            self.sock1.close()
            return
          
          self.sock2.send(data)
          
        if socket == self.sock2:
          data = self.sock2.recv(buffer_size)
          if len(data) == 0:
            self.sock2.close()
            self.sock1.close()
            return
          self.sock1.send(data)
      
    

class Server(Thread):
  allow_reuse_address = True
  def __init__(self, address):
    Thread.__init__(self)
    self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    self.server.bind(address)
    self.server.listen(200)
  
  def run(self):
    while True:
      conn, addr = self.server.accept()
      print 'Connected by', addr
      sockets.append(conn)
      if len(sockets) % 2 == 0:
        f = Forwarder(sockets[-2], sockets[-1])
        f.start()
        fwds.append(f)
        
      #while 1:
          #data = conn.recv(1024)
          #if not data: break
          #conn.sendall(data)
      #conn.close()
  

if __name__ == "__main__":

    # Create the server, binding to localhost on port 9999
    server = Server(('0.0.0.0', 6668))
    server.start()
    
    proxy = Server(('0.0.0.0', 6669))
    proxy.start()
    
    server.join()
    proxy.join()