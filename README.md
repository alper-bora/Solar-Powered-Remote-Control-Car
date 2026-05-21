# Solar Powered Remote Control Car

This project is for an ESP32-based remote control car that can be powered by solar energy. It uses a simple web interface to control the movement of the car from a smartphone or computer.

## How it works
The ESP32 creates its own Wi-Fi access point. When you connect to it and open the IP address in a browser, you get a control panel with sliders for steering and speed. The code handles the motor signals and even has a buzzer for a horn.

## Features
- Web-based control panel (best used in landscape mode)
- Dual motor control for steering and driving
- A retro style horn melody
- Emergency stop button to cut power to motors immediately

## Code Structure
The main logic is in `src/main.cpp`. It handles the Wi-Fi setup, the web server routes, and the PWM signals for the motor drivers. The UI is built with basic HTML and CSS embedded directly in the code.
