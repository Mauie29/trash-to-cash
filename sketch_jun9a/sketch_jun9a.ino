#include <Servo.h>

Servo paperLid;

const int PAPERPIN = 6;


void setup() {
  Serial.begin(9600);

  paperLid.attach(TOP_PAPERPIN_PIN);


}

void loop() {

  paperLid.write(90);

}
}