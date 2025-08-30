# ESP32 E-Ink Spectra 6-Color Display Controller

A high-performance, battery-optimized display controller for Waveshare 13.3" 6-color e-ink displays. Features HTTP polling for real-time image updates, advanced power management, and robust WiFi connectivity.

## Features

- **6-Color E-Ink Display**: Full support for Waveshare 13.3" displays (Black, White, Yellow, Red, Blue, Green)
- **HTTP Image Polling**: Automatic image updates via REST API with MD5 hash change detection
- **Power Optimized**: Light sleep mode reduces power consumption from 80mA to ~17mA average
- **Dual-Controller Architecture**: Supports 1200x1600 resolution through master/slave SPI controllers
- **Battery Monitoring**: Built-in LiPo battery level detection and display
- **WiFi Management**: Automatic reconnection and power-saving features
- **Robust Watchdog**: Advanced watchdog system prevents system hangs during long operations

## Hardware Requirements

### Core Components
- **Adafruit HUZZAH32 ESP32 Feather** - Main microcontroller
- **Waveshare 13.3" E-Paper HAT (6-color)** - Display module
- **LiPo Battery** - 3.7V minimum 1000mAh for reliable operation

### Wiring Diagram

```
ESP32 HUZZAH32    →    Waveshare 13.3" HAT
GPIO5  (SCK)      →    SCK
GPIO18 (MOSI)     →    MOSI  
GPIO33            →    CS_M (Master Chip Select)
GPIO15            →    CS_S (Slave Chip Select)
GPIO14            →    DC (Data/Command)
GPIO32            →    RST (Reset)
GPIO27            →    BUSY (Status)
GPIO21            →    PWR (Power Control, optional)
BAT               →    VCC (Battery 3.7V LiPo)
GND               →    GND
```

## Quick Start

### 1. Install Dependencies

Install the ESP32 Arduino Core and required libraries:
- ESP32 Arduino Core v3.3.0 or later
- Built-in WiFi and HTTP libraries

### 2. Configure WiFi and Server

Copy the configuration template:
```bash
cp WiFiConfig.h.example WiFiConfig.h
```

Edit `WiFiConfig.h` with your settings:
```cpp
const char* WIFI_SSID = "YourWiFiNetwork";
const char* WIFI_PASS = "YourWiFiPassword";
const char* VPS_HOST = "192.168.1.100";  // Your image server IP
const uint16_t VPS_PORT = 5001;           // Server port
```

### 3. Upload Firmware

1. Connect your ESP32 via USB
2. Select board: "Adafruit ESP32 Feather"
3. Upload the firmware

### 4. Set Up Image Server

The controller expects a REST API with these endpoints:

#### GET /api/image/info
Returns image metadata:
```json
{
  "hash": "abc123def456",
  "image_name": "display_image.jpg",
  "timestamp": 1704067200
}
```

#### GET /api/image/stream  
Returns raw image data in Waveshare 6-color format (960,000 bytes total):
- First 480,000 bytes: Master controller data (left half)
- Next 480,000 bytes: Slave controller data (right half)

## Configuration Options

### Power Management
- **Light Sleep Duration**: 15 seconds (configurable in code)
- **Stabilization Delay**: 3 seconds before sleep
- **Total Cycle Time**: ~18 seconds between image checks

### Network Settings
- **HTTP Timeout**: 5 seconds for info requests, 30 seconds for image downloads
- **WiFi Power Save**: Enabled (reduces consumption to ~10mA during active periods)
- **Connection Retry**: 10-second timeout with automatic retry

### Watchdog Configuration
- **Timeout**: 31 seconds (safe margin for all operations)
- **Automatic Reset**: Prevents system hangs during display updates

## Power Consumption

| Mode | Consumption | Notes |
|------|-------------|--------|
| Active (WiFi + Processing) | ~80mA | During HTTP requests and display updates |
| Light Sleep | ~0.8mA | 15-second sleep periods |
| **Average** | **~17mA** | 83% power reduction vs always-on |

With a 2000mAh LiPo battery:
- **Always-on operation**: ~25 hours
- **Optimized operation**: ~120+ hours

## API Integration Examples

