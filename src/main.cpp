#include <Arduino.h>
#include <Preferences.h>

// ==================== FUNCTION DECLARATIONS ====================
void setup();
void loop();
void handleCommand(String command);
void sendHeartbeat();
void sendStatusUpdate();
void sendSystemInfo();
void setLaserState(bool state);
void setLaserBrightness(int brightness);
void readAnalogPins();
String getFormattedTime();
String getFormattedUptime();
float getVoltageFromAnalog(int analogValue);
void printHelp();
void printSystemStatus();
void runDiagnostics();
void memoryTest();
void saveBrightnessToPreferences();
void loadBrightnessFromPreferences();
void sendInitialDeviceState(); // New function

// ==================== GLOBAL VARIABLES ====================
String receivedData = "";
unsigned long lastHeartbeat = 0;
unsigned long bootTime = 0;
bool heartbeatEnabled = true;
int heartbeatInterval = 5000; // 5 seconds default

// Laser control variables
bool laserState = false;
int laserBrightness = 50; // 0-100%
int laserPwmValue = 127;  // 0-255 PWM value

// Pin configuration
const int LASER_PIN = 6; // GPIO 6 for laser control
const int DEFAULT_ANALOG_PIN = A0;

// PWM configuration for laser
const int PWM_FREQ = 1000;    // 1kHz PWM frequency
const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)
const int PWM_CHANNEL = 0;    // PWM channel 0

// Version info
const String FIRMWARE_VERSION = "5.1";
const String BUILD_DATE = __DATE__;
const String BUILD_TIME = __TIME__;

// Preferences object
Preferences preferences;

// Connection state tracking
bool wasConnected = false;
unsigned long lastSerialActivity = 0;
const unsigned long CONNECTION_TIMEOUT = 3000; // 3 seconds

// ==================== SETUP FUNCTION ====================
void setup()
{
    Serial.begin(115200);

    unsigned long startTime = millis();
    while (!Serial && (millis() - startTime < 3000))
    {
        delay(10);
    }

    bootTime = millis();

    // Initialize preferences
    preferences.begin("laser-ctrl", false); // false = read/write mode

    // Load saved brightness value
    loadBrightnessFromPreferences();

    // Initialize laser pin with PWM
    pinMode(LASER_PIN, OUTPUT);

    // Configure PWM for laser control
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LASER_PIN, PWM_CHANNEL);

    // Initialize laser to OFF state but with saved brightness
    setLaserState(false);

    pinMode(DEFAULT_ANALOG_PIN, INPUT);

    delay(1000);

    Serial.println("ESP32-S3 Laser Controller v" + FIRMWARE_VERSION + " Ready");
    Serial.println("Loaded brightness: " + String(laserBrightness) + "%");

    // Send initial device state after a short delay
    delay(500);
    sendInitialDeviceState();
}

// ==================== MAIN LOOP ====================
void loop()
{
    bool currentlyConnected = false;

    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim();

        currentlyConnected = true;
        lastSerialActivity = millis();

        if (command.length() > 0)
        {
            handleCommand(command);
        }
    }

    // Check if we have recent serial activity (indicates connection)
    if (millis() - lastSerialActivity < CONNECTION_TIMEOUT)
    {
        currentlyConnected = true;
    }

    // Detect new connection (transition from not connected to connected)
    if (currentlyConnected && !wasConnected)
    {
        Serial.println("Connection detected - sending device state");
        delay(100); // Small delay to ensure UI is ready
        sendInitialDeviceState();
    }

    wasConnected = currentlyConnected;

    if (heartbeatEnabled && (millis() - lastHeartbeat > heartbeatInterval))
    {
        sendHeartbeat();
        lastHeartbeat = millis();
    }

    delay(10);
}

// ==================== NEW FUNCTION: SEND INITIAL DEVICE STATE ====================
void sendInitialDeviceState()
{
    // Send current device state as JSON for easy parsing
    Serial.println("{\"type\":\"initial_state\",\"laser_state\":" +
                   String(laserState ? "true" : "false") + ",\"laser_brightness\":" +
                   String(laserBrightness) + ",\"version\":\"" +
                   FIRMWARE_VERSION + "\",\"uptime_ms\":" +
                   String(millis() - bootTime) + ",\"free_heap_bytes\":" +
                   String(ESP.getFreeHeap()) + "}");

    // Also send a human-readable message
    Serial.println("Device initialized - Laser: " + String(laserState ? "ON" : "OFF") +
                   ", Brightness: " + String(laserBrightness) + "%");
}

