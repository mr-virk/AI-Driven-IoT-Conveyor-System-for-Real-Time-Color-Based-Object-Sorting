#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

//Pin Definations////////////////
// Motor A
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;
//Ultasonic
#define ScanArea 16
#define ThrowArea 17
//Servo
int servoPin = 18;
int pos = 0;  // variable to store the servo position
#define ConveyorLed 33
#define WifiLed 12
#define MQTTLedPin 14
#define ProcessLed 25

//Variables////////////////
// WiFi
const char *ssid = "WIFI";             // Enter your WiFi name
const char *password = "PASS";  // Enter WiFi password
// MQTT Broker
const char *mqtt_broker = "xyz";
const char *mqtt_topic = "topic";
const char *mqtt_username = "user";
const char *mqtt_password = "pass";
const int mqtt_port = 8883;
String message;  //to store recived payload

int sa = 0;
int ta = 0;

Servo myservo;
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Setting PWM properties for motor
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 100;

// Flags ////////////////
bool WifiStatus = false;
bool MQTTStatus = false;
bool ProcessingStatus = false;

// For Backgruound Tasks
unsigned long previousTime = 0;
const unsigned long interval = 1000;  // 1000 milliseconds = 1 Second

// Root CA Certificate
// Load DigiCert Global Root G2
const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// Core Functions
void setup() {
  Serial.begin(115200);

  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  // configure LEDC PWM for Motor
  ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel);
  // LED
  pinMode(WifiLed, OUTPUT);
  pinMode(MQTTLedPin, OUTPUT);
  pinMode(ProcessLed, OUTPUT);
  pinMode(ConveyorLed, OUTPUT);
  pinMode(ScanArea, INPUT);
  pinMode(ThrowArea, INPUT);

  // Initial Start
  digitalWrite(WifiLed, LOW);
  digitalWrite(MQTTLedPin, LOW);
  digitalWrite(ProcessLed, LOW);
  digitalWrite(ConveyorLed, LOW);

  // Initialize LCD
  lcd.init();       // Start I2C LCD
  lcd.backlight();  // Turn Backlight On
  lcd.clear();      // Clear LCD Screen

  lcd.setCursor(0, 0);
  lcd.print("   Starting  ");
  lcd.setCursor(0, 1);
  lcd.print(">-------------<");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(">-------------<");
  lcd.setCursor(0, 1);
  lcd.print(">-------------<");
  delay(1500);
  lcd.clear();

  // Initialize Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);  // standard 50 Hz servo
  myservo.attach(servoPin, 500, 2500);

  connectToWiFi();
  // Set Root CA certificate
  esp_client.setCACert(ca_cert);

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTT();
  digitalWrite(ConveyorLed, HIGH);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
}

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTT();
  }
  //mqtt_client.loop();

  // run core tasks
  RunTasks();
}

// Functions
void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Connecting to ");
  lcd.setCursor(0, 1);
  lcd.print(" WiFi -> MQTT ");

  delay(1000);

  digitalWrite(WifiLed, LOW);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  digitalWrite(WifiLed, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Connected to ");
  lcd.setCursor(0, 1);
  lcd.print(" WiFi Internet");

  delay(2000);
}

void connectToMQTT() {
  while (!mqtt_client.connected()) {
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s...\n", client_id.c_str());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Connecting to ");
    lcd.setCursor(0, 1);
    lcd.print(" MQTT Broker...");

    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);

      digitalWrite(MQTTLedPin, HIGH);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Connected to ");
      lcd.setCursor(0, 1);
      lcd.print(" MQTT Broker ");
      delay(1000);
      notifications();
      //mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP32 ^^");  // Publish message upon connection
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds.");
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  message = "";  // Clear the previous message
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  Serial.println("\n-----------------------");
}

void RunTasks() {
  unsigned long currentTime = millis();
  if (currentTime - previousTime >= interval) {
    previousTime = currentTime;
    message = "";

    // Tasks to execute every 1 second
    CoreFunction();
  }
}

void CoreFunction() {
  do {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    digitalWrite(ConveyorLed, HIGH);
  } while (digitalRead(ScanArea) != 0);

    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);

    digitalWrite(ConveyorLed, LOW);

    digitalWrite(ProcessLed, HIGH);

    do {
      //void mqttCallback(char *topic, byte *payload, unsigned int length);
      mqtt_client.loop();
    } while (message == "");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<COLOR DETECTED>");
    lcd.setCursor(5, 1);
    lcd.print(message);
    delay(15);

    digitalWrite(ProcessLed, LOW);

    if (message == "Green") {
      colorGreen();
    } else if (message == "Red") {
      colorRed();
    } else if (message == "Blue") {
      colorBlue();
    }

    notifications();
}

void colorGreen() {
  for (pos = 180; pos >= 0; pos -= 1) {  // goes from 180 degrees to 0 degrees
    myservo.write(pos);                  // tell servo to go to position in variable 'pos'
    delay(15);                           // waits 15ms for the servo to reach the position
  }
  Serial.println("Servo @ 0 degrees...");
  delay(1000);

  do {
    Serial.println("Moving forward...");
    digitalWrite(ConveyorLed, HIGH);
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
  } while (digitalRead(ThrowArea) != 0);

  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);

  Serial.println("Stopped...");
  digitalWrite(ConveyorLed, LOW);

  delay(1000);

  for (pos = 0; pos <= 180; pos += 1) {  // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);  // tell servo to go to position in variable 'pos'
    delay(15);           // waits 15ms for the servo to reach the position
  }
  Serial.println("Servo @ 180 degrees...");
  delay(1000);

  message = "";
}

void colorRed() {
  for (pos = 0; pos <= 180; pos += 1) {  // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);  // tell servo to go to position in variable 'pos'
    delay(15);           // waits 15ms for the servo to reach the position
  }

  Serial.println("Servo @ 180 degrees...");
  delay(1000);

  do {
    Serial.println("Moving forward...");
    digitalWrite(ConveyorLed, HIGH);
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
  } while (digitalRead(ThrowArea) != 0);

  Serial.println("Stopped...");
  digitalWrite(ConveyorLed, LOW);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);

  delay(1000);

  for (pos = 180; pos >= 0; pos -= 1) {  // goes from 180 degrees to 0 degrees
    myservo.write(pos);                  // tell servo to go to position in variable 'pos'
    delay(15);                           // waits 15ms for the servo to reach the position
  }

  Serial.println("Servo @ 0 degrees...");
  delay(1000);

  message = "";
}

void colorBlue() {
  for (pos = 0; pos <= 180; pos += 1) {  // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);  // tell servo to go to position in variable 'pos'
    delay(15);           // waits 15ms for the servo to reach the position
  }

  Serial.println("Servo @ 180 degrees...");
  delay(1000);

  Serial.println("Moving forward...");
  digitalWrite(ConveyorLed, HIGH);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);

  delay(2000);

  message = "";
}

void notifications() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("<-WIFI    MQTT->");
  lcd.setCursor(0, 1);
  lcd.print("<-AI  CONVEYOR->");
}
