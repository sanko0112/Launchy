#include <OneWire.h>

OneWire  ds(3); // Connect data to pin 4

void setup() {
  Serial.begin(9600);
  Serial.println("Hello");
}

void loop() {

  byte addr[8];

  if ( !ds.search(addr)) {
    ds.reset_search();
    return;
  }

  Serial.print("Serial#: ");
  for ( int i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();
  delay(500);
}