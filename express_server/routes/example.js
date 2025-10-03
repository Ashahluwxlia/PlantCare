const express = require('express');
const router = express.Router();

// GET /api/example - Example endpoint
router.get('/', (req, res) => {
  res.json({
    message: 'This is an example API endpoint',
    data: {
      timestamp: new Date().toISOString(),
      method: 'GET',
      endpoint: '/api/example'
    }
  });
});

// POST /api/example - Example POST endpoint
router.post('/', (req, res) => {
  const { data } = req.body;
  
  res.status(201).json({
    message: 'Data received successfully',
    received: data,
    timestamp: new Date().toISOString()
  });
});

// GET /api/example/:id - Example parameterized endpoint
router.get('/:id', (req, res) => {
  const { id } = req.params;
  
  res.json({
    message: `Example endpoint with parameter`,
    id: id,
    timestamp: new Date().toISOString()
  });
});

module.exports = router;