> **Note:** For a complete server implementation with image conversion and dithering, see [inkscreen-web-server](https://github.com/stephanebhiri/inkscreen-web-server)

### Binary File Format for Dual-Controller Architecture
The 13.3" display uses two SPI controllers (Master and Slave) to handle the 1200x1600 resolution:
- **Master Controller**: Manages left half (pixels 0-599)
- **Slave Controller**: Manages right half (pixels 600-1199)

The `display_image.bin` file must be formatted as:
- **Total size**: 960KB (480KB per controller)
- **Structure**: Left half data (all 1600 rows) followed by right half data (all 1600 rows)
- **Encoding**: 2 pixels per byte, 4 bits per pixel supporting 6 colors

### Python Flask Server (Basic Example)
```python
from flask import Flask, jsonify, send_file
import hashlib
import os

app = Flask(__name__)

@app.route('/api/image/info')
def image_info():
    image_path = 'display_image.bin'
    if os.path.exists(image_path):
        with open(image_path, 'rb') as f:
            data = f.read()
            hash_md5 = hashlib.md5(data).hexdigest()[:12]
        
        return jsonify({
            'hash': hash_md5,
            'image_name': 'display_image.bin',
            'timestamp': int(os.path.getmtime(image_path))
        })
    
    return jsonify({'error': 'No image available'}), 404

@app.route('/api/image/stream')
def image_stream():
    return send_file('display_image.bin', mimetype='application/octet-stream')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)
```

### Node.js Express Server (Basic Example)
```javascript
const express = require('express');
const fs = require('fs');
const crypto = require('crypto');
const app = express();

app.get('/api/image/info', (req, res) => {
    const imagePath = 'display_image.bin';
    
    if (!fs.existsSync(imagePath)) {
        return res.status(404).json({error: 'No image available'});
    }
    
    const data = fs.readFileSync(imagePath);
    const hash = crypto.createHash('md5').update(data).digest('hex').substring(0, 12);
    const stats = fs.statSync(imagePath);
    
    res.json({
        hash: hash,
        image_name: 'display_image.bin',
        timestamp: Math.floor(stats.mtime.getTime() / 1000)
    });
});

app.get('/api/image/stream', (req, res) => {
    const imagePath = 'display_image.bin';
    res.setHeader('Content-Type', 'application/octet-stream');
    fs.createReadStream(imagePath).pipe(res);
});

app.listen(5001, '0.0.0.0', () => {
    console.log('Image server running on port 5001');
});
```

## Troubleshooting

### Common Issues

#### Display Not Updating
- Check WiFi connection and server accessibility
- Verify image format (must be Waveshare 6-color format)
- Ensure server returns correct HTTP status codes

#### High Power Consumption
- Verify light sleep is working (check serial output)
- Ensure WiFi power saving is enabled
- Check for proper watchdog configuration

#### Connection Issues
- Verify wiring matches pin definitions in DEV_Config.h
- Check SPI speed settings (try reducing to 8MHz)
- Ensure proper ground connections

#### Watchdog Resets
- Check that operations complete within 31-second timeout
- Verify watchdog resets are called appropriately
- Review serial output for timing issues

### Serial Monitor Output

Normal operation shows:
```
WiFi connected: 192.168.1.50
Monitoring server: http://192.168.1.100:5001
Server response: {"hash":"abc123","image_name":"test.bin","timestamp":1704067200}
No image update needed
System stabilization (3s)...
Entering light sleep (15s)...
System wake-up
```

## Development

### Building from Source
1. Clone repository
2. Open in Arduino IDE
3. Install ESP32 board package
4. Configure WiFiConfig.h
5. Upload to device

### Custom Image Formats
The controller expects binary data in Waveshare's packed 6-color format:
- 4 bits per pixel (2 pixels per byte)
- Left-to-right, top-to-bottom ordering
- Total size: 960,000 bytes (1200×1600÷2)

### Modifying Update Intervals
Edit the sleep duration in the main loop:
```cpp
esp_sleep_enable_timer_wakeup(15000000); // 15 seconds in microseconds
```

## License

This project is released under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure all changes maintain compatibility with the existing hardware setup and follow the established coding standards.

## Changelog

### Version 2.0 (January 2025)
- Complete rewrite with optimized power management
- Added light sleep functionality
- Improved watchdog system
- Enhanced error handling and robustness
- Professional code structure and documentation

### Version 1.0 (December 2024)
- Initial release with basic HTTP polling
- Waveshare 13.3" display support
- Basic WiFi connectivity