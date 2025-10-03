const express = require('express');
const router = express.Router();
const mqttService = require('../services/mqtt');

// GET /api/mqtt/status - Get MQTT connection status
router.get('/status', (req, res) => {
  const status = mqttService.getStatus();
  res.json({
    message: 'MQTT connection status',
    status: status,
    timestamp: new Date().toISOString()
  });
});

// POST /api/mqtt/publish - Publish a message to a topic
router.post('/publish', (req, res) => {
  const { topic, message, options } = req.body;
  
  if (!topic || message === undefined) {
    return res.status(400).json({
      error: 'Topic and message are required',
      example: {
        topic: 'my/test/topic',
        message: 'Hello World',
        options: { qos: 1, retain: false }
      }
    });
  }
  
  const success = mqttService.publish(topic, message, options || {});
  
  if (success) {
    res.json({
      message: 'Message published successfully',
      topic: topic,
      data: message,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(500).json({
      error: 'Failed to publish message. MQTT client may not be connected.'
    });
  }
});

// POST /api/mqtt/subscribe - Subscribe to a topic
router.post('/subscribe', (req, res) => {
  const { topic, options } = req.body;
  
  if (!topic) {
    return res.status(400).json({
      error: 'Topic is required',
      example: {
        topic: 'my/test/topic',
        options: { qos: 1 }
      }
    });
  }
  
  const success = mqttService.subscribe(topic, options || {});
  
  if (success) {
    res.json({
      message: 'Subscribed to topic successfully',
      topic: topic,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(500).json({
      error: 'Failed to subscribe to topic. MQTT client may not be connected.'
    });
  }
});

// POST /api/mqtt/unsubscribe - Unsubscribe from a topic
router.post('/unsubscribe', (req, res) => {
  const { topic } = req.body;
  
  if (!topic) {
    return res.status(400).json({
      error: 'Topic is required',
      example: {
        topic: 'my/test/topic'
      }
    });
  }
  
  const success = mqttService.unsubscribe(topic);
  
  if (success) {
    res.json({
      message: 'Unsubscribed from topic successfully',
      topic: topic,
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(500).json({
      error: 'Failed to unsubscribe from topic. MQTT client may not be connected.'
    });
  }
});

// GET /api/mqtt/test - Test endpoint to publish a test message
router.get('/test', (req, res) => {
  const testTopic = 'test/api/message';
  const testMessage = {
    message: 'Hello from Express API',
    timestamp: new Date().toISOString(),
    source: 'express-server'
  };
  
  const success = mqttService.publish(testTopic, testMessage);
  
  if (success) {
    res.json({
      message: 'Test message sent successfully',
      topic: testTopic,
      data: testMessage
    });
  } else {
    res.status(500).json({
      error: 'Failed to send test message. MQTT client may not be connected.'
    });
  }
});

module.exports = router;
