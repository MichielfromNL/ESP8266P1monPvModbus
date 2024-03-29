# ESP8266-P1mon-PV-Modbus

A Wemos D1 mini to collect PV data and generate S0 pulses for P1mon 

P1mon  ( https://www.ztatz.nl/ ) is a Raspberry PI project that reads smart meter P1-port data, and makes beautifull graphs, stats and overviews of energy usage and production. For the production part it  can read PV data from solaredge, or calculate it from S0 pules from a separate Kwh meter. See https://www.ztatz.nl/kwh-meter-met-s0-meting/ for an explanation.

The challenge: I don't have a S0 KWH meter for my PV output, neither a Solaredge-compatible converter. My converter only has a TCP/IP modbus interface for the PV output and all sorts of other data..
P1mon is completely written in Python. So the logical thing is: make some Python code to read the modbus and enter the PV data straight into the P1mon database. But the problem is: p1mon is distributed and installed as a single PI image. To add something to it requires patching the P1mon installation after each update. Which involves changing startup files to start the PV daemon, or patch the code, et cetera. I chose that option for some time, but ran into problems: for example, when the modbus reads fail, or the Python daemon gets stuck for some reason, no PV data is recorded until I discover what went wrong and then a part of my PV history (production data) is already lost forever.

So I decided to switch to this approach: use a WEMOS D1, embed it in the PI case, connect it to a PI GPIO. And make a sketch that reads PV modbus data and convert it to S0 pulses on the PI GPIO. In this way the P1mon installation remains untouched. I can now control the modbus reads from the WiFi ESP, and make that as resilient as I want.
An additional advantage is that other devices in my network can now also read the PV data. For that purpose I added a simple JSON web interface on the ESP. With this setup there is only 1 modbus reader in my nework, and there are no issues with conflicts or need for locking when multiple devices would try to read the MOdbus simultaneously. It is also much easier to read the PV data.

The Wemos is embedded in the PI case, with 2 plastic spacers and some superglue.
![Overzicht](https://user-images.githubusercontent.com/80706499/162152017-5d35b2ba-c220-49d5-91f4-9ff5049de672.jpg)
![Montage](https://user-images.githubusercontent.com/80706499/162152091-15ab0248-f15e-4c5b-803f-92fe24e45866.jpg)
The wiring:
![J1 GPIO P3](https://user-images.githubusercontent.com/80706499/162152147-dae0eb5f-651e-4fd1-a4cd-8a8abf5f6cbb.jpg)
- J1 - 4 (5v) to Wemos 5V
- J1 - 6 (Gnd) to Wemos Gnd
- J1 - 3 (GPIO2) to Wemos D2
- Optional: reset. , J1 - 5 (GPIO3) to Wemos Rst. Which can be used to reset the ESP from the PI commandline. But sofar I have not succeeded in getting that working.

## Custom modbus class
The energy data-fetch code is of course very much dependent on how  the (local) PV datalogger works. That is why Energy.ino is a class, the init & get nmethods must be adapted to what the logger (or remote site) can provide. Mine is a wattsonic, but others (such as Growatt shinelan) require a completely different data fetch approach.
Consult your PV solution's manuals to find out what to do.

That's all, the sketch is in the Src. Feel free to copy and use, have fun!




