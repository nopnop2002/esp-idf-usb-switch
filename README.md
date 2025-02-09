# esp-idf-usb-switch
Turning USB ON/OFF using ESP-IDF.   

We can obtain products that turn USB on/off with a remote control about $4.   
Both of these use a frequency of 433MHz radio signal.   
Using these, you can turn on/off the USB device from the ESP32.   
![Image](https://github.com/user-attachments/assets/88d1a4d8-c98b-44c4-b589-0b053ff2534b)


# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
This project supports these wireless transceivers.   

- Wireless transmitter   
H34A-315/H34A-433   
SYN115   
STX882   

- Wireless receiver   
H3V3/H3V4   
SYN480R   
LR43B/LR33B   
SRX882/SRX887   

- Wireless RF Remote Control Switch   

I used these transceivers.
![H3V4F-2](https://user-images.githubusercontent.com/6020549/125154180-c36bdc00-e193-11eb-9e89-5934c2db7fae.JPG)
![H3V4F-3](https://user-images.githubusercontent.com/6020549/125154246-0ded5880-e194-11eb-974b-03232166ab12.JPG)


# How to use
First, do teaching.   
The teaching results are stored in NVS.   
Turn the USB ON/OFF using the teaching results.   

# Teaching
```
git clone https://github.com/nopnop2002/esp-idf-usb-switch
cd esp-idf-usb-switch/teaching
idf.py menuconfig
idf.py flash
```

## Configuration
![Image](https://github.com/user-attachments/assets/cb6fb520-e9eb-47b0-9246-33516a4a5e9d)
![Image](https://github.com/user-attachments/assets/65c98e54-c370-470d-a14b-f5fcb31ed772)


## Wirering
|Receiver Module||ESP32|
|:-:|:-:|:-:|
|DATA|--|GPIO4|
|GND|--|GND|
|VCC|--|3.3V|

## Typical circuit
![Image](https://github.com/user-attachments/assets/f185c4a0-90ab-4232-b97a-7e03921ae8a1)

## Start teaching
At this timing, press the ON button on the remote control.   
![Image](https://github.com/user-attachments/assets/942728d0-ae7e-48ac-b827-6270c3a87930)   

At this timing, press the OFF button on the remote control.   
![Image](https://github.com/user-attachments/assets/61a7b880-2af8-4051-8ef7-5b0ec154ca35)

# Re-teaching
Run this project again.   
Previous information will be overwritten.   

# ON/OFF by timer
Read [this](https://github.com/nopnop2002/esp-idf-usb-switch/tree/main/timer).   

# ON/OFF by cron
Read [this](https://github.com/nopnop2002/esp-idf-usb-switch/tree/main/cron).   