// ==================== COMMAND HANDLER ====================
void handleCommand(String command)
{
    // Laser control commands
    if (command == "LASER_ON")
    {
        setLaserState(true);
    }
    else if (command == "LASER_OFF")
    {
        setLaserState(false);
    }
    else if (command == "LASER_TOGGLE")
    {
        setLaserState(!laserState);
    }

    // Laser brightness control
    else if (command.startsWith("SET_LASER_PWM:"))
    {
        int brightness = command.substring(14).toInt();
        if (brightness >= 0 && brightness <= 100)
        {
            setLaserBrightness(brightness);
        }
    }
    else if (command.startsWith("SET_LASER_BRIGHTNESS:"))
    {
        int brightness = command.substring(21).toInt();
        if (brightness >= 0 && brightness <= 100)
        {
            setLaserBrightness(brightness);
        }
    }

    // System status commands
    else if (command == "STATUS")
    {
        sendStatusUpdate();
    }
    else if (command == "SYSTEM_INFO")
    {
        sendSystemInfo();
    }
    else if (command == "VERSION")
    {
        Serial.println("Firmware Version: " + FIRMWARE_VERSION);
        Serial.println("Build Date: " + String(BUILD_DATE) + " " + String(BUILD_TIME));
        Serial.println("Hardware: ESP32-S3 + CH340K");
        Serial.println("Laser Pin: GPIO " + String(LASER_PIN));
    }

    // Analog reading
    else if (command == "ANALOG_READ")
    {
        int analogValue = analogRead(DEFAULT_ANALOG_PIN);
        float voltage = getVoltageFromAnalog(analogValue);
        Serial.println("Analog A0: " + String(analogValue) + " (" + String(voltage, 2) + "V)");
    }

    // Laser status query
    else if (command == "LASER_STATUS")
    {
        Serial.println("Laser State: " + String(laserState ? "ON" : "OFF"));
        Serial.println("Laser Brightness: " + String(laserBrightness) + "%");
        Serial.println("PWM Value: " + String(laserPwmValue) + "/255");
    }

    // Request initial state (useful for manual refresh)
    else if (command == "GET_INITIAL_STATE")
    {
        sendInitialDeviceState();
    }

    // Heartbeat control
    else if (command == "HEARTBEAT_ON")
    {
        heartbeatEnabled = true;
    }
    else if (command == "HEARTBEAT_OFF")
    {
        heartbeatEnabled = false;
    }
    else if (command.startsWith("HEARTBEAT_INTERVAL:"))
    {
        int interval = command.substring(19).toInt();
        if (interval >= 1000 && interval <= 60000)
        {
            heartbeatInterval = interval;
        }
    }

    // Diagnostic commands
    else if (command == "DIAGNOSTICS")
    {
        runDiagnostics();
    }
    else if (command == "MEMORY_TEST")
    {
        memoryTest();
    }

    // System commands
    else if (command == "RESTART" || command == "REBOOT")
    {
        setLaserState(false); // Safety: turn off laser before restart
        delay(1000);
        ESP.restart();
    }
    else if (command == "HELP")
    {
        printHelp();
    }
}

// ==================== LASER CONTROL FUNCTIONS ====================
void setLaserState(bool state)
{
    laserState = state;

    if (state)
    {
        ledcWrite(PWM_CHANNEL, laserPwmValue);
    }
    else
    {
        ledcWrite(PWM_CHANNEL, 0);
    }
}

void setLaserBrightness(int brightness)
{
    brightness = constrain(brightness, 0, 100);
    laserBrightness = brightness;

    laserPwmValue = map(brightness, 0, 100, 0, 255);

    // Save to preferences whenever brightness changes
    saveBrightnessToPreferences();

    if (laserState)
    {
        ledcWrite(PWM_CHANNEL, laserPwmValue);
    }
}

