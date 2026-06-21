#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <time.h>
#include <HTTPClient.h>

// -------- WIFI --------
const char* ssid = "Galaxy S24 E92C";
const char* password = "whynotyou";

// -------- TELEGRAM --------
String botToken = "8619689304:AAHmHMbN5PrhaTkq-Gd-L-MdCygXnGqrqRM";
String chatID = "6677765707";

// -------- CITY --------
String city = "Kolkata";

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 20, 4);

// -------- PINS --------
#define TURBIDITY_PIN 34
#define TDS_PIN 35
#define BUZZER 25
#define ONE_WIRE_BUS 4

// -------- WATER LEVEL SENSOR --------
#define TRIG_PIN 26
#define ECHO_PIN 27

#define TURBIDITY_THRESHOLD 2300

// -------- GPS --------
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// -------- TEMP --------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// -------- VARIABLES --------
float turbidityValue = 0;
float tdsValue = 0;
float temperature = 0;
float latitude = 0;
float longitude = 0;
float distanceCm = 0;

unsigned long lastSend = 0;

// -------- TELEGRAM FUNCTION --------
void sendMessage(String msg) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + botToken + "/sendMessage";
  String data = "chat_id=" + chatID + "&text=" + msg;

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.print(data);
}

// -------- DASHBOARD FUNCTION --------
void sendDataToDashboard(float turbidity, float tds, float temp, float distance, String status) {
  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();

    HTTPClient http;
    String serverPath = "https://iotwater-dashboard.vercel.app/api/water-data";

    http.begin(*client, serverPath);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"ntu\":" + String(turbidity) +
                         ",\"tds\":" + String(tds) +
                         ",\"temperature\":" + String(temp) +
                         ",\"waterDistance\":" + String(distance) +
                         ",\"status\":\"" + status + "\"" +
                         ",\"location\":\"" + city + "\"" +
                         ",\"latitude\":" + String(latitude, 6) +
                         ",\"longitude\":" + String(longitude, 6) + "}";

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("Dashboard HTTP Code: ");
    Serial.println(httpResponseCode);

    http.end();
    delete client;
  }
}

// -------- TIME FUNCTION --------
String getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "No Time";

  int hour = timeinfo.tm_hour;
  String ampm = "AM";

  if (hour >= 12) {
    ampm = "PM";
    if (hour > 12) hour -= 12;
  }

  if (hour == 0) hour = 12;

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d %s", hour, timeinfo.tm_min, ampm.c_str());

  return String(timeStr);
}

// -------- READ TURBIDITY --------
float readTurbidity() {
  long sum = 0;

  for (int i = 0; i < 100; i++) {
    sum += analogRead(TURBIDITY_PIN);
    delay(2);
  }

  return sum / 100.0;
}

// -------- READ TDS --------
float readTDS() {
  long sum = 0;

  for (int i = 0; i < 100; i++) {
    sum += analogRead(TDS_PIN);
    delay(2);
  }

  float adcValue = sum / 100.0;
  float voltage = adcValue * 3.3 / 4095.0;

  float tds = (133.42 * voltage * voltage * voltage
             - 255.86 * voltage * voltage
             + 857.39 * voltage) * 0.5;

  return tds;
}

// -------- READ DISTANCE --------
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  float distance = duration * 0.0343 / 2.0;

  if (distance < 5 || distance > 400) return -1;

  return distance;
}

// -------- FILTERED DISTANCE --------
float readFilteredDistance() {
  float sum = 0;
  int count = 0;

  for (int i = 0; i < 10; i++) {
    float d = readDistance();

    if (d > 0) {
      sum += d;
      count++;
    }

    delay(50);
  }

  if (count > 0) {
    return sum / count;
  } else {
    return -1;
  }
}

void setup() {
  Serial.begin(115200);

  gpsSerial.begin(115200, SERIAL_8N1, 16, 17);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(BUZZER, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  tempSensor.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  lcd.print("Connecting WiFi");

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.print("WiFi Connected");
  } else {
    Serial.println();
    Serial.println("WiFi Failed");
    Serial.print("Status Code: ");
    Serial.println(WiFi.status());

    lcd.clear();
    lcd.print("WiFi Failed");
  }

  delay(1500);
  lcd.clear();

  configTime(19800, 0, "pool.ntp.org");
}

