# esphome-door-lock

This is project of door lock based on Wemos D1 mini and cheap RC522 RFID module. Software side is based on ESPHome with custom component which exposes data read by RC522 through ESPHome sensors.

That way it's super simple to connect it to Home Assistant.

# Hardware

Project is designed for Wemos D1 mini but can be easily adopted to any other ESPHome compatible hardware as long as it have enough GPIOs.

PCB project and component list is available here and on easyeda if you want to order it from JLCPCB: https://easyeda.com/hausnerr/door-lock.

# Software

Project contains:
 - door_lock.yaml - ESPHome configuration file. It defines all sensors, services and door lock switch for firmware,
 - door_lock_rfid.h - ESPHome custom component. It exposes data read by RC522 as sensors and verify if tag is from allowed list. List can be viewed and updated using Home Assistant services,
 - pcb - components list and gerber files for PCB etching.

# Updating users list

In Home Assistant door_lock exposes two services:
 - `print_users_list` - gets users list from device and shows it as notification with JSON format prepared for `upload_users_list`,
 - `upload_users_list` - gets JSON as parameter (same as `print_users_list` returns), and saves users in door_lock.

When users list is empty, door_lock returns example JSON:

```
{"list":[
"XX:XX:XX:XX=Example 1",
"XX:XX:XX:XX=Example 2"
]}
```

# SPIFFS info

ESPhome project removed SPIFFs reserved area from default build flags, but it's required in this project. That's why YAML file have all build_flags set explicitly (without disable warnings flag).