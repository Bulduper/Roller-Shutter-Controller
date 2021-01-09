# Roller Shutter Controller
This is a rather quick project that I was planning to do several years ago. This time, after gaining some experience, I felt I'd manage do make it work.

## Wireless control over the shutter
The goal was to make a small module on PCB, that would allow me to close and open the blinds:
- by a voice command
- by schedule
- by an app

But I still wanted my neat-looking electrical switch, so I needed the module to be as small as possible and it had to fit into the junction box. That was a challenge!

## The brain - ESP8266-01
Why ESP microcontroller? It is cheap, small, has 4 GPIO pins* (version 1) - enough and most importantly it has WiFi onboard!

*you might have heard of only 2 GPIOs on ESP8266-01, but if you don't need UART functionality you can use both RX and TX as GPIO as well. 

## MQTT & IFTTT
As I can call this project an IoT, the perfect solution for communicaton was MQTT communication. As a MQTT Broker I chose Adafruit.IO as it is free and is compatible with IFTTT service.
Connecting to IFTTT allowed me to send mqtt messages triggered by Google Assistant receiving my voice request.
I also use IFTTT to close and open the shutter at a specific time.