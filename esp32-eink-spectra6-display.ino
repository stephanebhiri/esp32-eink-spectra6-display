/**
 * ESP32 E-Ink Spectra 6-Color Display Controller
 * 
 * A high-performance display controller for Waveshare 13.3" 6-color e-ink displays.
 * Features HTTP polling for image updates, WiFi power management, and optimized
 * battery operation with light sleep functionality.
 * 
 * Hardware Requirements:
 * - Adafruit HUZZAH32 ESP32 Feather
 * - Waveshare 13.3" E-Paper HAT (6-color)
 * - LiPo battery (optional)
 * 
 * @author Stephane Bhiri
 * @version 2.0
 * @date January 2025
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "DEV_Config.h"
#include "EPD_13in3e.h"
#include "WiFiConfig.h"
#include <Preferences.h>

// Display specifications
#define EPD_WIDTH 1200
#define EPD_HEIGHT 1600
static const int BYTES_PER_LINE_HALF = EPD_WIDTH/4;

// Network configuration
char server_url[64];
char last_image_hash[32] = "";

// WiFi credential storage
Preferences preferences;
bool wifi_configured = false;
String stored_ssid = "";
String stored_password = "";

/**
 * Robust JSON parser for extracting string values
 * Handles whitespace and escaped quotes properly
 * 
 * @param payload JSON string to parse
 * @param key Key to extract value for
 * @return Extracted string value or empty string if not found
 */
String parseJsonValue(const String& payload, const char* key) {
  String search_pattern = "\"" + String(key) + "\"";
  int key_position = payload.indexOf(search_pattern);
  if (key_position == -1) return "";

  int current_pos = key_position + search_pattern.length();

  // Skip whitespace to find colon
  while (current_pos < payload.length() && isspace(payload[current_pos])) {
    current_pos++;
  }
  if (current_pos >= payload.length() || payload[current_pos] != ':') return "";
  current_pos++;

  // Skip whitespace to find opening quote
  while (current_pos < payload.length() && isspace(payload[current_pos])) {
    current_pos++;
  }
  if (current_pos >= payload.length() || payload[current_pos] != '"') return "";
  int value_start = current_pos + 1;

  // Find closing quote (handle escaped quotes)
  int value_end = payload.indexOf('"', value_start);
  while (value_end != -1 && payload[value_end - 1] == '\\') {
      value_end = payload.indexOf('"', value_end + 1);
  }
  if (value_end == -1) return "";

  return payload.substring(value_start, value_end);
}

/**
 * Read battery voltage and calculate percentage
 * Uses HUZZAH32 built-in voltage divider on A13
 * 
 * @return Battery percentage (0-100) or -1 if no battery detected
 */
int getBatteryLevel() {
  analogSetAttenuation(ADC_11db);
  analogSetWidth(12);
  
  // Average multiple readings for stability
  int total = 0;
  const int samples = 5;
  for (int i = 0; i < samples; i++) {
    total += analogRead(A13);
    delay(10);
  }
  int average = total / samples;
  
  // Convert to voltage (HUZZAH32 has voltage divider)
  float voltage = (average / 4095.0) * 3.3 * 2.0;
  
  if (average < 100) {
    Serial.println("USB power detected");
    return -1;
  }
  
  // LiPo voltage range: 3.0V (0%) to 4.2V (100%)
  float percentage = ((voltage - 3.0) / (4.2 - 3.0)) * 100.0;
  percentage = constrain(percentage, 0, 100);
  
  Serial.printf("Battery: %.2fV (%d%%)\n", voltage, (int)percentage);
  return (int)percentage;
}

/**
 * Load WiFi credentials from flash memory
 */
void loadWiFiCredentials() {
  preferences.begin("wifi", true);
  
  if (preferences.getBool("configured", false)) {
    stored_ssid = preferences.getString("ssid", "");
    stored_password = preferences.getString("password", "");
    wifi_configured = true;
    Serial.println("Loaded WiFi: " + stored_ssid);
  } else {
    Serial.println("No saved WiFi credentials");
    wifi_configured = false;
  }
  
  preferences.end();
}

/**
 * Save WiFi credentials to flash memory
 */
void saveWiFiCredentials() {
  preferences.begin("wifi", false);
  preferences.putString("ssid", stored_ssid);
  preferences.putString("password", stored_password);
  preferences.putBool("configured", true);
  preferences.end();
  Serial.println("WiFi credentials saved");
}

/**
 * Check server for new image updates
 * Compares MD5 hash to detect changes
 * 
 * @return true if new image available, false otherwise
 */
bool checkForNewImage() {
  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "%s/api/image/info", server_url);
  http.begin(url);
  http.setTimeout(5000);
  
  int response_code = http.GET();
  if (response_code == 200) {
    String response = http.getString();
    http.end();
    
    Serial.println("Server response: " + response);
    
    String current_hash = parseJsonValue(response, "hash");
    
    if (current_hash.length() > 0) {
      Serial.printf("Current hash: %s\n", current_hash.c_str());
      Serial.printf("Stored hash: %s\n", last_image_hash);
      
      if (strcmp(current_hash.c_str(), last_image_hash) == 0) {
        Serial.println("No image update needed");
        return false;
      }
      
      Serial.printf("New image detected: %s\n", current_hash.c_str());
      strncpy(last_image_hash, current_hash.c_str(), sizeof(last_image_hash) - 1);
      last_image_hash[sizeof(last_image_hash) - 1] = '\0';
      return true;
    } else {
      Serial.println("Failed to parse image hash");
    }
  }
  
  Serial.printf("Server request failed: HTTP %d\n", response_code);
  return false;
}

