#include <SoftwareSerial.h>
#include "HX711.h"

// Pin Definitions
#define DOUT A1
#define CLK A0
#define COIN_HOPPER_PIN 4

// SoftwareSerial for Nextion Display
SoftwareSerial nextion(10, 11);  // RX, TX

HX711 scale;
const float scaleFactor = 123000.0;
const float pricePerKg = 10.0;

float weight = 0;
int coins = 0;

float weightPaper = 0, weightGlass = 0, weightPlastic = 0;
int coinsPaper = 0, coinsGlass = 0, coinsPlastic = 0;
int coinsTotal = 0;

unsigned long lastUpdate = 0;

// Send a float or int value to a Nextion number object
void sendToNextionNumber(String objName, float value) {
  nextion.print(objName + ".val=");
  nextion.print(value, 2);
  nextion.write(0xFF);
  nextion.write(0xFF);
  nextion.write(0xFF);
  Serial.print("Sent to Nextion: ");
  Serial.print(objName);
  Serial.print(" = ");
  Serial.println(value);
}

// Dispense coins via relay or transistor
void dispenseCoins() {
  Serial.println("Dispensing coins...");
  digitalWrite(COIN_HOPPER_PIN, HIGH);
  delay(5000);
  digitalWrite(COIN_HOPPER_PIN, LOW);
}

// Optional: Get current page from Nextion if "Send Page ID" is enabled
int getCurrentPage() {
  if (nextion.available()) {
    if (nextion.read() == 0x66) {  // Page ID return signal
      while (!nextion.available())
        ;
      int page = nextion.read();
      while (nextion.available()) nextion.read();  // Clear remaining
      Serial.print("Current Page: ");
      Serial.println(page);
      return page;
    }
  }
  return -1;  // Not detected
}

void setup() {
  Serial.begin(9600);   // Debug serial
  nextion.begin(9600);  // Nextion serial
  scale.begin(DOUT, CLK);
  scale.set_scale(scaleFactor);
  scale.tare();

  pinMode(COIN_HOPPER_PIN, OUTPUT);
  digitalWrite(COIN_HOPPER_PIN, LOW);

  Serial.println("System Ready.");
}

void loop() {
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    weight = scale.get_units(10);
    coins = int(weight * pricePerKg);

    Serial.print("Weight: ");
    Serial.print(weight, 2);
    Serial.print(" kg, Coins: ");
    Serial.println(coins);

    int page = getCurrentPage();

    // If Nextion doesn't return page ID, you can manually set one for testing:
    if (page == -1) {
      Serial.println("No page ID received. (Make sure 'Send Page ID' is enabled in Nextion Editor)");
    }

    // Page 2 = Paper
    if (page == 2) {
      weightPaper = weight;
      coinsPaper = coins;
      sendToNextionNumber("nWeightPaper", weightPaper);
      sendToNextionNumber("nPricePaper", coinsPaper);
    }
    // Page 3 = Glass
    else if (page == 3) {
      weightGlass = weight;
      coinsGlass = coins;
      sendToNextionNumber("nWeightGlass", weightGlass);
      sendToNextionNumber("nPriceGlass", coinsGlass);
    }
    // Page 4 = Plastic
    else if (page == 4) {
      weightPlastic = weight;
      coinsPlastic = coins;
      sendToNextionNumber("nWeightPlastic", weightPlastic);
      sendToNextionNumber("nPricePlastic", coinsPlastic);
    }
    // Page 6 = Summary / Checkout
    else if (page == 6) {
      coinsTotal = coinsPaper + coinsGlass + coinsPlastic;
      float totalWeight = weightPaper + weightGlass + weightPlastic;
      sendToNextionNumber("nWeightTotal", totalWeight);
      sendToNextionNumber("nPriceTotal", coinsTotal);
      dispenseCoins();
    }
  }

  // Listen to commands from Nextion (e.g. button presses)
  // if (nextion.available()) {
  //   String cmd = "";
  //   while (nextion.available()) {
  //     char c = nextion.read();
  //     cmd += c;
  //     delay(2);  // Short delay to gather full message
  //   }
  //   cmd.trim();

  //   Serial.print("Received Command: ");
  //   Serial.println(cmd);

  //   if (cmd == "checkout.val=0") {
  //     dispenseCoins();
  //   }

  //   if (cmd == "b0.val=0") {
  //     nextion.print("page 5");
  //     nextion.write(0xFF); nextion.write(0xFF); nextion.write(0xFF);
  //   }
  // }
  static byte buffer[10];
  static int index = 0;

  while (nextion.available()) {
    byte b = nextion.read();
    buffer[index++] = b;

    // Print each byte received (debugging)
    Serial.print("Byte Received: 0x");
    Serial.println(b, HEX);

    // Detect end of message: 0xFF 0xFF 0xFF
    if (index >= 3 && buffer[index - 3] == 0xFF && buffer[index - 2] == 0xFF && buffer[index - 1] == 0xFF) {

      byte command = buffer[0];  // The first byte is your command ID

      switch (command) {
        case 0x01:
          Serial.println("Page change to page 5 triggered.");
          nextion.print("page 5");
          nextion.write(0xFF);
          nextion.write(0xFF);
          nextion.write(0xFF);
          break;

        case 0x02:
          Serial.println("Dispensing coins command received.");
          dispenseCoins();
          break;

        default:
          Serial.print("Unknown command: ");
          Serial.println(command, HEX);
          break;
      }

      index = 0;  // Reset for next command
    }

    if (index >= 10) index = 0;  // Avoid overflow
  }
}
