const mqtt = require('mqtt');
const influxdbService = require('./influxdbService');

class MQTTService {
  constructor() {
    this.client = null;
    this.isConnected = false;
    this.devices = new Map(); // Store device data
    this.telemetryHistory = new Map(); // Store recent telemetry (last 10 readings per device)
    
    this.options = {
      host: process.env.MQTT_HOST || 'localhost',
      port: parseInt(process.env.MQTT_PORT) || 8883,
      protocol: process.env.MQTT_PROTOCOL || 'mqtts',
      username: process.env.MQTT_USERNAME,
      password: process.env.MQTT_PASSWORD,
      // Additional options for HiveMQ Cloud
      clean: true,
      connectTimeout: 4000,
      reconnectPeriod: 1000,
    };
  }

  connect() {
    try {
      console.log('🔄 Connecting to MQTT broker...');
      console.log(`📡 Host: ${this.options.host}:${this.options.port}`);
      
      this.client = mqtt.connect(this.options);

      this.client.on('connect', () => {
        this.isConnected = true;
        console.log('✅ MQTT Client connected successfully');
        
        // Subscribe to default topics if specified
        const defaultTopics = process.env.MQTT_DEFAULT_TOPICS;
        if (defaultTopics) {
          const topics = defaultTopics.split(',');
          topics.forEach(topic => {
            this.subscribe(topic.trim());
          });
        }
      });

      this.client.on('error', (error) => {
        this.isConnected = false;
        console.error('❌ MQTT Connection error:', error);
      });

      this.client.on('close', () => {
        this.isConnected = false;
        console.log('🔌 MQTT Connection closed');
      });

      this.client.on('reconnect', () => {
        console.log('🔄 MQTT Reconnecting...');
      });

      this.client.on('message', (topic, message) => {
        console.log('📨 Received message:', {
          topic: topic,
          message: message.toString(),
          timestamp: new Date().toISOString()
        });
        
        // Emit custom event for message handling
        this.handleMessage(topic, message);
      });

    } catch (error) {
      console.error('❌ MQTT Setup error:', error);
    }
  }

  handleMessage(topic, message) {
    try {
      const messageStr = message.toString();
      const timestamp = new Date().toISOString();
      
      // Try to parse as JSON
      let parsedMessage;
      try {
        parsedMessage = JSON.parse(messageStr);
      } catch {
        parsedMessage = messageStr;
      }

      // Handle different topic patterns
      if (topic.includes('/status')) {
        this.handleDeviceStatus(topic, parsedMessage, timestamp);
      } else if (topic.includes('/telemetry')) {
        this.handleTelemetryData(topic, parsedMessage, timestamp);
      } else if (topic.includes('/claim/hello/')) {
        this.handleDeviceClaim(topic, parsedMessage, timestamp);
      } else {
        console.log('🔍 Processing unknown message:', {
          topic,
          data: parsedMessage,
          type: typeof parsedMessage
        });
      }

    } catch (error) {
      console.error('❌ Error handling message:', error);
    }
  }

  handleDeviceStatus(topic, message, timestamp) {
    // Extract device ID from topic: irrig/ABCD-1234/status (new format)
    const deviceMatch = topic.match(/irrig\/([^\/]+)\/status/);
    if (deviceMatch) {
      const deviceId = deviceMatch[1];
      const status = message; // "online" or "offline"
      
      if (!this.devices.has(deviceId)) {
        this.devices.set(deviceId, {});
      }
      
      const device = this.devices.get(deviceId);
      device.id = deviceId;
      device.status = status;
      device.lastStatusUpdate = timestamp;
      
      console.log(`📱 Device ${deviceId}: ${status}`);
      
      // Store device status in InfluxDB
      influxdbService.writeDeviceStatus(deviceId, status, new Date(timestamp));
    }
  }

