// include for i2c comms with LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// BLE
#include <ArduinoBLE.h>

// time keeping
#include "RTC.h" // RTC lib
#include <NTPClient.h> // NTP lib
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h" // wifi SSID and PSWD

// function forward declarations
void heartBeat(uint8_t pin);
void explorePeripheral(BLEDevice);
void exploreService(BLEService);
void exploreCharacteristic(BLECharacteristic);
String processWeatherData(String);
String processTimeDate();
void lcdPrintData(String timeDate, String weatherData);
void printWifiStatus();
void connectToWifi();
unsigned updateTime();

// init LCD display
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// UUID(s) for HM10 BLE ; HM10 advertises a single GATT service with UUID FFE0
const char* hm10ServiceUUID = "ffe0";
const char* hm10CharacteristicUUID = "ffe1";

// wifi SSID and password
char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
NTPClient timeClient(Udp);

// for maintaining 1 second heartbeats
unsigned prevMillis = 0;
unsigned heartbeatRate = 1000; // 1000 ms = 1 s


void setup() {
  Serial.begin(9600);
  while (!Serial);

  // heartbeat
  pinMode(LED_BUILTIN, OUTPUT);

  // lcd
  lcd.init(); // automatically sets A4 as SDA and A5 as SCL
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing...");

  // wifi and retrieve time
  lcd.setCursor(0,1);
  lcd.print("Cnnectng to WiFi");
  connectToWiFi();
  RTC.begin();
  timeClient.begin();
  updateTime();

  if (!BLE.begin()) {
    Serial.println("Starting R4 Weather Station BLE Failed!");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR: Start R4");
    lcd.setCursor(0,1);
    lcd.print("Wthr Statn FAIL");
    while (true);
  }
  // Start scanning for BLE devices
  BLE.scan();
  Serial.println("UNO R4 Central initialized. Scanning for peripherals.");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initialized.");
  lcd.setCursor(0,1);
  lcd.print("Scanning.");
}


/* 
LOOP: Checks for BLE devices and calls explorePeripheral() on WEATHERPROBE
*/
void loop() {
  // check if peripheral discovered
  BLEDevice peripheral = BLE.available();
  
  // explore peripheral
  if (peripheral) {
    Serial.print("Found " + String(peripheral.localName()));
    Serial.print(" at address: " + String(peripheral.address()));
    Serial.println(" with UUID: " + peripheral.advertisedServiceUuid());

    // confirm we found weather probe
    if (peripheral.localName() == "WEATHERPROBE") {
      BLE.stopScan();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Fnd WEATHERPROBE");
      lcd.setCursor(0,1);
      lcd.print("Connecting...");
      // get and print data from WEATHERPROBE
      explorePeripheral(peripheral);

      while (true);
    }

  }
    
  heartBeat(LED_BUILTIN);
}


/* 
EXPLORE_PERIPHERAL: Connects to peripheral device, discovering attributes and looking
    for service with hm10ServiceUUID and calls exploreService() on it
*/
void explorePeripheral(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.println("Connected to: " + String(peripheral.localName()));
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // iterate the services and look for the main service
  for (int s = 0; s < peripheral.serviceCount(); s++) {
    BLEService service = peripheral.service(s);

    Serial.println("Found service: " + String(service.uuid()));
    if (String(service.uuid()) == String(hm10ServiceUUID)) {
      exploreService(service);
    }
  }

}


/* 
EXPLORE_SERVICE: Connects to hm10Service, iterates characteristic. Looks for weatherCharacteristic with 
    hm10CharacteristicUUID = 0xffe1 and calls exploreCharacteristic() on it
*/
void exploreService(BLEService service) {
  // iterate the characteristics, looking for hm10 characteristic
  for (int c = 0; c < service.characteristicCount(); c++) {
    BLECharacteristic characteristic = service.characteristic(c);

    Serial.println("Found characteristic: " + String(characteristic.uuid()));
    if (String(characteristic.uuid()) == String(hm10CharacteristicUUID)) {
      exploreCharacteristic(characteristic);
    }
  }
}


/* 
EXPLORE_CHARACTERISTIC: subscribes to the characteristic's notifications.
    Waits for updates; passes data to processData()
*/
void exploreCharacteristic(BLECharacteristic characteristic) {
  Serial.print("\tCharacteristic ");
  Serial.print(characteristic.uuid());
  Serial.print(", properties 0x");
  Serial.println(characteristic.properties(), HEX);

  // subscribe to notifs from WEATHERPROBE
  if (characteristic.canSubscribe()) {
    if (!characteristic.subscribe()) {
      Serial.println("Failed to subscribe to characteristic notifications!");
      return;
    }
    Serial.println("Successfully subscribed to characteristic notifications.");
  } else {
    Serial.println("Characteristic does not support notifications.");
  }

  // set up buffer to read BLE
  const uint8_t maxBLEPacketSize = 20;
  char buffer[maxBLEPacketSize + 1];

  // wait for updates
  while (characteristic) {
    if (characteristic.valueUpdated()) {
      String weatherData = String((const char*)characteristic.value());

      Serial.println("Received from BLE: " + weatherData); // debug

      // process weather data
      weatherData = processWeatherData(weatherData);

      // get time and date data
      String timeDateData = processTimeDate();

      // print timeDate and weather 
      lcdPrintData(timeDateData, weatherData);
    }

    heartBeat(LED_BUILTIN);
    delay(5000);
  }
}


