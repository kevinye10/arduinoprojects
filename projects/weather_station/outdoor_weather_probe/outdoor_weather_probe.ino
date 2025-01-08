/*
  Name: outdoor_weather_probe.ino
  Created: 01/05/2025
  Author: Kevin Ye

  DESIGNED USING: 
    ARDUINO NANO;  
    DHT22 Temp & Weather Sensor
      (+)-->5V
      out-->D4
      (-)-->GND
    HM-10 BLE
      VCC-->5V
      GND-->GND
      TXD-->D2
      RXD-->D3
*/

#include <DHT.h>
#include <SoftwareSerial.h>

// dht sensor input to arduino
#define DHT_TYPE DHT22
#define PIN_DHT 4
#define HM10_TX 2
#define HM10_RX 3

// initialize dht sensor
DHT dht(PIN_DHT, DHT_TYPE);

// struct because cool and practice!
struct weatherData {
  float temperature;
  float humidity;
};
weatherData *data = new weatherData();

// initialize softwareSerial for HM-10 communication
SoftwareSerial HM10(HM10_TX, HM10_RX);
// hm10 handles UUID, characteristic, BLE operations internally

// for maintaining 1 second heartbeats
unsigned prevMillis = 0;
unsigned heartbeatRate = 1000; // 1000 ms = 1 s

// runs once
void setup() {
  // Initialize Serial comms
  Serial.begin(9600);
  HM10.begin(9600); // hm10 default baud rate
  delay(1000); // Wait for HM-10 to initialize

  // Init DHT sensor
  dht.begin();

  // send attention (AT) commands to configure HM-10
  HM10.print("AT+NAMEWEATHERPROBE"); // set device name (max 12 chars)
  delay(100);
  HM10.print("AT+RESET");
  delay(1000); // Wait for reset

  // pinmode for heartbeat
  pinMode(LED_BUILTIN, OUTPUT);

  delay(200);
  Serial.println("Nano Weather Probe initialized");
}

// loop
String packet;
void loop() {
  // // debug -- print hm10 output to serial and send serial input to hm10
  // if (HM10.available()) {
  //   char c = HM10.read();
  //   Serial.write(c);
  // }
  // if (Serial.available()) {
  //   char c = Serial.read();
  //   HM10.write(c);
  // }

  // obtain updated weather information
  updateWeatherData(data);

  // debug: make sure data is not NAN
  if (data->temperature == -1 || data->humidity == -1) {
    Serial.println("Failed to read DHT sensor");
    Serial.println("Temperature: " + String(data->temperature) + "\t Humidity: " + String(data->humidity));
  }

  // format packet as "T:tt.t,H:hh". Default: Celsius, Percent
  // since this is under 20 byte BLE packet limit, HM10 handles for us
  packet = "T:" + String(data->temperature, 1) + ",H:" + String(data->humidity, 0) + '\0'; // include null-terminator '\0'

  // send packet
  HM10.print(packet);

  // more debug for packet sent
  Serial.println("Sent " + packet);

  heartBeat(LED_BUILTIN);
  delay(1000);
}


/*
  UPDATEWEATHERDATA: Probes DHT temperature and humidy sensor, places float data in dynamically allocated struct.
*/
void updateWeatherData(weatherData *d_) {
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // check nan
  d_->temperature = isnan(temperature) ? -1 : temperature;
  d_->humidity = isnan(humidity) ? -1 : humidity;
}

/*
  HEARTBEAT: Maintains LED blinks based on heartbeatRate
*/
void heartBeat(uint8_t pin) {
  unsigned currMillis = millis();
  if (currMillis - prevMillis >= heartbeatRate) {
    // beat the heart
    if (digitalRead(pin) == HIGH) {
      digitalWrite(pin, LOW);
    } else {
      digitalWrite(pin, HIGH);
    }
  }
 
}

