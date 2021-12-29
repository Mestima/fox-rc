#include <RF24.h>
#include <nRF24L01.h>
#define RADIO_CHANNEL 42
#define RADIO_RATE RF24_250KBPS //RF24_1MBPS
#define RADIO_POWER RF24_PA_MAX
#define RADIO_CE 7
#define RADIO_CSN 8

#define MOTOR1_MAIN1 5
#define MOTOR1_MAIN2 6
#define MOTOR2_MAIN1 9
#define MOTOR2_MAIN2 10
#define MOTOR_WHEEL1 A1
#define MOTOR_WHEEL2 A2

const bool DEBUG = false;

const uint8_t pipe[6] = "Foxy1";
RF24 radio(RADIO_CE, RADIO_CSN);

struct Package {
  byte steering; // 0 - off, 1 - turn left, 2 - turn right
  byte drive; // 0 - full, 1 - back, 2 - front | same type as uint8_t
  bool direction; // false - forward, true - backward
  uint8_t speed; // int number 0-255
};

bool emergency = false;
int emergencyTimer = 1000;
unsigned long emergencyCounter = 0;

bool handleEmergency() {
  if (!emergency) {
    return false;
  }
  if (DEBUG) {
    Serial.println("Emergency lock!");
  }
  analogWrite(MOTOR1_MAIN1, 0);
  analogWrite(MOTOR1_MAIN2, 0);
  analogWrite(MOTOR2_MAIN1, 0);
  analogWrite(MOTOR2_MAIN2, 0);
  digitalWrite(MOTOR_WHEEL1, LOW);
  digitalWrite(MOTOR_WHEEL2, LOW);
  return true;
}

void handleMotors(uint8_t speed, byte steering, byte drive, bool direction) {
  switch(drive) {
    case(0): { // full drive
      switch(direction) {
        case(false): { // forward
          analogWrite(MOTOR1_MAIN1, 0);
          analogWrite(MOTOR1_MAIN2, speed);
          analogWrite(MOTOR2_MAIN1, 0);
          analogWrite(MOTOR2_MAIN2, speed);
          break;
        }
        case(true): { // backward
          analogWrite(MOTOR1_MAIN1, speed);
          analogWrite(MOTOR1_MAIN2, 0);
          analogWrite(MOTOR2_MAIN1, speed);
          analogWrite(MOTOR2_MAIN2, 0);
          break;
        }
      }
      break;
    }
    case(1): { // back drive
      switch(direction) {
        case(false): { // forward
          analogWrite(MOTOR1_MAIN1, 0);
          analogWrite(MOTOR1_MAIN2, speed);
          break;
        }
        case(true): { // backward
          analogWrite(MOTOR1_MAIN1, speed);
          analogWrite(MOTOR1_MAIN2, 0);
          break;
        }
      }
      break;
    }
    case(2): { // front drive
      switch(direction) {
        case(false): { // forward
          analogWrite(MOTOR2_MAIN1, 0);
          analogWrite(MOTOR2_MAIN2, speed);
          break;
        }
        case(true): { // backward
          analogWrite(MOTOR2_MAIN1, speed);
          analogWrite(MOTOR2_MAIN2, 0);
          break;
        }
      }
      break;
    }
  }

  switch(steering) {
    case(0): { // off
      digitalWrite(MOTOR_WHEEL1, LOW);
      digitalWrite(MOTOR_WHEEL2, LOW);
      break;
    }
    case(1): { // turn left
      digitalWrite(MOTOR_WHEEL2, HIGH);
      digitalWrite(MOTOR_WHEEL1, LOW);
      break;
    }
    case(2): { // turn right
      digitalWrite(MOTOR_WHEEL1, HIGH);
      digitalWrite(MOTOR_WHEEL2, LOW);
      break;
    }
  }
}

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial) {
      ;
    }
    Serial.println("Serial is listening...");
  }

  pinMode(MOTOR_WHEEL1, OUTPUT);
  pinMode(MOTOR_WHEEL2, OUTPUT);
  digitalWrite(MOTOR_WHEEL1, LOW);
  digitalWrite(MOTOR_WHEEL2, LOW);

  radio.begin();
  radio.setChannel(RADIO_CHANNEL);
  radio.setDataRate(RADIO_RATE);
  radio.setPALevel(RADIO_POWER);
  radio.openReadingPipe(1, pipe);
  radio.setAutoAck(false); 
  radio.powerUp();
  radio.startListening();

  emergencyCounter = millis();
}

void loop() {
  if (millis() - emergencyCounter > emergencyTimer) {
    emergency = true;
  }

  if(radio.available()){
    if (emergency) {
      emergency = false;
    }
    emergencyCounter = millis();
    Package pack;
    radio.read(&pack, sizeof(pack));
    handleMotors(pack.speed, pack.steering, pack.drive, pack.direction);
    if (DEBUG) {
      Serial.print("Incoming packet - Speed: ");
      Serial.print(pack.speed);
      Serial.print(" | Steering: ");
      Serial.print(pack.steering);
      Serial.print(" | Drive: ");
      Serial.print(pack.drive);
      Serial.print(" | Direction: ");
      Serial.println(pack.direction);
    }
  } else {
    //handleEmergency();
  }
}
