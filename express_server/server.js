const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
require('dotenv').config();

// Import services
const mqtt = require('./services/mqtt');
const influxdbService = require('./services/influxdbService');

const app = express();
const PORT = process.env.PORT || 3002;

// Middleware
app.use(helmet()); 
app.use(cors()); 
app.use(morgan('combined')); 
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true })); 

// Routes
app.use('/api/example', require('./routes/example'));
app.use('/api/mqtt', require('./routes/mqtt'));
app.use('/api/irrigation', require('./routes/irrigation'));


// Health check endpoint
app.get('/health', (req, res) => {
  res.status(200).json({
    status: 'OK',
    message: 'Server is running',
    services: {
      influxdb: influxdbService.getStatus(),
      mqtt: mqtt.getStatus()
    },
    timestamp: new Date().toISOString()
  });
});

// Root endpoint
app.get('/', (req, res) => {
  res.json({
    message: 'Welcome to Express Server',
    version: '1.0.0',
    endpoints: {
      health: '/health',
      example: '/api/example',
      mqtt: '/api/mqtt',
      irrigation: '/api/irrigation'
    }
  });
});

// 404 handler
app.use('*', (req, res) => {
  res.status(404).json({
    error: 'Route not found',
    message: `Cannot ${req.method} ${req.originalUrl}`
  });
});

// Global error handler
app.use((err, req, res, next) => {
  console.error('Error:', err.stack);
  
  res.status(err.status || 500).json({
    error: process.env.NODE_ENV === 'production' ? 'Internal Server Error' : err.message,
    ...(process.env.NODE_ENV !== 'production' && { stack: err.stack })
  });
});

// Initialize services
mqtt.connect();
influxdbService.connect();

// Start server
app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
  console.log(`Health check: http://localhost:${PORT}/health`);
  console.log(`Environment: ${process.env.NODE_ENV || 'development'}`);
});

// Graceful shutdown
process.on('SIGINT', async () => {
  console.log('\nShutting down gracefully...');
  mqtt.disconnect();
  await influxdbService.close();
  process.exit(0);
});

module.exports = app;