  handleTelemetryData(topic, message, timestamp) {
    // Extract device ID from topic: irrig/ABCD-1234/telemetry (new format)
    const deviceMatch = topic.match(/irrig\/([^\/]+)\/telemetry/);
    if (deviceMatch && typeof message === 'object') {
      const deviceId = deviceMatch[1];
      
      if (!this.devices.has(deviceId)) {
        this.devices.set(deviceId, {});
      }
      
      // Update latest telemetry
      const device = this.devices.get(deviceId);
      device.id = deviceId;
      device.latestTelemetry = { ...message, timestamp };
      
      // Store in history (keep last 10 readings)
      if (!this.telemetryHistory.has(deviceId)) {
        this.telemetryHistory.set(deviceId, []);
      }
      
      const history = this.telemetryHistory.get(deviceId);
      history.unshift({ ...message, timestamp });
      if (history.length > 10) {
        history.pop();
      }
      
      console.log(`🌱 Telemetry from ${deviceId}:`, {
        soil: `${message.soil_pct}%`,
        temp: `${message.temp_c}°C`,
        humidity: `${message.humidity_pct}%`,
        pump: message.pump_running ? 'ON' : 'OFF'
      });

      // Store telemetry data in InfluxDB
      influxdbService.writeTelemetryData(deviceId, { ...message, timestamp });
    }
  }

  handleDeviceClaim(topic, message, timestamp) {
    // Extract device ID from topic: claim/hello/esp32-dce98e
    const deviceMatch = topic.match(/claim\/hello\/([^\/]+)/);
    if (deviceMatch && typeof message === 'object') {
      const deviceId = deviceMatch[1];
      
      if (!this.devices.has(deviceId)) {
        this.devices.set(deviceId, {});
      }
      
      const device = this.devices.get(deviceId);
      device.id = deviceId;
      device.claimCode = message.claimCode;
      device.firmware = message.fw;
      device.lastClaimUpdate = timestamp;
      
      console.log(`🔑 Device claim from ${deviceId}: ${message.claimCode} (FW: ${message.fw})`);
    }
  }

  // Get all devices data
  getDevices() {
    return Array.from(this.devices.values());
  }

  // Get specific device data
  getDevice(deviceId) {
    return this.devices.get(deviceId) || null;
  }

  // Get telemetry history for a device
  getTelemetryHistory(deviceId) {
    return this.telemetryHistory.get(deviceId) || [];
  }

  // Get latest telemetry for all devices
  getLatestTelemetry() {
    const result = {};
    for (const [deviceId, device] of this.devices.entries()) {
      if (device.latestTelemetry) {
        result[deviceId] = device.latestTelemetry;
      }
    }
    return result;
  }

  subscribe(topic, options = {}) {
    if (!this.client || !this.isConnected) {
      console.error('❌ MQTT client not connected');
      return false;
    }

    this.client.subscribe(topic, options, (error) => {
      if (error) {
        console.error(`❌ Failed to subscribe to topic "${topic}":`, error);
      } else {
        console.log(`✅ Subscribed to topic: "${topic}"`);
      }
    });
    return true;
  }

  unsubscribe(topic) {
    if (!this.client || !this.isConnected) {
      console.error('❌ MQTT client not connected');
      return false;
    }

    this.client.unsubscribe(topic, (error) => {
      if (error) {
        console.error(`❌ Failed to unsubscribe from topic "${topic}":`, error);
      } else {
        console.log(`✅ Unsubscribed from topic: "${topic}"`);
      }
    });
    return true;
  }

  publish(topic, message, options = {}) {
    if (!this.client || !this.isConnected) {
      console.error('❌ MQTT client not connected');
      return false;
    }

    // Convert message to string if it's an object
    const messageStr = typeof message === 'object' ? JSON.stringify(message) : String(message);

    this.client.publish(topic, messageStr, options, (error) => {
      if (error) {
        console.error(`❌ Failed to publish to topic "${topic}":`, error);
      } else {
        console.log(`✅ Published to topic "${topic}": ${messageStr}`);
      }
    });
    return true;
  }

  getStatus() {
    return {
      connected: this.isConnected,
      host: this.options.host,
      port: this.options.port,
      protocol: this.options.protocol,
      username: this.options.username ? '***' : null
    };
  }

  disconnect() {
    if (this.client) {
      this.client.end();
      this.isConnected = false;
      console.log('🔌 MQTT client disconnected');
    }
  }
}

// Create singleton instance
const mqttService = new MQTTService();

module.exports = mqttService;
