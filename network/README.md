# ON/OFF by network
Turning USB ON/OFF using HTTP/MQTT.   
This project requires a WiFi connection.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-usb-switch
cd esp-idf-usb-switch/network
idf.py menuconfig
idf.py flash
```

# Configuration
![Image](https://github.com/user-attachments/assets/b158b4a6-d93c-4f16-9a8b-8a730bf7f045)
![Image](https://github.com/user-attachments/assets/a089a1d9-6513-4f3e-a4b2-73492b3cf229)

## WiFi Setting   
![Image](https://github.com/user-attachments/assets/6eab8920-0677-41bf-b817-9f870185ba4b)

You can connect using the mDNS hostname instead of the IP address.   
![Image](https://github.com/user-attachments/assets/b98ff986-9b92-407f-a790-a55865249930)

You can use static IP.   
![Image](https://github.com/user-attachments/assets/87a5935d-c0d8-4337-9f33-205907574472)

## Network protocol Setting
Using HTTP   
![Image](https://github.com/user-attachments/assets/92db655f-2fcc-4fb2-9e0e-267cd99e0452)

Using MQTT   
![Image](https://github.com/user-attachments/assets/bb8a0ec5-49d3-4f2b-8617-451d436208b4)

## RF Setting   
Set the information of transmitter module.   
![Image](https://github.com/user-attachments/assets/ce2a2fed-7393-439e-a2d9-353a8e538712)

# Wirering
|Transmitter Module||ESP32|
|:-:|:-:|:-:|
|DATA|--|GPIO5|
|GND|--|GND|
|VCC|--|3.3V|

# Typical circuit
![Image](https://github.com/user-attachments/assets/e784ee89-77f6-41f4-a515-87160c520f98)


# API for HTTP

- turn on   
```curl -X POST -H "Content-Type: application/json" http://esp32-server.local:8080/api/on```

- turn off   
```curl -X POST -H "Content-Type: application/json" http://esp32-server.local:8080/api/off```



# API for MQTT

- turn on   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/usb/on" -m ""```

- turn off   
```mosquitto_pub -h broker.emqx.io -p 1883 -t "/api/usb/off" -m ""```

