# IR-wifi-password
Sending wifi SSID/password over IR-serial protocol .

This is sketch for sending wifi SSID/password over IR.

Idea is to "pair" wifi devices by putting them really close together so they can 
exchange wifi ssid/password using infrared. 

so far no code but use cases :
* "pairing" any two esp8266 , so no need to connect them to computer to update wifi stuff. 
* "pairing" any other devices, like headphones. It is better than QR code because it does not require camera, just IR led. 
etc.

TODO: everything :D
close-proximity IR is "secure" enough that it can blast in cleartext, f.e. using 1200bps serial over IR "protocol".
this is sure shot for one-way advertising devices. 
It really has much room for improvement, like rolling code, symmetric encryption (f.e. using same shared codebook)
and much much more depending on use. f.e. unlike static printed qr code it allows generating different wifi password 
for each client wishing to connect to wireless AP. 

For me most usefull will be code merely allowing esp8266 to log into another esp8266 softAP without need to use 
anything else than esp's themselves, so i can have portable wireless vt100 terminal which i can connect to anything in my lab 
without need to type anything. 
