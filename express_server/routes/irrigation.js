const express = require('express');
const router = express.Router();
const mqttService = require('../services/mqtt');

// GET /api/irrigation/devices - Get all irrigation devices
router.get('/devices', (req, res) => {
  const devices = mqttService.getDevices();
  res.json({
    message: 'Irrigation devices',
    count: devices.length,
    devices: devices,
    timestamp: new Date().toISOString()
  });
});

// GET /api/irrigation/devices/:deviceId - Get specific device
router.get('/devices/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const device = mqttService.getDevice(deviceId);
  
  if (!device) {
    return res.status(404).json({
      error: 'Device not found',
      deviceId: deviceId
    });
  }
  
  res.json({
    message: `Device ${deviceId} details`,
    device: device,
    timestamp: new Date().toISOString()
  });
});

// GET /api/irrigation/telemetry - Get latest telemetry from all devices
router.get('/telemetry', (req, res) => {
  const telemetry = mqttService.getLatestTelemetry();
  res.json({
    message: 'Latest telemetry data',
    telemetry: telemetry,
    timestamp: new Date().toISOString()
  });
});

// GET /api/irrigation/telemetry/:deviceId - Get telemetry for specific device
router.get('/telemetry/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const device = mqttService.getDevice(deviceId);
  
  if (!device) {
    return res.status(404).json({
      error: 'Device not found',
      deviceId: deviceId
    });
  }
  
  res.json({
    message: `Latest telemetry for ${deviceId}`,
    deviceId: deviceId,
    telemetry: device.latestTelemetry || null,
    timestamp: new Date().toISOString()
  });
});

// GET /api/irrigation/history/:deviceId - Get telemetry history for specific device
router.get('/history/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const history = mqttService.getTelemetryHistory(deviceId);
  
  if (history.length === 0) {
    return res.status(404).json({
      error: 'No telemetry history found for device',
      deviceId: deviceId
    });
  }
  
  res.json({
    message: `Telemetry history for ${deviceId}`,
    deviceId: deviceId,
    count: history.length,
    history: history,
    timestamp: new Date().toISOString()
  });
});

// POST /api/irrigation/pump/:deviceId - Control pump for specific device
router.post('/pump/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const { action, duration } = req.body; // action: "start" or "stop", duration in milliseconds 
  
  if (!action || !['start', 'stop'].includes(action)) {
    return res.status(400).json({
      error: 'Action must be "start" or "stop"',
      example: { action: 'start', duration: 5000 }
    });
  }
  
  const commandTopic = `irrig/${deviceId}/cmd`;
  let commandMessage;
  
  if (action === 'start') {
    const waterDuration = duration || 5000; // Default 5 seconds
    commandMessage = JSON.stringify({ water_ms: waterDuration });
  } else if (action === 'stop') {
    commandMessage = JSON.stringify({ water_ms: 0 }); // Stop pump
  }
  
  const success = mqttService.publish(commandTopic, commandMessage);
  
  if (success) {
    res.json({
      message: `Pump ${action} command sent to ${deviceId}`,
      deviceId: deviceId,
      action: action,
      duration: action === 'start' ? (duration || 5000) : 0,
      topic: commandTopic,
      payload: commandMessage,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(500).json({
      error: 'Failed to send pump command. MQTT client may not be connected.'
    });
  }
});

// GET /api/irrigation/dashboard - Get dashboard overview
router.get('/dashboard', (req, res) => {
  const devices = mqttService.getDevices();
  const telemetry = mqttService.getLatestTelemetry();
  
  // Calculate summary stats
  const onlineDevices = devices.filter(d => d.status === 'online').length;
  const totalDevices = devices.length;
  const devicesWithTelemetry = Object.keys(telemetry).length;
  
  // Get alerts 
  const alerts = [];
  Object.entries(telemetry).forEach(([deviceId, data]) => {
    if (data.soil_pct < 10) {
      alerts.push({
        type: 'warning',
        device: deviceId,
        message: `Low soil moisture: ${data.soil_pct}%`
      });
    }
    if (data.pump_running) {
      alerts.push({
        type: 'info',
        device: deviceId,
        message: 'Pump is running'
      });
    }
  });
  
  res.json({
    message: 'Irrigation system dashboard',
    summary: {
      totalDevices,
      onlineDevices,
      devicesWithTelemetry,
      alertCount: alerts.length
    },
    devices: devices,
    latestTelemetry: telemetry,
    alerts: alerts,
    timestamp: new Date().toISOString()
  });
});

module.exports = router;
