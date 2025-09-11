# ON/OFF by cron
Turning USB ON/OFF using cron.   
This project requires a WiFi connection.   
I found [this](https://github.com/staticlibs/ccronexpr) library.   
Defining dates and times in crontab is complicated, but this library handles it well.   
This is a great library.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-usb-switch
cd esp-idf-usb-switch/cron
idf.py menuconfig
idf.py flash
```

# Configuration
![Image](https://github.com/user-attachments/assets/c61a2327-6c7a-4f3d-8415-fed6d4cb870b)
![Image](https://github.com/user-attachments/assets/0ca043af-9667-41bf-a6a1-9969553684fb)

## WiFi Setting   
Set the information of your access point.   
![Image](https://github.com/user-attachments/assets/7accc112-40b1-4180-aab6-8cc359628391)

## NTP Setting   
Set the information of your NTP server and time zone.   
![Image](https://github.com/user-attachments/assets/bd723c26-b26b-4c2a-a4b2-57b35a01d1d5)

## RF Setting   
Set the information of transmitter module.   
![Image](https://github.com/user-attachments/assets/0633bef4-edb8-4f95-af89-544cc2f4a0e9)

# Wiring
|Transmitter Module||ESP32|
|:-:|:-:|:-:|
|DATA|--|GPIO5|
|GND|--|GND|
|VCC|--|3.3V|

# Typical circuit
![Image](https://github.com/user-attachments/assets/e784ee89-77f6-41f4-a515-87160c520f98)


# Setting crontab
crontab is in the crontab directory.
Supports cron expressions with seconds field.
```
      +---------------------------  second (0 - 59)
      |  +------------------------  minute (0 - 59)
      |  |  +---------------------  hour (0 - 23)
      |  |  |  +------------------  day of month (1 - 31)
      |  |  |  |  +---------------  month (1 - 12)
      |  |  |  |  |  +------------  day of week (0 - 6) (Sunday to Saturday;
      |  |  |  |  |  |                                   7 is also Sunday on some systems)
      |  |  |  |  |  |  +---------  task name
      |  |  |  |  |  |  |
      *  *  *  *  *  *  task
```

In Linux you specify the command, but in this project you specify the task name.   
This project uses task_on and task_off tasks.   
Note:   
Task names in FreeRTOS are function names.   
Tasks are created using the ```xTaskCreate``` function.
However, each task works independently.   

```
# Edit this file to introduce tasks to be run by cron.
#
# Each task to run has to be defined through a single line
# indicating with different fields when the task will be run
# and what command to run for the task
#
# To define the time you can provide concrete values for
# minute (m), hour (h), day of month (dom), month (mon),
# and day of week (dow) or use '*' in these fields (for 'any').
#
# Notice that tasks will be started based on the cron's system
# daemon's notion of time and timezones.
#
# Output of the crontab jobs (including errors) is sent through
# email to the user the crontab file belongs to (unless redirected).
#
# For example, you can run a backup of all your user accounts
# at 5 a.m every week with:
# 0 5 * * 1 tar -zcf /var/backups/home.tgz /home/
#
# For more information see the manual pages of crontab(5) and cron(8)
#
# s m h  dom mon dow   task
0 0-59/10 * * * * task_on         # run at 0/10/20/30/40/50 minute
0 5-59/10 * * * * task_off        # run at 5/15/25/35/45/55 minute
```


# Crontab syntax
Crontab allows complex scheduling.   
```
"*/15 * 1-4 * * *",  "2012-07-01_09:53:50", "2012-07-02_01:00:00"
"0 */2 1-4 * * *",   "2012-07-01_09:00:00", "2012-07-02_01:00:00"
"0 0 7 ? * MON-FRI", "2009-09-26_00:42:55", "2009-09-28_07:00:00"
"0 30 23 30 1/3 ?",  "2011-04-30_23:30:00", "2011-07-30_23:30:00"
```
See more examples in [here](https://github.com/staticlibs/ccronexpr).   
