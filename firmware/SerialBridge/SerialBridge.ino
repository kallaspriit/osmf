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
HardwareSerial2 *remote = &Serial2;

// runtime
unsigned long lastUpdateTime = 0;
unsigned long lastButtonPressTime = 0;

void setup() {
  setupPinModes();
  setupSerials();
  setupStatusListeners();
  setupGsmModule();
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
  //delay(5000);
  //toggleGsmPower();
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long dt = currentTime - lastUpdateTime;
  
  updateButtons(currentTime, dt);
  updateSerials(currentTime, dt);
  
  lastUpdateTime = currentTime;
}

void updateButtons(unsigned long currentTime, unsigned long dt) {
  if (digitalRead(TEST_BTN_PIN) == LOW && currentTime - lastButtonPressTime > BTN_DEBOUNCE_PERIOD_MS) {
    local->println("Button pressed");
    
    lastButtonPressTime = currentTime;
    
    onButtonPressed(Button::BTN_TEST);
  }
}

void updateSerials(unsigned long currentTime, unsigned long dt) {
  while (local->available() > 0) {
    char character = local->read();

    remote->print(character);
  }

  while (remote->available() > 0) {
    char character = remote->read();

    local->print(character);
  }  
}

void onButtonPressed(int btn) {
  switch (btn) {
    case Button::BTN_TEST:
      toggleGsmPower();
    break;
  }
}

void onGsmStatusChange() {
  boolean on = digitalRead(GSM_STATUS_PIN) == HIGH;
  
  //local->print("Gsm status changed to ");
  
  if (on) {
    //local->println("ON");
    showRedStatus(255);
  } else {
    //local->println("OFF");
    showRedStatus(0);
  }
}

void onGsmNetChange() {
  boolean on = digitalRead(GSM_NET_PIN) == HIGH;
  
  //local->print("Net status changed to ");
  
  if (on) {
    //local->println("ON");
    showGreenStatus(255);
  } else {
    //local->println("OFF");
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
  local->print("Toggling GSM power..");
  
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(1000);
  digitalWrite(GSM_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(3000);
  
  local->println("done!");
}