// ==================== PREFERENCES FUNCTIONS ====================
void saveBrightnessToPreferences()
{
    preferences.putInt("brightness", laserBrightness);
    Serial.println("Brightness saved: " + String(laserBrightness) + "%");
}

void loadBrightnessFromPreferences()
{
    // Load brightness from preferences, default to 50 if not found
    laserBrightness = preferences.getInt("brightness", 50);

    // Ensure the value is within valid range
    laserBrightness = constrain(laserBrightness, 0, 100);

    // Update PWM value based on loaded brightness
    laserPwmValue = map(laserBrightness, 0, 100, 0, 255);
}

// ==================== COMMUNICATION FUNCTIONS ====================
void sendHeartbeat()
{
    unsigned long uptime = millis() - bootTime;
    int freeHeap = ESP.getFreeHeap();

    Serial.println("{\"type\":\"heartbeat\",\"uptime_ms\":" +
                   String(uptime) + ",\"free_heap_bytes\":" +
                   String(freeHeap) + ",\"laser_state\":" +
                   String(laserState ? "true" : "false") + ",\"laser_brightness\":" +
                   String(laserBrightness) + ",\"timestamp\":\"" +
                   getFormattedTime() + "\",\"version\":\"" +
                   FIRMWARE_VERSION + "\"}");
}

void sendStatusUpdate()
{
    unsigned long uptime = millis() - bootTime;
    int freeHeap = ESP.getFreeHeap();
    int totalHeap = ESP.getHeapSize();
    int analogValue = analogRead(DEFAULT_ANALOG_PIN);
    float voltage = getVoltageFromAnalog(analogValue);

    Serial.println("{\"type\":\"status\",\"uptime_ms\":" +
                   String(uptime) + ",\"free_heap_bytes\":" +
                   String(freeHeap) + ",\"total_heap_bytes\":" +
                   String(totalHeap) + ",\"laser_state\":" +
                   String(laserState ? "true" : "false") + ",\"laser_brightness\":" +
                   String(laserBrightness) + ",\"laser_pwm_value\":" +
                   String(laserPwmValue) + ",\"analog_a0\":" +
                   String(analogValue) + ",\"voltage_a0\":" +
                   String(voltage, 2) + ",\"cpu_freq_mhz\":" +
                   String(ESP.getCpuFreqMHz()) + ",\"timestamp\":\"" +
                   getFormattedTime() + "\",\"version\":\"" +
                   FIRMWARE_VERSION + "\",\"heartbeat_enabled\":" +
                   String(heartbeatEnabled ? "true" : "false") + "}");
}

void sendSystemInfo()
{
    Serial.println("ESP32-S3 Laser Controller System Information v" + FIRMWARE_VERSION);
    Serial.println("Hardware: " + String(ESP.getChipModel()) + " Rev " + String(ESP.getChipRevision()));
    Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    Serial.println("Heap Size: " + String(ESP.getHeapSize()) + " bytes");
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("SDK Version: " + String(ESP.getSdkVersion()));
    Serial.println("Build: " + String(BUILD_DATE) + " " + String(BUILD_TIME));
    Serial.println("Boot Time: " + getFormattedUptime());
    Serial.println("Laser Pin: GPIO " + String(LASER_PIN));
    Serial.println("Laser State: " + String(laserState ? "ON" : "OFF"));
    Serial.println("Laser Brightness: " + String(laserBrightness) + "% (saved in preferences)");
    if (heartbeatEnabled)
    {
        Serial.println("Heartbeat Interval: " + String(heartbeatInterval / 1000) + " seconds");
    }
}

// ==================== UTILITY FUNCTIONS ====================
String getFormattedTime()
{
    unsigned long totalSeconds = (millis() - bootTime) / 1000;
    unsigned long hours = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;

    return String(hours) + ":" +
           (minutes < 10 ? "0" : "") + String(minutes) + ":" +
           (seconds < 10 ? "0" : "") + String(seconds);
}

String getFormattedUptime()
{
    unsigned long totalSeconds = (millis() - bootTime) / 1000;
    unsigned long days = totalSeconds / 86400;
    unsigned long hours = (totalSeconds % 86400) / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;

    String uptime = "";
    if (days > 0)
        uptime += String(days) + "d ";
    if (hours > 0 || days > 0)
        uptime += String(hours) + "h ";
    if (minutes > 0 || hours > 0 || days > 0)
        uptime += String(minutes) + "m ";
    uptime += String(seconds) + "s";

    return uptime;
}

