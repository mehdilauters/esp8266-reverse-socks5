#Esp8266-reverse-socks5

Uses the esp8266 with a known wifi access point to build a reverse socks5 tunnel.

 - Flash and start the esp
 - wait it creates a configuration access point
 - configure wifi and home server
 - reboot

Then on the server side, start the socks5 proxy
 - ````python main.py -s -r 8888 -l 8889````
 
 And then connect to localhost:8889 with a socks5 client to access the remote network.
 For ssh, it would be:
 ````ssh -v -o ProxyCommand='nc -X 5 -x 127.0.0.1:8889 %h %p' 192.168.x.x```` # remote local machine
