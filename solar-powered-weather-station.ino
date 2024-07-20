#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <FS.h>
#include <SPIFFS.h>

// WiFi credentials
const char* ssid = "Techno SAP";
const char* password = "12345678";

// Create an instance of the server
WebServer server(80);

// DHT sensor
#define DHTPIN 4                    // DHT11 sensor connected to GPIO 4
#define DHTTYPE DHT11               // DHT11 sensor type
DHT dht(DHTPIN, DHTTYPE);

// Battery measurement
const float VCC = 3.3;
const float R1 = 100.0;
const float R2 = 100.0;
const float correctionFactor = 1.080;
const float lowBat = 2.9;
const float fullBat = 4.2;
const int batt_pin = 35;

// Solar panel measurement
const float lowSolar = 0.0;
const float fullSolar = 6.0;
const int solar_pin = 34;
const float solarCorrectionFactor = 0.9157;

// Function declarations
void handleRoot();
void handleTemperature();
void handleHumidity();
void handleBattery();
void handleSolar();
void handleData();

// Helper function to save data
void saveData(float temperature, float humidity, float batteryVoltage, float solarVoltage);

// Disclaimer and verification
const char* disclaimer = "/*\n * This code is developed by Techno SAP. \n * Removing or modifying the name 'Techno SAP' will cause the code to stop functioning.\n */";
const char* verification = "Techno SAP";

void setup() {
  // Check for the presence of the verification string
  if (strstr(disclaimer, verification) == NULL) {
    Serial.println("Verification failed. Code will not run.");
    while (true);
  }

  Serial.begin(9600);
  dht.begin();                      // Initialize DHT sensor

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize the server and define routes
  server.on("/", handleRoot);
  server.on("/temperature", handleTemperature);
  server.on("/humidity", handleHumidity);
  server.on("/battery", handleBattery);
  server.on("/solar", handleSolar);
  server.on("/data", handleData);  // New route to handle data
  server.begin();
}

void loop() {
  server.handleClient();

  // Collect and average battery voltage samples
  int i;
  float batt_volt = 0;
  const int numSamples = 100;

  for (i = 0; i < numSamples; i++) {
    int analogValue = analogRead(batt_pin);
    float voltage = analogValue * (VCC / 4095.0);
    batt_volt += voltage * ((R1 + R2) / R2);
    delay(5);
  }
  batt_volt = (batt_volt / numSamples) * correctionFactor;

  // Calculate battery percentage
  float batteryPercent = ((batt_volt - lowBat) / (fullBat - lowBat)) * 100;
  batteryPercent = constrain(batteryPercent, 0, 100); // Ensure percentage is within 0-100 range

  // Collect and average solar panel voltage samples
  float solar_volt = 0;
  for (i = 0; i < numSamples; i++) {
    int analogValue = analogRead(solar_pin);
    float voltage = analogValue * (VCC / 4095.0);
    solar_volt += voltage * ((R1 + R2) / R2);
    delay(5);
  }
  solar_volt = (solar_volt / numSamples) * solarCorrectionFactor;

  // Calculate solar panel percentage
  float solarPercent = ((solar_volt - lowSolar) / (fullSolar - lowSolar)) * 100;
  solarPercent = constrain(solarPercent, 0, 100); // Ensure percentage is within 0-100 range

  // Read temperature and humidity from DHT11 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Print battery voltage and percentage
  Serial.print("Battery: ");
  Serial.print(batt_volt, 3);
  Serial.print("V ");
  Serial.print(batteryPercent, 2);
  Serial.println("%");

  // Print solar panel voltage and percentage power
  Serial.print("Solar Panel: ");
  Serial.print(solar_volt, 3);
  Serial.print("V ");
  Serial.print(solarPercent, 2);
  Serial.println("%");

  // Print temperature and humidity
  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(humidity, 1);
  Serial.println("%");

  // Save data to file every minute
  static unsigned long lastSaveTime = 0;
  if (millis() - lastSaveTime >= 60000) {
    lastSaveTime = millis();
    saveData(temperature, humidity, batt_volt, solar_volt);
  }

  delay(1000);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Weather Station</title>
<style>
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    text-align: center;
    background-color: #e0e0e0;
    margin: 0;
    padding: 20px;
  }
  .dashboard {
    background-color: #ffffff;
    border-radius: 15px;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
    padding: 30px;
    max-width: 800px;
    margin: auto;
  }
  .heading {
    font-size: 28px;
    font-weight: bold;
    margin-bottom: 20px;
    color: #333333;
  }
  .emojis {
    font-size: 40px;
    margin-bottom: 20px;
  }
  .percentage {
    font-size: 24px;
    color: #555555;
  }
  .dashboard-container {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 25px;
    width: 90%;
    margin: 40px auto;
    padding: 30px;
    border: 1px solid #ccc;
    border-radius: 15px;
    background-color: #ffffff;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
  }
  .chart-container {
    border: 1px solid #ddd;
    border-radius: 15px;
    padding: 20px;
    background-color: #fafafa;
    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.05);
  }
  .chart-container h3 {
    font-size: 20px;
    color: #444444;
    margin-bottom: 15px;
  }
  .button {
    display: inline-block;
    padding: 10px 20px;
    font-size: 16px;
    font-weight: bold;
    color: #ffffff;
    background-color: #007bff;
    border: none;
    border-radius: 5px;
    text-decoration: none;
    cursor: pointer;
    margin-top: 20px;
  }
  .button:hover {
    background-color: #0056b3;
  }
