This program can be used to send bytes on the ACC_ID pin of the iDevice
in order to identify as a genuine lightning controller and open the data
bus and power line.

This is intended to run on an Arduino but other AVR boards can be used.

Use the [acc_id_sniff](../acc_id_sniff/) program to sniff the bytes sent
by a genuine controller and copy them in the *response* array in *acc_id_spoof.c*
(leaving 0xFF bytes for the bytes sent by the iDevice).

The ACC_ID pin must be connected the the pin PD3 of the board
(Digital pin 3 of the Arduino Uno, or digital pin 18 of the Arduino Mega).
But any other digital pin could be used, you just have to modify the code for that.

**The code is not written with the Arduino IDE. Here are instructions
for compiling and uploading the code:**

 - For Arduino Uno:
```
avr-gcc -DF_CPU=16000000UL -mmcu=atmega328p -Os -o acc_id_spoof.o acc_id_spoof.c
avr-objcopy -O ihex -R .eeprom acc_id_spoof.o acc_id_spoof.hex
avrdude -V -F -c arduino -p m328p -b 115200 -P /dev/ttyACM0 -U flash:w:acc_id_spoof.hex
```

 - For Arduino Mega:
```
avr-gcc -DF_CPU=16000000UL -mmcu=atmega2560 -Os -o acc_id_spoof.o acc_id_spoof.c
avr-objcopy -O ihex -R .eeprom acc_id_spoof.o acc_id_spoof.hex
avrdude -V -F -c stk500v2 -p m2560 -b 115200 -P /dev/ttyACM0 -U flash:w:acc_id_spoof.hex
```