float getVoltageFromAnalog(int analogValue)
{
    return (analogValue * 3.3) / 4095.0;
}

void printSystemStatus()
{
    Serial.println("Board: " + String(ESP.getChipModel()) + " @ " + String(ESP.getCpuFreqMHz()) + "MHz");
    Serial.println("Memory: " + String(ESP.getFreeHeap()) + "/" + String(ESP.getHeapSize()) + " bytes free");
    Serial.println("Laser Pin: GPIO " + String(LASER_PIN));
}

// ==================== DIAGNOSTIC FUNCTIONS ====================
void runDiagnostics()
{
    Serial.println("Running Laser Controller Diagnostics");

    bool originalLaserState = laserState;
    int originalBrightness = laserBrightness;

    setLaserBrightness(10);
    setLaserState(true);
    delay(500);
    setLaserState(false);

    setLaserBrightness(originalBrightness);
    setLaserState(originalLaserState);

    int analogValue = analogRead(DEFAULT_ANALOG_PIN);
    Serial.println("A0 reading: " + String(analogValue) + " (" + String(getVoltageFromAnalog(analogValue), 2) + "V)");

    memoryTest();

    Serial.println("Diagnostics completed");
}

void memoryTest()
{
    Serial.println("Heap: " + String(ESP.getFreeHeap()) + "/" + String(ESP.getHeapSize()) + " bytes");
    Serial.println("PSRAM: " + String(ESP.getFreePsram()) + "/" + String(ESP.getPsramSize()) + " bytes");

    int *testArray = (int *)malloc(1000 * sizeof(int));
    if (testArray != nullptr)
    {
        Serial.println("Memory allocation test passed");
        free(testArray);
    }
    else
    {
        Serial.println("Memory allocation test failed");
    }
}

// ==================== HELP FUNCTION ====================
void printHelp()
{
    Serial.println("ESP32-S3 Laser Controller v" + FIRMWARE_VERSION + " Commands");
    Serial.println("Laser Control:");
    Serial.println("  LASER_ON                    - Turn on laser");
    Serial.println("  LASER_OFF                   - Turn off laser");
    Serial.println("  LASER_TOGGLE                - Toggle laser state");
    Serial.println("  SET_LASER_PWM:value         - Set laser brightness (0-100%) - SAVED");
    Serial.println("  SET_LASER_BRIGHTNESS:value  - Set laser brightness (0-100%) - SAVED");
    Serial.println("  LASER_STATUS                - Show laser status");
    Serial.println("Reading:");
    Serial.println("  ANALOG_READ         - Read analog value from A0");
    Serial.println("System:");
    Serial.println("  STATUS              - Get device status (JSON)");
    Serial.println("  SYSTEM_INFO         - Show detailed system info");
    Serial.println("  VERSION             - Show firmware version");
    Serial.println("  GET_INITIAL_STATE   - Get current device state (JSON)");
    Serial.println("  DIAGNOSTICS         - Run system diagnostics");
    Serial.println("  MEMORY_TEST         - Test memory allocation");
    Serial.println("  RESTART             - Restart the ESP32-S3");
    Serial.println("Heartbeat Control:");
    Serial.println("  HEARTBEAT_ON        - Enable periodic heartbeat");
    Serial.println("  HEARTBEAT_OFF       - Disable heartbeat");
    Serial.println("  HEARTBEAT_INTERVAL:ms - Set heartbeat interval (1000-60000)");
    Serial.println("Examples:");
    Serial.println("  SET_LASER_PWM:75          - Set laser to 75% brightness");
    Serial.println("  HEARTBEAT_INTERVAL:5000   - 5 second heartbeat");
    Serial.println("Laser Pin: GPIO " + String(LASER_PIN));
    Serial.println("Safety: Laser automatically turns off on restart");
    Serial.println("Note: Brightness values are automatically saved and restored on power cycle");
    Serial.println("      Device state is automatically sent on connection detection");
}