# ESP32-S3 SPIC Board Project

This project combines the **SPIC Board learning platform** with the **ESP32-S3** microcontroller. It demonstrates the development of robust, object-oriented firmware in C++ using the **FreeRTOS** real-time operating system (ESP-IDF).

The SPIC (System Programming in C) Board is a hardware platform for digital circuits and microcontrollers, originally developed for educational purposes at FAU Erlangen-Nürnberg. 
For more information about the hardware, visit the [SPIC Board API Reference](https://sys.cs.fau.de/lehre/ws23/spic/spicboard/extern/index.html).

---

## Project Structure

- `main/` - Contains an example main application (`main.cpp`), task definitions, and the FreeRTOS logic.
- `components/spic_lib/` - Object-Oriented (C++) hardware drivers for the SPIC Board:
  - `Spic_Button`: Button evaluation with hardware-level deferred debouncing via OS timers.
  - `Spic_ADC`: Reading the potentiometer and photoresistor.
  - `Spic_LED`: Controls for the 8 LEDs.
  - `Spic_7seg`: Multiplexing driver for the 7-segment display.
  - `Spic_Timer`: Custom wrapper class for `esp_timer` (hardware timers & blocking RTOS delays).

---

## Features & Operating Modes

The example program features three distinct operating modes. 
**Mode Switching:** Pressing both buttons simultaneously (**Button 0 + Button 1**) switches the system to the next mode.

### Mode 0: Stopwatch / Counter + LED Control
A background hardware timer increments every second from 0 to 99, displaying the value on the 7-segment display.
- **Button 0:** Start / Pause the timer and lights up LEDs one by one.
- **Button 1:** Reset the timer to `0` and turns off LEDs one by one.

### Mode 1: Potentiometer (ADC)
Reads the current analog value from the potentiometer on the SPIC Board.
- Displays the raw value (0-4095) scaled down (0-99) on the 7-segment display.
- Visualizes the analog level on the LED bar graph.

### Mode 2: Photoresistor (Light Sensor)
Works similarly to Mode 1, but reads the analog value from the onboard light sensor (LDR).
- Updates the 7-segment display and the LED bar graph dynamically based on ambient light intensity.

---

## 🧠 Software Architecture (FreeRTOS)

This project implements modern RTOS concepts to ensure maximum stability and performance:
- **Separation of Concerns:** Hardware events (button presses) and cyclic sensor updates run in isolated, independent tasks.
- **Event Queues:** Button presses are sent asynchronously to the main loop via a `QueueHandle_t`. This guarantees that no events are dropped, even during high CPU load.
- **Mutexes (Semaphores):** Shared hardware resources (like the LEDs and the 7-segment display) are protected by a `SemaphoreHandle_t` (Mutex) to prevent race conditions when multiple tasks try to update the hardware simultaneously.
- **Deferred Debouncing:** Button interrupts automatically disable themselves upon the first touch and start a one-shot FreeRTOS timer (50ms). The button state is safely evaluated only after the mechanical bouncing has settled.

---

## 🛠️ Compiling & Flashing

This project requires an installed and configured **ESP-IDF** (Espressif IoT Development Framework) environment.

1. **Build the project:**
   ```bash
   idf.py build