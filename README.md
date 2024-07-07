# ESP32 CMD LED Blinker

Hello. In this project, I have written a command line utility that runs on an ESP32, and can be used to control the state of an LED connected to the board.

## How to build
The project can be easily compiled with ESP-IDF. Visit this [link](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation) for more information.

## How to run

The diagram below describes the connections to be made. 
![image](misc/Screenshot%202024-07-07%20141143.png)

The LED should be connected to GPIO pin two of your board. Furthermore, in this project we use pins **4** and **5** as the **Tx** and **Rx** of the ESP32, for serial communication. An FTDI USB to Serial adapter can be used to connected your computer to the serial ports of the ESP32.


The command line termnial offers two commands-
1. `echo <args>` - This command will echo back the arguments provided to it.
2. `led <arg0>` - This command accepts only one integer argument. If `arg0` is `0`, then the LED will be turned off. If  `arg0` is `1`, the LED will be turned on. For any other integer value of `arg0`, it will blink with a half time period of the value, in milliseconds. So `led 0` means OFF, `led 1` means ON (no blinking), and `led 2000` will turn the LED on for 2 secs, then turn if off for 2 secs, and then turn it back on for 2 secs, and so on...


## Wokwi Simulator

The project can be run with actual ESP32, by using [Wokwi Simulator](https://wokwi.com/). I would recommend installing the [VS Code Extension](https://docs.wokwi.com/vscode/getting-started) to run the simulator.

Simply build the project using ESP-IDF, and then open the `wokwi/diagram.json` file in VS Code. Then click on the `Run Simulation` button to the run the simulation. When running the project in Wokwi, the serial monitor is available at `localhost:4000`.

To access ESP32's logs, change the line 18 from `[ "esp:4", "$serialMonitor:RX", "", [] ],` to `[ "esp:TX", "$serialMonitor:RX", "", [] ],`. The serial monitor will then output the log messages, while the Rx will still work the same.