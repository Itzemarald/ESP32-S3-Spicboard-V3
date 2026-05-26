# ESP32 UART Remote Control (Transmitter)

This branch contains **exclusively the code for the remote control (transmitter)** of the ESP32-SPIC-Board project. It is designed to be flashed onto a second, "bare" ESP32 that does not require any additional hardware.

*(Note: The code for the main system (receiver), including the SPIC Board drivers, can be found in the `main` branch of this repository).*

---

## Functionality

This program turns the ESP32 into a simple wired remote control. 
It utilizes the onboard **BOOT button (GPIO 0)** to trigger a hardware interrupt. As soon as the button is pressed, the board sends the command `"UART_NEXT_MODE"` via the UART interface to the main system.

## Technical Implementation

- **Hardware Interrupts (ISR):** The BOOT button is configured as `GPIO_INTR_NEGEDGE`. Pressing it triggers a hardware-level interrupt.
- **FreeRTOS Queues:** To keep the interrupt execution time as short as possible, the event is passed to the main loop asynchronously via `xQueueSendFromISR`.
- **Software Debouncing:** Since mechanical buttons experience "bounce" (sending multiple signals per physical press), the RTOS system filters out any signals occurring within 300ms of each other in the main loop using `xTaskGetTickCount()`.
- **UART:** Message transmission runs asynchronously at 115200 baud via the custom `Spic_UART` component.

---

## 🔌 Wiring with the Main System (ESP 1)

For the remote control to function, the two ESP32 boards must be connected using three wires (Crossover / Null-modem principle):

| Remote Control (ESP 2) | Connection | Main System (ESP 1) |
| :--- | :---: | :--- |
| **GND** | ↔️ | **GND** (required!) |
| **TX Pin** (e.g., Pin 4) | ➡️ | **RX Pin** (e.g., Pin 5) |
| **RX Pin** (e.g., Pin 5) | ⬅️ | **TX Pin** (e.g., Pin 4) |

---

## Compiling & Flashing

1. **Build the project:**
   ```bash
   idf.py build