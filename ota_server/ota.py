##sudo pip install --upgrade pip

#import tftpy

#server = tftpy.TftpServer('./')
#server.listen('0.0.0.0', 69) 


#pip install dyntftpd
from dyntftpd.server import *
server = TFTPServer('0.0.0.0',69,'./')
server.serve_forever()