void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }

  tempSensor.requestTemperatures();
  temperature = tempSensor.getTempCByIndex(0);

  turbidityValue = readTurbidity();

  String turbidityStatus;

  if (turbidityValue > TURBIDITY_THRESHOLD) {
    turbidityStatus = "CLEAN WATER";
  } else {
    turbidityStatus = "DIRTY WATER";
  }

  tdsValue = readTDS();

  String tdsStatus;

  if (tdsValue < 150) {
    tdsStatus = "Excellent Water";
  }
  else if (tdsValue < 300) {
    tdsStatus = "Good Water";
  }
  else if (tdsValue < 500) {
    tdsStatus = "Acceptable Water";
  }
  else if (tdsValue < 900) {
    tdsStatus = "Poor Water";
  }
  else {
    tdsStatus = "Very Poor Water";
  }

  distanceCm = readFilteredDistance();

  String waterLevelStatus;

  if (distanceCm < 0) {
    waterLevelStatus = "No Valid Reading";
  } else {
    waterLevelStatus = "Distance OK";
  }

  String status;

  if (turbidityStatus == "CLEAN WATER" && tdsValue < 300) {
    status = "CLEAN";
  }
  else if (turbidityStatus == "CLEAN WATER" && tdsValue < 500) {
    status = "ACCEPTABLE";
  }
  else {
    status = "DIRTY";
  }

  digitalWrite(BUZZER, status == "DIRTY" ? HIGH : LOW);

  String currentTime = getTime();

  lcd.setCursor(0, 0);
  lcd.print(city + " " + currentTime + "   ");

  lcd.setCursor(0, 1);
  lcd.print("Turb:");
  lcd.print(turbidityValue, 0);
  lcd.print(" TDS:");
  lcd.print(tdsValue, 0);

  lcd.setCursor(0, 2);
  lcd.print("Temp:");
  lcd.print(temperature, 1);
  lcd.print("C D:");
  if (distanceCm > 0) {
    lcd.print(distanceCm, 0);
    lcd.print("cm ");
  } else {
    lcd.print("No ");
  }

  lcd.setCursor(0, 3);
  lcd.print(status + "              ");

  if (millis() - lastSend > 10000) {

    String link = "https://maps.google.com/?q=" +
                  String(latitude, 6) + "," +
                  String(longitude, 6);

    String msg = "City: " + city + "\n";
    msg += "Time: " + currentTime + "\n";
    msg += "Temp: " + String(temperature, 1) + " C\n";
    msg += "Turbidity Raw: " + String(turbidityValue, 0) + "\n";
    msg += "Turbidity Status: " + turbidityStatus + "\n";
    msg += "TDS: " + String(tdsValue, 0) + " ppm\n";
    msg += "TDS Status: " + tdsStatus + "\n";

    if (distanceCm > 0) {
      msg += "Filtered Distance: " + String(distanceCm, 1) + " cm\n";
    } else {
      msg += "Water Level Sensor: No Valid Reading\n";
    }

    msg += "Final Status: " + status + "\n";
    msg += link;

    sendMessage(msg);
    sendDataToDashboard(turbidityValue, tdsValue, temperature, distanceCm, status);

    lastSend = millis();
  }

  Serial.print("Turbidity Avg: ");
  Serial.print(turbidityValue);
  Serial.print(" | ");
  Serial.print(turbidityStatus);

  Serial.print(" | TDS: ");
  Serial.print(tdsValue, 0);
  Serial.print(" ppm | ");
  Serial.print(tdsStatus);

  Serial.print(" | Temp: ");
  Serial.print(temperature);

  Serial.print(" | ");

  if (distanceCm > 0) {
    Serial.print("Filtered Distance: ");
    Serial.print(distanceCm);
    Serial.print(" cm");
  } else {
    Serial.print("No Valid Reading");
  }

  Serial.print(" | Final: ");
  Serial.print(status);

  Serial.print(" | Lat: ");
  Serial.print(latitude, 6);

  Serial.print(" | Lng: ");
  Serial.println(longitude, 6);

  Serial.println("----------------------");

  delay(1000);
}
