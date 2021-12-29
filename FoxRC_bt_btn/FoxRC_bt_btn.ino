#include <SoftwareSerial.h>
#include <Time.h>
#include <RF24.h>
#include <nRF24L01.h>
#define RADIO_CHANNEL 42
#define RADIO_RATE RF24_250KBPS //RF24_1MBPS
#define RADIO_POWER RF24_PA_MAX
#define RADIO_CE 9
#define RADIO_CSN 10

const bool DEBUG = false;

const int gLedPin = 13;
const int gRxPin = 5;
const int gTxPin = 6;

const uint8_t pipe[6] = "Foxy1"; // radio address
RF24 radio(RADIO_CE, RADIO_CSN);

SoftwareSerial BTSerial(gRxPin, gTxPin);

bool input_lock = 0;
byte drive = 0;

struct Package {
  byte steering; // 0 - off, 1 - turn left, 2 - turn right
  byte drive; // 0 - full, 1 - back, 2 - front | same type as uint8_t
  bool direction; // false - forward, true - backward
  uint8_t speed; // int number 0-255
};

bool send_package(Package* pkg, int size) {
  radio.write(pkg, size);
  return 1; // loook what is this
}

bool Broadcast(Package pkg) {
  Package pack = pkg;
  return send_package(&pack, sizeof(pack));
}

Package buildPack(uint8_t speed, byte steering, bool direction) {
  Package pack;
  pack.speed = speed;
  pack.steering = steering;
  pack.direction = direction;
  pack.drive = drive;
  return pack;
}

void setup() {
  BTSerial.begin(9600);
  BTSerial.setTimeout(2);
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial) {
      ;
    }
    Serial.println("Serial is listening...");
  }

  radio.begin();
  radio.setChannel(RADIO_CHANNEL);
  radio.setDataRate(RADIO_RATE);
  radio.setPALevel(RADIO_POWER);
  radio.openWritingPipe(pipe);
  radio.setAutoAck(false);
  radio.powerUp();
  radio.stopListening();
}

void loop() {
  if (BTSerial.available()) {
    String stickInfo = BTSerial.readStringUntil('\n');
    Serial.println(stickInfo);
    if (stickInfo.length() != 8) {
      if (DEBUG) {
        Serial.print("Error: invalid bluetooth input: ");
        Serial.println(stickInfo);
      }
      return;
    }
    int angle = stickInfo.substring(0,3).toInt();
    uint8_t speed = map(stickInfo.substring(3,6).toInt(), 0, 100, 0, 255);
    uint8_t button = stickInfo.substring(6).toInt(); // 1 - X, 2 - O, 3 - triangle, 4 - square

    if (button == 1) {
      input_lock = !input_lock;
      if (DEBUG) {
        Serial.print("Input Lock status: ");
        Serial.println(input_lock);
      }
    }

    if (input_lock) {
      if (DEBUG) {
        Serial.println("Input Lock!");
      }
      return;
    }

    if (button == 3) {
      drive++;
      if (drive > 2) {
        drive = 0;
      }
      if (DEBUG) {
        Serial.print("Drive mode set to ");
        Serial.println(drive);
      }
    }

    byte steering = 0;
    if (button != 1 && button != 3) { // 0 is default position that should set steering to 0
      if (button == 2) {
        steering = 2;
      } else if (button == 4) {
        steering = 1;
      }
    }

    if (DEBUG) {
      Serial.println(steering);
    }

    bool direction = 0;
    if (angle > 180) {
      direction = 1;
    }

    Package pack = buildPack(speed, steering, direction);
    bool sent = Broadcast(pack);
    if (DEBUG && sent) {
      Serial.print("Package is sent: ");
      Serial.println(stickInfo);
    }
  }
}
