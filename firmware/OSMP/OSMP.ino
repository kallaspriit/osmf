#include "Commander.h"

// application config
const int GSM_BAUDRATE = 19200;
const unsigned long BTN_DEBOUNCE_PERIOD_MS = 300;

// pin config
const int TEST_BTN_PIN = 14;
const int STATUS_LED_GREEN_PIN = 3;
const int STATUS_LED_RED_PIN = 4;
const int GSM_PWRKEY_PIN = 2;
const int GSM_STATUS_PIN = 7;
const int GSM_NET_PIN = 8;

enum Button {
  BTN_TEST
};

// serials
usb_serial_class *local = &Serial;
//HardwareSerial *serial1 = &Serial1;
HardwareSerial2 *remote = &Serial2;

// runtime
unsigned long lastUpdateTime = 0;
unsigned long lastButtonPressTime = 0;
boolean isGsmModulePoweredOn = false;

Commander commander(local);

void setup() {
  setupPinModes();
  setupSerials();
  setupStatusListeners();
  setupGsmModule();
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long dt = currentTime - lastUpdateTime;

  updateButtons(currentTime, dt);
  updateSerials(currentTime, dt);
  updateCommander(currentTime, dt);

  lastUpdateTime = currentTime;
}

void setupPinModes() {
  pinMode(TEST_BTN_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_GREEN_PIN, OUTPUT);
  pinMode(STATUS_LED_RED_PIN, OUTPUT);
  pinMode(GSM_PWRKEY_PIN, OUTPUT);
  pinMode(GSM_STATUS_PIN, INPUT);
  pinMode(GSM_NET_PIN, INPUT);
}

void setupSerials() {
  local->begin(115200);
  remote->begin(GSM_BAUDRATE);
}

void setupStatusListeners() {
  attachInterrupt(GSM_STATUS_PIN, onGsmStatusChange, CHANGE);
  attachInterrupt(GSM_NET_PIN, onGsmNetChange, CHANGE);
}

void setupGsmModule() {

}

void updateButtons(unsigned long currentTime, unsigned long dt) {
  if (digitalRead(TEST_BTN_PIN) == LOW && currentTime - lastButtonPressTime > BTN_DEBOUNCE_PERIOD_MS) {
    local->println("Button pressed");

    lastButtonPressTime = currentTime;

    onButtonPressed(Button::BTN_TEST);
  }
}

void updateSerials(unsigned long currentTime, unsigned long dt) {
  /*while (local->available() > 0) {
    char character = local->read();

    remote->print(character);
  }*/

  while (remote->available() > 0) {
    char character = remote->read();

    local->print(character);
  }
}

void updateCommander(unsigned long currentTime, unsigned long dt) {
  while (commander.gotCommand()) {
    handleCommand(commander.command, commander.parameters, commander.parameterCount);
  }
}

void onButtonPressed(int btn) {
  switch (btn) {
    case Button::BTN_TEST:
      local->print("Toggling GSM power..");
      toggleGsmPower();
      break;
  }
}

void onGsmStatusChange() {
  boolean on = digitalRead(GSM_STATUS_PIN) == HIGH;

  if (on) {
    isGsmModulePoweredOn = true;
    showRedStatus(255);
  } else {
    isGsmModulePoweredOn = false;
    showRedStatus(0);
  }
}

void onGsmNetChange() {
  boolean on = digitalRead(GSM_NET_PIN) == HIGH;

  // TODO extract status from the pulse length and off period
  if (on) {
    showGreenStatus(255);
  } else {
    showGreenStatus(0);
  }
}

void showStatus(int green, int red) {
  analogWrite(STATUS_LED_GREEN_PIN, green);
  analogWrite(STATUS_LED_RED_PIN, red);
}

void showGreenStatus(int value) {
  analogWrite(STATUS_LED_GREEN_PIN, value);
}

void showRedStatus(int value) {
  analogWrite(STATUS_LED_RED_PIN, value);
}

void toggleGsmPower() {
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(1000);
  digitalWrite(GSM_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(3000);

  local->println("done!");
}

void sendTextMessage(String number, String message) {
  remote->print("AT+CMGF=1\r");
  
  delay(100);
  remote->print("AT + CMGS = \"");
  remote->print(number);
  remote->println("\"");
  
  delay(100);
  remote->println(message);
  
  delay(100);
  remote->println((char)26);// the ASCII code of the ctrl+z is 26
  
  delay(100);
  remote->println();
}

void startCall(String number) {
 remote->print("ATD + ");
 remote->print(number);
 remote->println(";");
 
 delay(100);
 remote->println();
}

void endCall() {
  remote->println("ATH");
}

void handleCommand(String command, String parameters[], int parameterCount) {
  if (command == "on" && parameterCount == 0) {
    handleOnCommand();
  } else if (command == "off" && parameterCount == 0) {
    handleOffCommand();
  } else if (command == "cmd" && parameterCount == 1) {
    handleCmdCommand(parameters);
  } else if (command == "sms" && parameterCount == 2) {
    handleSmsCommand(parameters);
  } else if (command == "call" && parameterCount == 1) {
    handleCallCommand(parameters);
  } else {
    local->print("Unhandled command '");
    local->print(command);
    local->print("' with ");
    local->print(parameterCount);
    local->println(" parameters: ");
    
    for (int i = 0; i < parameterCount; i++) {
      local->print("  > ");
      local->print(i);
      local->print(": ");
      local->println(parameters[i]);
    }
  }
}

void handleOnCommand() {
  if (isGsmModulePoweredOn) {
    local->println("Module is already turned on, ignoring request");

    return;
  }

  local->println("Turning GSM module on");

  toggleGsmPower();
}

void handleOffCommand() {
  if (!isGsmModulePoweredOn) {
    local->println("Module is already turned off, ignoring request");

    return;
  }

  local->println("Turning GSM module off");

  toggleGsmPower();
}

void handleCmdCommand(String parameters[]) {
  String cmd = parameters[0];

  local->print("Forwarding AT command '");
  local->print(cmd);
  local->println("'");

  remote->print(cmd);
  remote->print('\r');  
}

void handleSmsCommand(String parameters[]) {
  String number = parameters[0];
  String message = parameters[1];

  local->print("Sending SMS message '");
  local->print(message);
  local->print("' to number '");
  local->print(number);
  local->println("'");
  
  sendTextMessage(number, message);
}

void handleCallCommand(String parameters[]) {
  String number = parameters[0];

  local->print("Calling number '");
  local->print(number);
  local->println("'");
  
  startCall(number);
  delay(20000);
  endCall();
}