/* 
PROCESSWEATHERDATA: Parses data in the format of T:temp,H:humid (total, )
  temp is temperature in celsius rounded to 1 decimal (3-4 bytes)
  humid is humidity in percent with no decimals (2 bytes)
*/
String processWeatherData(String str) {
  // str of format 'T:XX.X,H:XX'
  String t, h;
  uint8_t index_comma = str.indexOf(',');

  t = str.substring(0, index_comma); // 'T:XX.X'
  t = t.substring(2); // ':' will always be at index 1
  float tmp = t.toFloat();
  tmp = tmp * 1.8 + 32;
  t = String(tmp, 1);
  
  h = str.substring(index_comma + 1);
  h = h.substring(2); // ':' will always be at index 1
  
  String weather = t + char(223) + "F/" + h + "%";

  return weather;
}

/*
PROCESSTIMEDATE: Retrieves current time from RTC via wifi
  RETURNS STRING OF FORMAT: "DAY MTH dM YEAR/HH:MM" (dM is dayOfMonth)
*/
String processTimeDate() {
  RTCTime currentTime(updateTime()); // RTCTime obj
  
  // update the time
  RTC.getTime(currentTime);
  unsigned year = currentTime.getYear();
  int dayOfMonth = currentTime.getDayOfMonth();
  String time = (currentTime.getHour() < 10 ? "0" : "") + String(currentTime.getHour()) + ":" + 
                    (currentTime.getMinutes() < 10 ? "0" : "") + String(currentTime.getMinutes());
  
  DayOfWeek day = currentTime.getDayOfWeek();
  String d;
  switch (day) { // set the day
    case DayOfWeek::MONDAY:
      d = "Mon";
      break;
    case DayOfWeek::TUESDAY:
      d = "Tue";
      break;
    case DayOfWeek::WEDNESDAY:
      d = "Wed";
      break;
    case DayOfWeek::THURSDAY:
      d = "Thu";
      break;
    case DayOfWeek::FRIDAY:
      d = "Fri";
      break;
    case DayOfWeek::SATURDAY:
      d = "Sat";
      break;
    case DayOfWeek::SUNDAY:
      d = "Sun";
      break;
    default:
      d = "ERR";
  }
  Month month = currentTime.getMonth();
  String m;
  switch (month) { // set the month
    case Month::JANUARY:
      m = "Jan";
      break;
    case Month::FEBRUARY:
      m = "Feb";
      break;
    case Month::MARCH:
      m = "Mar";
      break;
    case Month::APRIL:
      m = "Apr";
      break;
    case Month::MAY:
      m = "May";
      break;
    case Month::JUNE:
      m = "Jun";
      break;
    case Month::JULY:
      m = "Jul";
      break;
    case Month::AUGUST:
      m = "Aug";
      break;
    case Month::SEPTEMBER:
      m = "Sep";
      break;
    case Month::OCTOBER:
      m = "Oct";
      break;
    case Month::NOVEMBER:
      m = "Nov";
      break;
    case Month::DECEMBER:
      m = "Dec";
      break;
    default:
      m = "ERR";
      break;
  }

  String ret = d + " " + m + " " + String(dayOfMonth) + " " + String(year) + "/" + time;

  return ret;
}


/*
UPDATETIME: Reprobes pool.ntp.org for accurate time
*/

unsigned updateTime() {
  timeClient.update();
  auto timeZoneOffsetHours = -6; // set for UTC-6
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  return unixTime;
}
/*
LCDPRINTDATA: prints both time&date and weather data to the LCD
*/
void lcdPrintData(String timeDate, String weatherData) {
  uint8_t slashIdx = timeDate.indexOf('/');
  String top = timeDate.substring(0, slashIdx);
  String HHMM = timeDate.substring(slashIdx + 1);

  String bottom = HHMM + " " + weatherData;
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(top);

  lcd.setCursor(0,1); // (col, row)
  lcd.print(bottom);
}

/*
PRINTWIFISTATUS: debug, prints SSID, board's local IP, and signal strength
*/
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


/*
CONNECTTOWIFI: connects to wifi
*/
void connectToWiFi(){
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // connect to WPA2 network
    wifiStatus = WiFi.begin(ssid, password);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();
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