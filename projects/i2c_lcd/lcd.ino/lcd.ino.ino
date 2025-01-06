#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// initialize lcd obj lib
LiquidCrystal_I2C lcd(0x27, 20, 4);

// for maintaining 1 second heartbeats
unsigned prevMillis = 0;
unsigned heartbeatRate = 1000; // 1000 ms = 1 s


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  lcd.init(); // automatically sets A4 as SDA and A5 as SCL
  lcd.backlight();
}

/*
  Begin I2C (active low):
  1. Start condition : From idle, SDA to low while SCL remains high
  2. Next 7 bits are the address; then the 8th is R/W'
  3. ACK (0) / NACK (1) (every 9th bit)
    - ACKnowledged message; continue transmission
    - NotACKnowledged message; retry OR Stop Condition: SDA to high while SCL is high
  4. 8 bits of data
  5. Go to 3
*/

void loop() {
  // put your main code here, to run repeatedly:
  lcd.clear();
  lcd.setCursor(0, 0);
  slideWords("Hello, World!");
  // lcd.print("Hello world");
  
  heartBeat(LED_BUILTIN);
  delay(500);
}


void slideWords(String s) {
  lcd.clear();
  uint8_t len = s.length();
  
  for (uint8_t i = 0; i < len; i++) {
    heartBeat(LED_BUILTIN);
    lcd.setCursor(i % 16, i / 16);
    lcd.print(s[i]);
    delay(200);
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