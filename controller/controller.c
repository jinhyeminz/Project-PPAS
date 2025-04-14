#include <ArduinoBLE.h>

const int buttonPins[5] = {2, 3, 4, 5, 6};
const uint8_t buttonIDs[5] = {0x01, 0x02, 0x03, 0x04, 0x05};  
// Power, Pre, Next, Play, Laser

const int vibrationPin = 7;  // 진동모터 핀
const int laser = 8; // 레이저

const unsigned long debounceDelay = 50;
bool lastButtonStates[5] = {HIGH};
bool buttonStates[5] = {HIGH};
unsigned long lastDebounceTimes[5] = {0};

bool isPlaying = false;

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-1234-1234-abcdefabcdef"

BLEService presenterService(SERVICE_UUID);
BLECharacteristic buttonCharacteristic(CHARACTERISTIC_UUID, BLERead | BLENotify | BLEWrite, 40); // BLEWrite 추가

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 5; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  pinMode(laser, OUTPUT); // 레이저
  pinMode(vibrationPin, OUTPUT); // 진동 모터
  digitalWrite(vibrationPin, LOW); // 초기 진동 OFF

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed");
    while (1);
  }

  BLE.setLocalName("ESP32");
  BLE.setAdvertisedService(presenterService);
  presenterService.addCharacteristic(buttonCharacteristic);
  BLE.addService(presenterService);
  BLE.advertise();

  buttonCharacteristic.setEventHandler(BLEWritten, onBLEWrite); // 수신 콜백 설정

  Serial.println("advertising started...");
}

void loop() {
  BLEDevice central = BLE.central();
  bool isConnected = central && central.connected();

  handleButton(0, true);  // 전원 버튼, BLE 없어도 처리
  handleButton(4, true);  // 레이저 버튼, BLE 없어도 처리

  if (isConnected) {
    for (int i = 1; i <= 3; i++) {
      handleButton(i, false);  // 나머지 버튼은 BLE 연결된 경우만 처리
    }
  }
}



void handleButton(int i, bool alwaysActive) {
  int reading = digitalRead(buttonPins[i]);

  if (reading != lastButtonStates[i]) {
    lastDebounceTimes[i] = millis();
  }

  if ((millis() - lastDebounceTimes[i]) > debounceDelay) {
    uint8_t packet[2];

    if (i == 0 && alwaysActive) {  // Power
      if (reading == LOW && buttonStates[i] == HIGH) {
        packet[0] = buttonIDs[i];
        packet[1] = 0x01;
        sendBLEMessage(packet, 2);
        buttonStates[i] = LOW;
      } else if (reading == HIGH && buttonStates[i] == LOW) {
        buttonStates[i] = HIGH;
      }
    }

    else if (i == 4 && alwaysActive) {  // Laser
      if (reading == LOW && buttonStates[i] == HIGH) {
        digitalWrite(8, HIGH);
        buttonStates[i] = LOW;
      } else if (reading == HIGH && buttonStates[i] == LOW) {
        digitalWrite(8, LOW);
        buttonStates[i] = HIGH;
      }
    }

    else if (!alwaysActive) {
      if (i >= 1 && i <= 2) {
        if (reading == LOW && buttonStates[i] == HIGH) {
          packet[0] = buttonIDs[i];
          packet[1] = 0x01;
          sendBLEMessage(packet, 2);
          buttonStates[i] = LOW;
        } else if (reading == HIGH && buttonStates[i] == LOW) {
          buttonStates[i] = HIGH;
        }
      }

      else if (i == 3) { // Play/Pause
        if (reading == LOW && buttonStates[i] == HIGH) {
          isPlaying = !isPlaying;
          packet[0] = buttonIDs[i];
          packet[1] = isPlaying ? 0x01 : 0x02;
          sendBLEMessage(packet, 2);
          buttonStates[i] = LOW;
        } else if (reading == HIGH && buttonStates[i] == LOW) {
          buttonStates[i] = HIGH;
        }
      }
    }
  }

  lastButtonStates[i] = reading;
}



void sendBLEMessage(const uint8_t* data, size_t length) {
  buttonCharacteristic.writeValue(data, length);
//  Serial.print("Sent BLE: [");
//  for (size_t i = 0; i < length; i++) {
//    Serial.print("0x");
//    Serial.print(data[i], HEX);
//    if (i < length - 1) Serial.print(", ");
//  }
//  Serial.println("]");
}

// 🔔 수신 콜백 함수
void onBLEWrite(BLEDevice central, BLECharacteristic characteristic) {
  uint8_t buffer[2];
  int length = characteristic.readValue(buffer, sizeof(buffer));

  if (length == 2) {
//    Serial.print("Received BLE: [0x");
//    Serial.print(buffer[0], HEX);
//    Serial.print(", 0x");
//    Serial.print(buffer[1], HEX);
//    Serial.println("]");

    if (buffer[0] == 0x06) { // 진동 명령
      if (buffer[1] == 0x01) {
        digitalWrite(vibrationPin, HIGH); // 진동 ON
//        Serial.println("Vibration ON");
      } else if (buffer[1] == 0x00) {
        digitalWrite(vibrationPin, LOW); // 진동 OFF
//        Serial.println("Vibration OFF");
      }
    }
  }
}