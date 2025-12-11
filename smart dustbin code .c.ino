#include <ESP8266WiFi.h>
#include <Servo.h>

const char* ssid = "...";
const char* password = "244466666";

WiFiServer server(80);

#define TRIG_PIN D5
#define ECHO_PIN D6
#define IR_PIN D7
#define SERVO_PIN D8

Servo servo;

long duration;
float distance;
int fillLevel;
const int maxDistance = 40;  // 40 cm = Empty
const int minDistance = 0;   // 0 cm = Full
const int binNumber = 17;    // <-- You can change to 18 if you like

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  
  servo.attach(SERVO_PIN);
  servo.write(0); // Lid closed initially

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("Web Server IP: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop() {
  // Ultrasonic distance measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  distance = duration * 0.034 / 2;

  if (distance == 0 || distance > maxDistance) distance = maxDistance;

  // Calculate fill level (invert scale)
  fillLevel = map(distance, maxDistance, minDistance, 0, 100);
  fillLevel = constrain(fillLevel, 0, 100);

  // IR sensor detection
  int irValue = digitalRead(IR_PIN);
  String irStatus = (irValue == LOW) ? "Hand Detected" : "No Hand";

  // Servo control
  if (irValue == LOW) {
    servo.write(90); // Open lid
    delay(1500);
  } else {
    servo.write(0); // Close lid
  }

  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) delay(1);

  client.readStringUntil('\r'); // Read request
  client.flush();

  // Create HTML response
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2'>";
  html += "<title>Smart Dustbin Web Server</title>";
  html += "<style>";
  html += "body { font-family: Arial; text-align:center; background-color:#fff; }";
  html += ".bar { width: 50%; height: 25px; border: 1px solid #000; margin:auto; }";
  html += ".fill { height: 25px; background-color: green; }";
  html += "</style></head><body>";
  html += "<h2>Smart Dustbin Web Server</h2>";
  html += "<h3>Bin No: " + String(binNumber) + "</h3>";
  html += "<h3>Bin Level: " + String(fillLevel) + "%</h3>";
  html += "<div class='bar'><div class='fill' style='width:" + String(fillLevel) + "%;'></div></div><br>";
  html += "<p>Distance: " + String(distance, 2) + " cm</p>";
  html += "<p>IR Sensor: " + irStatus + "</p>";
  html += "</body></html>";

  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  client.print(html);

  delay(100);
}
