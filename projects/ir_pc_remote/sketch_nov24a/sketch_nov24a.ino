#include <IRremote.hpp>

#define PIN_GATE 10
#define PIN_RECEIVER 11

// IR REMOTE DEFINES
#define REPEAT 0x0
#define PWR 0xBA45FF00
#define VOL_PLUS 0xB946FF00
#define VOL_MINUS 0xEA15FF00
#define FUNC_STOP 0xB847FF00
#define REWIND 0xBB44FF00
#define PAUSE_PLAY 0xBF40FF00
#define FAST_FORWARD 0xBC43FF00
#define DOWN 0xF807FF00
#define UP 0xF609FF00
#define BUTTON_ZERO 0xE916FF00
#define EQ 0xE619FF00
#define ST_REPT 0xF20DFF00
#define BUTTON_1 0xF30CFF00
#define BUTTON_2 0xE718FF00
#define BUTTON_3 0xA15EFF00
#define BUTTON_4 0xF708FF00
#define BUTTON_5 0xE31CFF00
#define BUTTON_6 0xA55AFF00
#define BUTTON_7 0xBD42FF00
#define BUTTON_8 0xAD52FF00
#define BUTTON_9 0xB54AFF00

unsigned long irData;

void setup() {
  Serial.begin(9600);       // Start serial communication
  IrReceiver.begin(PIN_RECEIVER); // Start the IR reciever
  Serial.println("IR Receiver is ready");
  
  // setup GATE OUT
  pinMode(PIN_GATE, OUTPUT);

}

/*
  repeat = 0x0
  pwr = 0xBA45FF00
  vol_plus = 0xB946FF00
  vol_minus = 0xEA15FF00
  func_stop = 0xB847FF00
  rewind = 0xBB44FF00
  pause_play = 0xBF40FF00
  fast_forward = 0xBC43FF00
  down = 0xF807FF00
  up = 0xF609FF00
  button_zero = 0xE916FF00
  eq = 0xE619FF00
  st_rept = 0xF20DFF00
  button_1 = 0xF30CFF00
  button_2 = 0xE718FF00
  button_3 = 0xA15EFF00
  button_4 = 0xF708FF00
  button_5 = 0xE31CFF00
  button_6 = 0xA55AFF00
  button_7 = 0xBD42FF00
  button_8 = 0xAD52FF00
  button_9 = 0xB54AFF00
*/


void loop() {
  if (IrReceiver.decode()) {
    irData = IrReceiver.decodedIRData.decodedRawData;
    
    // ignore Repeated presses
    if (irData == 0x0) {
      IrReceiver.resume();
      return;
    }

    // Print the received value in hexadecimal format
    Serial.print("Hex Code: 0x");
    Serial.println(irData, HEX);
    
    // print the button on the remote that was pressed
    testButton(irData);

    // power button on the remote (0x0) turns the LED on
    if (irData == PWR) {
      digitalWrite(PIN_GATE, HIGH);
      Serial.println("Gate set HIGH");
      delay(500);
      digitalWrite(PIN_GATE, LOW);
      Serial.println("Gate set LOW");
    }

    // Resume the IR receiver to receive the next signal
    IrReceiver.resume();
  }

}


void testButton(unsigned long irData) {
  switch (irData) {
    case REPEAT:
      Serial.println("Repeat button was pressed");
      break;
    case PWR:
      Serial.println("Power button was pressed");
      break;
    case VOL_PLUS:
      Serial.println("Volume Plus button was pressed");
      break;
    case VOL_MINUS:
      Serial.println("Volume Minus button was pressed");
      break;
    case FUNC_STOP:
      Serial.println("Function Stop button was pressed");
      break;
    case REWIND:
      Serial.println("Rewind button was pressed");
      break;
    case PAUSE_PLAY:
      Serial.println("Pause/Play button was pressed");
      break;
    case FAST_FORWARD:
      Serial.println("Fast Forward button was pressed");
      break;
    case DOWN:
      Serial.println("Down button was pressed");
      break;
    case UP:
      Serial.println("Up button was pressed");
      break;
    case BUTTON_ZERO:
      Serial.println("Button 0 was pressed");
      break;
    case EQ:
      Serial.println("EQ button was pressed");
      break;
    case ST_REPT:
      Serial.println("ST/Repeat button was pressed");
      break;
    case BUTTON_1:
      Serial.println("Button 1 was pressed");
      break;
    case BUTTON_2:
      Serial.println("Button 2 was pressed");
      break;
    case BUTTON_3:
      Serial.println("Button 3 was pressed");
      break;
    case BUTTON_4:
      Serial.println("Button 4 was pressed");
      break;
    case BUTTON_5:
      Serial.println("Button 5 was pressed");
      break;
    case BUTTON_6:
      Serial.println("Button 6 was pressed");
      break;
    case BUTTON_7:
      Serial.println("Button 7 was pressed");
      break;
    case BUTTON_8:
      Serial.println("Button 8 was pressed");
      break;
    case BUTTON_9:
      Serial.println("Button 9 was pressed");
      break;
    default:
      Serial.println("Unknown button was pressed");
      break;
  }
}