/**
 * Download and display new image via HTTP streaming
 * Uses dual-controller architecture for 1200x1600 resolution
 * 
 * @return true if successful, false on error
 */
bool updateDisplay() {
  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "%s/api/image/stream", server_url);
  http.begin(url);
  http.setTimeout(30000);
  
  int response_code = http.GET();
  if (response_code != 200) {
    Serial.printf("Image download failed: HTTP %d\n", response_code);
    http.end();
    return false;
  }
  
  Serial.println("Downloading image...");
  
  EPD_13IN3E_PowerOn();
  EPD_13IN3E_Init();
  WiFiClient* stream = http.getStreamPtr();
  uint8_t line_buffer[BYTES_PER_LINE_HALF];
  
  // Master controller (left half)
  EPD_13IN3E_BeginFrameM();
  size_t master_bytes = 0;
  for (int y = 0; y < EPD_HEIGHT; y++) {
    int bytes_read = stream->readBytes(line_buffer, BYTES_PER_LINE_HALF);
    if (bytes_read != BYTES_PER_LINE_HALF) {
      Serial.printf("Stream error at line %d\n", y);
      break;
    }
    EPD_13IN3E_WriteLineM(line_buffer);
    master_bytes += bytes_read;
    if ((y % 100) == 0) Serial.printf("Progress: %d%%\r", (y * 100) / EPD_HEIGHT);
  }
  EPD_13IN3E_EndFrameM();
  
  // Slave controller (right half)
  EPD_13IN3E_BeginFrameS();
  size_t slave_bytes = 0;
  for (int y = 0; y < EPD_HEIGHT; y++) {
    int bytes_read = stream->readBytes(line_buffer, BYTES_PER_LINE_HALF);
    if (bytes_read != BYTES_PER_LINE_HALF) {
      Serial.printf("Stream error at line %d\n", y);
      break;
    }
    EPD_13IN3E_WriteLineS(line_buffer);
    slave_bytes += bytes_read;
  }
  EPD_13IN3E_EndFrameS();
  
  http.end();
  
  // Verify complete data transfer
  size_t expected_bytes = (size_t)EPD_HEIGHT * BYTES_PER_LINE_HALF;
  if (master_bytes == expected_bytes && slave_bytes == expected_bytes) {
    Serial.println("\nRefreshing display...");
    EPD_13IN3E_RefreshNow();
    Serial.println("Display update complete");
    EPD_13IN3E_PowerOff();
    return true;
  } else {
    Serial.println("Incomplete data transfer");
    return false;
  }
}

/**
 * System initialization
 */
void setup() {
  Serial.begin(115200);
  
  // Configure watchdog for long operations
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 31000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  
  // Optimize power consumption
  setCpuFrequencyMhz(160);
  
  // Initialize hardware
  DEV_Module_Init();
  
  // Configure server URL
  snprintf(server_url, sizeof(server_url), "http://%s:%d", VPS_HOST, VPS_PORT);
  
  // Load WiFi settings
  loadWiFiCredentials();
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  
  if (wifi_configured && stored_ssid.length() > 0) {
    Serial.println("Connecting with saved credentials: " + stored_ssid);
    WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
  } else {
    Serial.printf("Connecting to: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  
  unsigned long start_time = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 10000) {
    delay(300);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi connected: %s\n", WiFi.localIP().toString().c_str());
    wifi_configured = true;
    
    // Enable power saving
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    Serial.println("WiFi power saving enabled");
  } else {
    Serial.println("\nWiFi connection failed");
    Serial.println("Check WiFiConfig.h settings");
    wifi_configured = false;
  }

  // Display boot screen
  EPD_13IN3E_PowerOn();
  EPD_13IN3E_Init();
  int battery_level = getBatteryLevel();
  EPD_13IN3E_ShowBootSplash(WIFI_SSID, VPS_PORT, battery_level);
  delay(1000);
  EPD_13IN3E_PowerOff();

  Serial.printf("Monitoring server: %s\n", server_url);
  Serial.println("Checking for updates every 18 seconds");
}

/**
 * Main application loop
 */
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    delay(5000);
    return;
  }
  
  // Check for image updates
  if (checkForNewImage()) {
    Serial.println("Updating display...");
    if (updateDisplay()) {
      Serial.println("Update successful");
    } else {
      Serial.println("Update failed");
    }
  }
  
  // Power management cycle
  esp_task_wdt_reset();
  
  Serial.println("System stabilization (3s)...");
  Serial.flush();
  delay(3000);
  esp_task_wdt_reset();
  
  Serial.println("Entering light sleep (15s)...");
  Serial.flush();
  delay(100);
  esp_sleep_enable_timer_wakeup(15000000);
  esp_light_sleep_start();
  
  delay(100);
  Serial.println("System wake-up");
  Serial.flush();
  esp_task_wdt_reset();
}