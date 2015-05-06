This program can be used to sniff the bytes exchanged between
an iDevice and a MFI accessory during the identification process on
the ACC_ID pin.

This is intended to run on an Arduino but other AVR boards can be used.

The ACC_ID pin must be connected to the pin PD3 of the board
(Digital pin 3 of the Arduino Uno, or digital pin 18 of the Arduino Mega).
But any other digital pin could be used, you just have to modify the code for that.

**The code is not written with the Arduino IDE. Here are instructions
for compiling and uploading the code:**

 - For Arduino Uno:
```
avr-gcc -DF_CPU=16000000UL -mmcu=atmega328p -Os -o acc_id_sniff.o acc_id_sniff.c
avr-objcopy -O ihex -R .eeprom acc_id_sniff.o acc_id_sniff.hex
avrdude -V -F -c arduino -p m328p -b 115200 -P /dev/ttyACM0 -U flash:w:acc_id_sniff.hex
```

 - For Arduino Mega:
```
avr-gcc -DF_CPU=16000000UL -mmcu=atmega2560 -Os -o acc_id_sniff.o acc_id_sniff.c
avr-objcopy -O ihex -R .eeprom acc_id_sniff.o acc_id_sniff.hex
avrdude -V -F -c stk500v2 -p m2560 -b 115200 -P /dev/ttyACM0 -U flash:w:acc_id_sniff.hex
```
