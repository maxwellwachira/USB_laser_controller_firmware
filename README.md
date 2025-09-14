# USB Laser Controller - Firmware

ESP32-S3 firmware for laser controller devices. This firmware provides serial communication, laser control, persistent settings, and real-time device monitoring capabilities.

## Features

- 🔌 **USB Serial Communication**: High-speed communication via serial USB bridge
- ⚡ **PWM Laser Control**: Precise brightness control with 8-bit resolution
- 💾 **Persistent Settings**: Automatic saving/loading of brightness preferences
- 📊 **Real-time Monitoring**: Device stats, memory usage, and system diagnostics
- 🔄 **Auto-sync**: Automatic state broadcasting on client connection
- ⚙️ **Comprehensive Commands**: Full command set for laser and system control
- 🛡️ **Safety Features**: Automatic laser shutdown on restart/disconnect

## Hardware Requirements

### Supported Microcontrollers
- **ESP32-S3** (Primary target)
- ESP32-S3-WROOM modules
- ESP32-S3-DevKitC development boards

### Hardware Specifications
- **Laser Pin**: GPIO 6
- **USB Interface**: USB-to-Serial bridge
- **PWM Frequency**: 1kHz
- **PWM Resolution**: 8-bit (0-255)
- **Baud Rate**: 115200
- **Power Requirements**: 5V via USB

## Prerequisites

### PlatformIO IDE
You need PlatformIO installed in Visual Studio Code.

#### Installing PlatformIO:

1. **Install Visual Studio Code**
   - Download from [code.visualstudio.com](https://code.visualstudio.com/)

2. **Install PlatformIO Extension**
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Install the official PlatformIO extension
   - Restart VS Code

3. **Verify Installation**
   - You should see the PlatformIO icon in the sidebar
   - The PlatformIO toolbar should appear at the bottom

### Alternative: PlatformIO Core (Command Line)
```bash
# Install Python 3.6+ first, then:
pip install platformio

# Verify installation
pio --version
```

## Installation

1. **Clone the firmware repository**
   ```bash
   git clone https://github.com/maxwellwachira/USB_laser_controller_firmware.git
   ```

2. **Navigate to the project directory**
   ```bash
   cd USB_laser_controller_firmware
   ```

3. **Open in Visual Studio Code**
   ```bash
   code .
   ```

## Building and Uploading

### Method 1: Using PlatformIO IDE (Recommended)

1. **Open the project** in VS Code with PlatformIO extension
2. **Connect your ESP32-S3** device via USB
3. **Build the project**: Click the ✅ checkmark in the PlatformIO toolbar
4. **Upload firmware**: Click the ➡️ arrow in the PlatformIO toolbar

### Method 2: Using PlatformIO Core

```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Configuration

### Pin Configuration
The laser control pin is configured in the firmware:
```cpp
const int LASER_PIN = 6; // GPIO 6 for laser control
```

### PWM Settings
```cpp
const int PWM_FREQ = 1000;    // 1kHz PWM frequency
const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)
const int PWM_CHANNEL = 0;    // PWM channel 0
```

### Serial Configuration
```cpp
Serial.begin(115200); // Fixed baud rate
```

## Firmware Commands

### Laser Control
```
LASER_ON                     - Turn on laser
LASER_OFF                    - Turn off laser  
LASER_TOGGLE                 - Toggle laser state
SET_LASER_PWM:value          - Set brightness (0-100%)
SET_LASER_BRIGHTNESS:value   - Set brightness (0-100%)
LASER_STATUS                 - Get laser status
```

### System Commands
```
STATUS                       - Get device status (JSON)
SYSTEM_INFO                  - Show detailed system info
VERSION                      - Show firmware version
GET_INITIAL_STATE           - Get current device state
DIAGNOSTICS                 - Run system diagnostics
MEMORY_TEST                 - Test memory allocation
RESTART                     - Restart the ESP32-S3
HELP                        - Show all commands
```

### Monitoring Commands
```
ANALOG_READ                 - Read analog pin A0
HEARTBEAT_ON               - Enable periodic heartbeat
HEARTBEAT_OFF              - Disable heartbeat  
HEARTBEAT_INTERVAL:ms      - Set heartbeat interval (1000-60000ms)
```

## Serial Communication Protocol

### JSON Data Format
The firmware sends structured JSON data for easy parsing:

```json
{
  "type": "initial_state",
  "laser_state": false,
  "laser_brightness": 75,
  "version": "5.1",
  "uptime_ms": 45000,
  "free_heap_bytes": 234567
}
```

### Message Types
- `initial_state` - Sent automatically on connection
- `status` - Response to STATUS command
- `heartbeat` - Periodic status updates

## Development

### Project Structure
```
USB_laser_controller_firmware/
├── src/
│   └── main.cpp            # Main firmware source
├── include/                # Header files
├── lib/                    # Libraries
├── platformio.ini          # PlatformIO configuration
└── README.md              # This file
```

### PlatformIO Configuration
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

### Dependencies
- **Arduino Framework** - Core ESP32 support
- **Preferences Library** - NVRAM storage for settings

## Troubleshooting

### Upload Issues
- **Driver Problems**: Install CH34x USB drivers
- **Port Detection**: Try different USB cables/ports
- **Permissions**: On Linux, add user to dialout group: `sudo usermod -a -G dialout $USER`

### Serial Communication
- **Baud Rate**: Ensure client uses 115200 baud
- **Flow Control**: Set to "none" in client applications
- **Buffer Issues**: Commands should end with newline (`\n`)

### Connection Detection
- The firmware automatically detects new connections
- If auto-sync fails, send `GET_INITIAL_STATE` command
- Check for proper USB cable and driver installation

### Memory Issues
- Use `MEMORY_TEST` command to check heap
- Monitor free heap with `STATUS` command
- Restart device if memory gets low

## Safety Features

- **Automatic Shutdown**: Laser turns off on device restart
- **Preference Backup**: Brightness settings survive power cycles  
- **Command Validation**: Input validation for all commands
- **Error Handling**: Graceful handling of communication errors

## Customization

### Changing Laser Pin
```cpp
const int LASER_PIN = 6; // Change to your desired GPIO pin
```

### Adjusting PWM Frequency
```cpp
const int PWM_FREQ = 1000; // Change frequency as needed
```

### Modifying Heartbeat Interval
```cpp
int heartbeatInterval = 5000; // Default 5 seconds
```

## Version History

- **v5.1** - Auto-sync on connection, improved JSON protocol
- **v5.0** - Preferences support, enhanced commands
- **v4.x** - PWM control, basic serial communication


## Related Projects

- [USB Laser Controller Client](https://github.com/maxwellwachira/USB_laser_controller) - Web-based interface for this firmware