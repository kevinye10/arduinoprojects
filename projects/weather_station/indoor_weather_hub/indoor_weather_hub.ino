#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoBLE.h>

// function forward declarations
void heartBeat(uint8_t pin);
void explorePeripheral(BLEDevice);
void exploreService(BLEService);
void exploreCharacteristic(BLECharacteristic);

// init LCD display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// UUID(s) for HM10 BLE ; HM10 advertises a single GATT service with UUID FFE0
const char* hm10ServiceUUID = "ffe0";
const char* hm10CharacteristicUUID = "ffe1";

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

  if (!BLE.begin()) {
    Serial.println("Starting R4 Weather Station BLE Failed!");
    while (true);
  }
  // Start scanning for BLE devices
  BLE.scan();
  Serial.println("UNO R4 Central initialized. Scanning for peripherals.");
}

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

      // get and print data from WEATHERPROBE
      explorePeripheral(peripheral);

      while (true);
    }

  }
    
  heartBeat(LED_BUILTIN);
}

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

void exploreCharacteristic(BLECharacteristic characteristic) {
  Serial.print("\tCharacteristic ");
  Serial.print(characteristic.uuid());
  Serial.print(", properties 0x");
  Serial.print(characteristic.properties(), HEX);

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

  while (characteristic) {
    // wait for updates
    if (characteristic.valueUpdated()) {
      String data = String((const char*)characteristic.value());
      Serial.println("Received from BLE: " + data);
    }

    heartBeat(LED_BUILTIN);
    delay(1000);
  }
}

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