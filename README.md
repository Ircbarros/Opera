# Home Automation Recipes with ESP32, ESP8266 and IBM Cloud

Wireless Smart Switch, Temperature Sensor and ESP-NOW with ESP32/ESP8266 &amp; IBM CLOUD

Codes used to prototype a low-cost home automation system during the internship at Ópera Automação based on a 32-bit LX6 microprocessor MCU in conjunction with an SSR and impulse relay for lighting control using the IBM cloud and a Node-RED application. Some codes for temperature and humidity control have also been developed.

# How the Dimmer Relay (Wireless Smart Switch) Worked?

- In the first power up, the ESP32 could communicate with the home wireless network using WPS automatically after clicking on a button;
- This communication could also be done manually by accessing an HTML page containing some information about ESP;
- If the communication with the wireless network was accepted, a green LED would light up, otherwise a Red LED would turn ON and the ESP would go into a Loop, trying to reconnect (activating a yellow LED);
- After connecting to the internet, a MQTT communication with the IBM cloud starts;
- The Node-RED was responsible for the application layer at the IBM Cloud;
- The dimmerization of one LED lamb could be done through a capacitive touch (using an impulse relay) or through an application created with Node-RED and Ionic.

# How the Temperature Sensor Worked?

- In the first power up, the ESP8266 could communicate with the home wireless network using WPS automatically after clicking on a button;
- This communication could also be done manually by accessing an HTML page containing some information about ESP;
- If the communication with the wireless network was accepted, a green LED would light up, otherwise a Red LED would turn ON and the ESP would go into a Loop, trying to reconnect (activating a yellow LED);
- After connecting to the internet, the microprocessor starts to read the humidity and temperature sensors;
- The values were sent to a mobile application using the adafruit cloud 
