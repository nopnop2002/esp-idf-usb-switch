# esp-idf-usb-switch ON/OFF by timer
Turning USB ON/OFF using ESP-IDF.   

# Configuration
```
git clone https://github.com/nopnop2002/esp-idf-usb-switch
cd esp-idf-rc-switch/timer
idf.py menuconfig
idf.py flash
```

![Image](https://github.com/user-attachments/assets/3a74f1f2-926d-45be-8a1c-225e9b502dad)
![Image](https://github.com/user-attachments/assets/2394cfb1-537c-4f05-b8e7-d772864c7fe4)


# Wirering
|Transmitter Module||ESP32|
|:-:|:-:|:-:|
|DATA|--|GPIO5|
|GND|--|GND|
|VCC|--|3.3V|

# Typical circuit
![Image](https://github.com/user-attachments/assets/e784ee89-77f6-41f4-a515-87160c520f98)
