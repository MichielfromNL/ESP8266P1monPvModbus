#ESP8266P1monPvModbus

A Wemos D1 mini to collect PV data and generate S0 pulses for P1mon 

P1mon  ( https://www.ztatz.nl/ ) is a Raspberry PI that reads smart meter P1 port data, and makes beautifull graphs, stats and overviews of energy data.  
It can read PV watts from solaredge, or from S0 pules from a seperate meter. See https://www.ztatz.nl/kwh-meter-met-s0-meting/

The challenge: I don't have a S0 KWH meter vor my PV output, and neither do I have a solaredge-compatible converter. My converterhas a TCP/IP modbus interface with the PV output and all sorts of other data.

P1mon is completely written in Python. So the logical thing would be: make some Python ocde to read the modbus, and enter the PV data straight into the P1mon database.
But the problem is: p1mon is delivered and installed as a single PI image. To add somthing to it requires patching the P1mon installation after each update. Which involves changing startup files to start a daemon, or patch code, et cetera. I still chose that option for some time, but ran into problems:  when the modbus reads fail, or the Python daemon gets stuck (e.g. when the converter restarts, gets another IP address from DHCP et cetera), no PV data is recorded until I discover what goes wrong, and then a part of my PV history (production data) is lost forever. 

So I decided for this approach: a a WEMOS D1 , embedded in the PI case that reads PV modbus data and converts it to S0 pulses on a PI GPIO. The P1mon installation is untouched, vanilla, and I can control the modbus reads and make it as resilient as I want.

An additional advantage is that other devices can read the PV data easier. For that I made a simple JSON web interface on the ESP, so that there is 1 modbus reader (no issues with locking of conflicting reads) and other devices can also read the PV data much easier   

The Wemos is embedded in the PI case, with 2 plastic spacers. 
![Overzicht](https://user-images.githubusercontent.com/80706499/162152017-5d35b2ba-c220-49d5-91f4-9ff5049de672.jpg)
![Montage](https://user-images.githubusercontent.com/80706499/162152091-15ab0248-f15e-4c5b-803f-92fe24e45866.jpg)
The wiring:
![J1 GPIO P3](https://user-images.githubusercontent.com/80706499/162152147-dae0eb5f-651e-4fd1-a4cd-8a8abf5f6cbb.jpg)
J1 - 4 (5v) to Wemos 5V
J1 - 6 (Gnd) to Wemos Gnd
J1 - 3 (GPIO12) to Wemos D2
Optional: reset. , J1 - 5 (GPIO3) to Wemos Rst. Which can be used to reset the ESP from the PI commandline

I used two spacers and superglue to fix the Wemos in the case.


That's all, the sketch is in the Src. Feel free to copy and use



