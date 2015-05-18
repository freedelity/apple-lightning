This program can be used to hijack the communication between an iDevice
and a MFI accessory in order to communicate with an iOS app using
any UART interface.

This is intended to run on an Arduino Mega but other AVR boards with 3 UART can be used.

 - **RX1** should be connected to **Data1** pin of the iDevice's lightning port.
 - **TX1** should be connected to **Data2** pin of the iDevice's lightning port.
 - **RX2** should be connected to **Data2** pin of the accessory's lightning connector.
 - **TX2** should be connected to **Data1** pin of the accessory's lightning connector.
 - **RX0/TX0** is connected to the Arduino USB interface and should be connected to the UART of the custom device.

**The code is not written with the Arduino IDE. Here are instructions
for compiling and uploading the code:**

 - For Arduino Mega:
```
avr-gcc -DF_CPU=16000000UL -mmcu=atmega2560 -Os -o serial_mitm.o serial_mitm.c
avr-objcopy -O ihex -R .eeprom serial_mitm.o serial_mitm.hex
avrdude -V -F -c stk500v2 -p m2560 -b 115200 -P /dev/ttyACM0 -U flash:w:serial_mitm.hex
```

Then, you can interact with the iOS app by sending bytes in the Arduino USB interface.

On Linux, use this commands to configure the serial interface: (assuming the Arduino is connected as /dev/ttyACM0)
```
stty -F /dev/ttyACM0 57600 cs8 raw -echo
``` 

Then, you can read bytes with this command:
````cat /dev/ttyACM0```

And send bytes with this one:
```echo "Some text" > /dev/ttyACM0```
