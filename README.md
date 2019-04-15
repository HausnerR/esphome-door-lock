# esphome-door-lock

This is project of door lock based on Wemos D1 mini and cheap RC522 RFID module. Software side is based on ESPHome with custom component which exposes data read by RC522 through ESPHome sensors.

That way it's super simple to connect it to Home Assistant.

# Hardware

Project is designed for Wemos D1 mini but can be easily adopted to any other ESPHome compatible hardware as long as it have enough GPIOs.

PCB project and component list is available here and on easyeda if you want to order it from JLCPCB: https://easyeda.com/hausnerr/door-lock.

# Software

Project contains:
 - door_lock.yaml - ESPHome configuration file. It defines all sensors, services and door lock switch for firmware,
 - door_lock_rfid.h - ESPHome custom component. It exposes data read by RC522 as sensors and verify if tag is from allowed list. List can be viewed and updated using services,
 - pcb - components list and gerber files for PCB etching.

# Updating users list

To update users list you need to expose `users list file` on any HTTP server. Simplest way is to use built-in HA server. Create directory `www` inside your HA config dir and put there your `users list file`.
It will be available at http://HA:8123/local/filename.

Then through Home Assistant execute service `esphome.door_lock_sync_users_list_from_server` and send service data like: `{"url":"http://HA:8123/local/filename"}`.
