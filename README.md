# Friendsies Bag: Smart Eco-Friendly Shopping Bag

An IoT-based graduation project that combines smart weight monitoring with a web-based interface to encourage sustainable shopping habits.

## Project Overview

Friendsies Bag is a prototype smart shopping bag designed to monitor the weight of carried items, alert users when the bag exceeds its weight limit, and track the environmental and financial impact of reusable shopping bags.

The system uses an **ESP32 microcontroller** connected to a load cell through an HX711 amplifier. The ESP32 creates its own Wi-Fi access point and hosts a web interface that users can access directly from their phones or computers.

The project operates locally without requiring an internet connection or a traditional server-side database.

## Key Features

- **Real-Time Weight Monitoring:** Displays weight measurements through an interactive dashboard.
- **Overload Protection:** Activates a buzzer and displays a warning when the weight exceeds 7 kg.
- **Trip Tracking:** Records completed shopping trips and their associated weights.
- **Environmental Impact:** Estimates plastic bags diverted and CO₂ emissions prevented.
- **Financial Impact:** Estimates money saved by using reusable shopping bags.
- **Achievement System:** Rewards users with badges for reaching shopping milestones.
- **Web-Based Interface:** Includes login, dashboard, trip history, trophy room, and settings pages.
- **Local Data Storage:** Saves trip history and achievements using browser localStorage.

## Technologies Used

**Hardware**
- ESP32 development board
- HX711 load cell amplifier
- Strain gauge load cell
- Buzzer

**Software**
- C++ / Arduino
- HTML5
- CSS3
- JavaScript
- ESP32 Wi-Fi and WebServer libraries
- HX711_ADC library
- Browser localStorage

## How It Works

1. The ESP32 creates a local Wi-Fi network named `Friendsies_Bag`.
2. The user connects their device to the network.
3. The ESP32 hosts the web interface, accessible through its local IP address.
4. The user logs in and starts a weighing session.
5. The load cell measures weight, and the ESP32 processes the readings using a moving average filter.
6. The dashboard displays updated weight measurements.
7. If the weight exceeds 7 kg, an overload warning is activated.
8. When a valid session is completed, the trip is recorded in the browser's local storage.

## Project Screenshots

*Screenshots of the dashboard, trip history, trophy room, and physical prototype will be added here.*

## Installation and Setup

### Prerequisites

- Arduino IDE
- ESP32 board package
- HX711_ADC library
- Compatible ESP32 development board and connected hardware
- USB cable

### Uploading the Firmware

1. Open `Friendsies_Bag_F.ino` in Arduino IDE.
2. Select the appropriate ESP32 board and serial port.
3. Install the HX711_ADC library if necessary.
4. Verify and upload the sketch.
5. Open Serial Monitor at **57600 baud**.
6. Find the ESP32's local IP address in the serial output.

### Accessing the Web Interface

1. Power the ESP32.
2. Connect your device to the `Friendsies_Bag` Wi-Fi network using the password configured in the firmware.
3. Open the ESP32 IP address in your browser (typically `http://192.168.4.1`).
4. Sign in using the prototype credentials configured in the code.

The HTML, CSS, and JavaScript are embedded directly in the Arduino sketch and served through the ESP32's web server.

## Data Storage

Friendsies Bag uses browser **localStorage** to save shopping trips and achievement progress.

Data is stored locally for each configured user rather than in an external database.

Because storage is browser-specific, clearing browser data or switching devices may result in the loss of saved history.

**Note:** The login system uses hardcoded prototype credentials and is intended for demonstration purposes, not production-grade authentication.

## Calibration

The HX711 load cell uses a calibration factor configured in the firmware.

To recalibrate the system:

1. Place a known weight on the load cell.
2. Adjust the calibration factor in the Arduino sketch.
3. Re-upload the firmware.
4. Verify that the measured weight matches the reference weight.

## Troubleshooting

- **Wi-Fi network not visible:** Check that the ESP32 is powered and running.
- **Incorrect weight readings:** Inspect the load cell wiring, mounting, and calibration.
- **Overload warning persists:** Reduce the weight below the configured threshold.
- **Missing trip history:** Check whether browser storage was cleared or a different browser was used.

## Contributors

Developed as a university graduation project by:

- Lama Alasous
- Taif Alqahtani

**Supervisor:** Noha Qadi

**University:** Imam Mohammad Ibn Saud Islamic University

## Project Status

Completed graduation project prototype.

The repository contains the final working ESP32 firmware, including the embedded web interface.
