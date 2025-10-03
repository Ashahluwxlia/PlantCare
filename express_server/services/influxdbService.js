const { InfluxDBClient, Point } = require('@influxdata/influxdb3-client');

class InfluxDBService {
  constructor() {
    this.client = null;
    this.bucket = process.env.INFLUXDB_BUCKET || 'plantcare-telemetry';
    this.host = process.env.INFLUXDB_HOST || 'https://us-east-1-1.aws.cloud2.influxdata.com';
    this.token = process.env.INFLUXDB_TOKEN;
    this.isConnected = false;
  }

  async connect() {
    try {
      if (!this.token) {
        console.error('❌ INFLUXDB_TOKEN not found in environment variables');
        return false;
      }

      this.client = new InfluxDBClient({
        host: this.host,
        token: this.token
      });

      this.isConnected = true;
      console.log('✅ InfluxDB client connected successfully');
      console.log(`📊 Bucket: ${this.bucket}`);
      console.log(`🌐 Host: ${this.host}`);
      
      return true;
    } catch (error) {
      console.error('❌ InfluxDB connection error:', error);
      this.isConnected = false;
      return false;
    }
  }

  async writeTelemetryData(deviceId, telemetryData) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return false;
    }

    try {
      const point = new Point('irrigation_telemetry')
        .setTag('device_id', deviceId)
        .setFloatField('soil_mv', telemetryData.soil_mv)
        .setFloatField('soil_pct', telemetryData.soil_pct)
        .setFloatField('light_mv', telemetryData.light_mv)
        .setFloatField('light_pct', telemetryData.light_pct)
        .setFloatField('temp_c', telemetryData.temp_c)
        .setFloatField('humidity_pct', telemetryData.humidity_pct)
        .setBooleanField('pump_running', telemetryData.pump_running)
        .setTimestamp(new Date(telemetryData.timestamp));

      await this.client.write(point, this.bucket);
      
      console.log(`📊 Telemetry data written to InfluxDB for ${deviceId}:`, {
        soil: `${telemetryData.soil_pct}%`,
        temp: `${telemetryData.temp_c}°C`,
        humidity: `${telemetryData.humidity_pct}%`,
        pump: telemetryData.pump_running ? 'ON' : 'OFF'
      });

      return true;
    } catch (error) {
      console.error('❌ Error writing telemetry data to InfluxDB:', error);
      return false;
    }
  }

  async writeDeviceStatus(deviceId, status, timestamp = new Date()) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return false;
    }

    try {
      const point = new Point('device_status')
        .setTag('device_id', deviceId)
        .setStringField('status', status)
        .setTimestamp(timestamp);

      await this.client.write(point, this.bucket);
      
      console.log(`📊 Device status written to InfluxDB: ${deviceId} = ${status}`);
      return true;
    } catch (error) {
      console.error('❌ Error writing device status to InfluxDB:', error);
      return false;
    }
  }

  async writePumpEvent(deviceId, action, duration = null, timestamp = new Date()) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return false;
    }

    try {
      const point = new Point('pump_events')
        .setTag('device_id', deviceId)
        .setTag('action', action)
        .setStringField('action', action)
        .setTimestamp(timestamp);

      if (duration !== null) {
        point.setIntField('duration_ms', duration);
      }

      await this.client.write(point, this.bucket);
      
      console.log(`📊 Pump event written to InfluxDB: ${deviceId} ${action}${duration ? ` (${duration}ms)` : ''}`);
      return true;
    } catch (error) {
      console.error('❌ Error writing pump event to InfluxDB:', error);
      return false;
    }
  }

  async queryTelemetry(deviceId = null, hours = 24) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return [];
    }

    try {
      let query = `SELECT *
                   FROM "device_telemetry"
                   WHERE time >= now() - interval '${hours} hours'
                   AND ("temp_c" IS NOT NULL OR "soil_pct" IS NOT NULL OR "pump_running" IS NOT NULL OR "light_pct" IS NOT NULL OR "humidity_pct" IS NOT NULL)`;

      if (deviceId) {
        query += ` AND "device_id" IN ('${deviceId}')`;
      }

      query += ` ORDER BY time DESC`;

      const rows = await this.client.query(query, this.bucket);
      const results = [];

      for await (const row of rows) {
        results.push({
          device_id: row.device_id,
          soil_mv: row.soil_mv,
          soil_pct: row.soil_pct,
          light_mv: row.light_mv,
          light_pct: row.light_pct,
          temp_c: row.temp_c,
          humidity_pct: row.humidity_pct,
          pump_running: row.pump_running,
          time: new Date(row.time)
        });
      }

      console.log(`📊 Retrieved ${results.length} telemetry records from InfluxDB`);
      return results;
    } catch (error) {
      console.error('❌ Error querying telemetry data from InfluxDB:', error);
      return [];
    }
  }

  async queryDeviceStatus(deviceId = null, hours = 24) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return [];
    }

    try {
      let query = `SELECT *
                   FROM "device_status"
                   WHERE time >= now() - interval '${hours} hours'`;

      if (deviceId) {
        query += ` AND "device_id" IN ('${deviceId}')`;
      }

      query += ` ORDER BY time DESC`;

      const rows = await this.client.query(query, this.bucket);
      const results = [];

      for await (const row of rows) {
        results.push({
          device_id: row.device_id,
          status: row.status,
          time: new Date(row.time)
        });
      }

      console.log(`📊 Retrieved ${results.length} status records from InfluxDB`);
      return results;
    } catch (error) {
      console.error('❌ Error querying device status from InfluxDB:', error);
      return [];
    }
  }

  async queryPumpEvents(deviceId = null, hours = 24) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return [];
    }

    try {
      let query = `SELECT * FROM 'pump_events' 
                   WHERE time >= now() - interval '${hours} hours'`;

      if (deviceId) {
        query += ` AND device_id = '${deviceId}'`;
      }

      query += ` ORDER BY time ASC`;

      const rows = await this.client.query(query, this.bucket);
      const results = [];

      for await (const row of rows) {
        results.push({
          device_id: row.device_id,
          action: row.action,
          duration_ms: row.duration_ms || null,
          time: new Date(row.time)
        });
      }

      console.log(`📊 Retrieved ${results.length} pump event records from InfluxDB`);
      return results;
    } catch (error) {
      console.error('❌ Error querying pump events from InfluxDB:', error);
      return [];
    }
  }

  async getLatestTelemetry(deviceId = null) {
    if (!this.client || !this.isConnected) {
      console.error('❌ InfluxDB client not connected');
      return [];
    }

    try {
      let query = `SELECT * FROM 'irrigation_telemetry'`;

      if (deviceId) {
        query += ` WHERE device_id = '${deviceId}'`;
      }

      query += ` ORDER BY time DESC LIMIT 1`;

      const rows = await this.client.query(query, this.bucket);
      const results = [];

      for await (const row of rows) {
        results.push({
          device_id: row.device_id,
          soil_mv: row.soil_mv,
          soil_pct: row.soil_pct,
          light_mv: row.light_mv,
          light_pct: row.light_pct,
          temp_c: row.temp_c,
          humidity_pct: row.humidity_pct,
          pump_running: row.pump_running,
          time: new Date(row.time)
        });
      }

      return results;
    } catch (error) {
      console.error('❌ Error querying latest telemetry from InfluxDB:', error);
      return [];
    }
  }

  getStatus() {
    return {
      connected: this.isConnected,
      bucket: this.bucket,
      host: this.host,
      hasToken: !!this.token
    };
  }

  async close() {
    if (this.client) {
      await this.client.close();
      this.isConnected = false;
      console.log('🔌 InfluxDB client disconnected');
    }
  }
}

// Create singleton instance
const influxdbService = new InfluxDBService();

module.exports = influxdbService;