</style>
<script src="https://code.highcharts.com/highcharts.js"></script>
</head>
<body>
  <div class="dashboard">
    <div class="heading">
      Weather Station
    </div>
    <div class="emojis">
      ☀️ <span id="solarPercentage" class="percentage">0%</span> &nbsp;&nbsp;&nbsp; 🔋 <span id="batteryPercentage" class="percentage">0%</span>
    </div>
    <a href="/data" class="button">View Data</a>
  </div>

  <div class="dashboard-container">
    <!-- Battery Voltage Chart -->
    <div class="chart-container">
      <h3><span>&#x1F50B;</span> Battery Voltage</h3>
      <div id="batteryChart"></div>
    </div>

    <!-- Solar Panel Voltage Chart -->
    <div class="chart-container">
      <h3><span>&#x1F31F;</span> Solar Panel Voltage</h3>
      <div id="solarChart"></div>
    </div>

    <!-- Temperature Chart -->
    <div class="chart-container">
      <h3><span>&#x1F321;</span> Temperature</h3>
      <div id="temperatureChart"></div>
    </div>

    <!-- Humidity Chart -->
    <div class="chart-container">
      <h3><span>&#x1F4A7;</span> Humidity</h3>
      <div id="humidityChart"></div>
    </div>
  </div>

  <script>
    function updateData() {
      fetch('/battery')
        .then(response => response.json())
        .then(data => {
          const percentage = ((data.voltage - 2.9) / (4.2 - 2.9)) * 100;
          document.getElementById('batteryPercentage').textContent = percentage.toFixed(2) + '%';
          updateChart(batteryChart, data.timestamp, data.voltage);
        });

      fetch('/solar')
        .then(response => response.json())
        .then(data => {
          const percentage = ((data.voltage - 0.0) / (6.0 - 0.0)) * 100;
          document.getElementById('solarPercentage').textContent = percentage.toFixed(2) + '%';
          updateChart(solarChart, data.timestamp, data.voltage);
        });

      fetch('/temperature')
        .then(response => response.json())
        .then(data => {
          updateChart(temperatureChart, data.timestamp, data.temperature);
        });

      fetch('/humidity')
        .then(response => response.json())
        .then(data => {
          updateChart(humidityChart, data.timestamp, data.humidity);
        });
    }

    function updateChart(chart, timestamp, value) {
      chart.series[0].addPoint([timestamp, value], true, chart.series[0].data.length >= 20);
    }

    const batteryChart = Highcharts.chart('batteryChart', {
      chart: { type: 'spline' },
      title: { text: null },
      xAxis: { type: 'datetime' },
      yAxis: { title: { text: 'Voltage (V)' }, min: 2.9, max: 4.2 },
      series: [{ name: 'Battery Voltage', data: [] }]
    });

    const solarChart = Highcharts.chart('solarChart', {
      chart: { type: 'spline' },
      title: { text: null },
      xAxis: { type: 'datetime' },
      yAxis: { title: { text: 'Voltage (V)' }, min: 0.0, max: 6.0 },
      series: [{ name: 'Solar Panel Voltage', data: [] }]
    });

    const temperatureChart = Highcharts.chart('temperatureChart', {
      chart: { type: 'spline' },
      title: { text: null },
      xAxis: { type: 'datetime' },
      yAxis: { title: { text: 'Temperature (°C)' }, min: 0, max: 50 },
      series: [{ name: 'Temperature', data: [] }]
    });

    const humidityChart = Highcharts.chart('humidityChart', {
      chart: { type: 'spline' },
      title: { text: null },
      xAxis: { type: 'datetime' },
      yAxis: { title: { text: 'Humidity (%)' }, min: 0, max: 100 },
      series: [{ name: 'Humidity', data: [] }]
    });

    updateData();
    setInterval(updateData, 10000);
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleTemperature() {
  float temperature = dht.readTemperature();
  String json = "{\"temperature\": " + String(temperature) + "}";
  server.send(200, "application/json", json);
}

void handleHumidity() {
  float humidity = dht.readHumidity();
  String json = "{\"humidity\": " + String(humidity) + "}";
  server.send(200, "application/json", json);
}

void handleBattery() {
  float batt_volt = 0;
  const int numSamples = 100;
  for (int i = 0; i < numSamples; i++) {
    int analogValue = analogRead(batt_pin);
    float voltage = analogValue * (VCC / 4095.0);
    batt_volt += voltage * ((R1 + R2) / R2);
    delay(5);
  }
  batt_volt = (batt_volt / numSamples) * correctionFactor;
  String json = "{\"voltage\": " + String(batt_volt) + "}";
  server.send(200, "application/json", json);
}

void handleSolar() {
  float solar_volt = 0;
  const int numSamples = 100;
  for (int i = 0; i < numSamples; i++) {
    int analogValue = analogRead(solar_pin);
    float voltage = analogValue * (VCC / 4095.0);
    solar_volt += voltage * ((R1 + R2) / R2);
    delay(5);
  }
  solar_volt = (solar_volt / numSamples) * solarCorrectionFactor;
  String json = "{\"voltage\": " + String(solar_volt) + "}";
  server.send(200, "application/json", json);
}

void handleData() {
  File file = SPIFFS.open("/data.txt", "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file for reading");
    return;
  }

  String data = file.readString();
  file.close();
  server.send(200, "text/plain", data);
}

void saveData(float temperature, float humidity, float batteryVoltage, float solarVoltage) {
  File file = SPIFFS.open("/data.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  String data = String(millis()) + "," + String(temperature) + "," + String(humidity) + "," + String(batteryVoltage) + "," + String(solarVoltage) + "\n";
  file.print(data);
  file.close();
}
