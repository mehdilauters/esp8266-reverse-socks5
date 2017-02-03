#Esp8266-reverse-socks5

Reverse http proxy, reverse tcp, reverse socks5 proxy based on the esp8266.

Once you know the wifi credential, this projects aims to setup a reverse tcp backdoor.

It can be hidden in a dropable box or any usb powered devices

![blackbox open](https://github.com/mehdilauters/esp8266-wifiScanMap/raw/master/doc/blackbox_open.png)  ![blackbox](https://github.com/mehdilauters/esp8266-wifiScanMap/raw/master/doc/usb.png)

Uses the esp8266 with a known wifi access point to build a reverse socks5 tunnel.

 - Setup your esp-open-rtos path in the makefile
 - Flash and start the esp: ````make flash````
 - wait it creates a configuration access point
 - connect to [http://172.16.0.1/](http://172.16.0.1/)
 - configure wifi and home server
 - do not forget to fill the ````Security check```` with the serial id as returned by ````sdk_system_get_chip_id()````
 - reboot

## Socks5 proxy

On the server side:
 ````python main.py -s -r 8888 -l 8889````
 
 And then connect to localhost:8889 with a socks5 client to access the remote network.
 For ssh, it would be:
 
 ````ssh -v -o ProxyCommand='nc -X 5 -x 127.0.0.1:8889 %h %p' 192.168.x.x```` # remote local machine

## HTTP proxy

 On the server side:
 
 ````python main.py -p -r 8888 -l 8889````
 
 ````HTTP_PROXY="127.0.0.1:8889" wget http://hackaday.com````
 
## Raw TCP socket

 On the server side:


 ````python main.py -r 8888 -l 8889````
 
 ````echo "hello from server" | nc localhost 8889````
 