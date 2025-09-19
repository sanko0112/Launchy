#include <ldr.h>


void setup() {
  // put your setup code here, to run once:
  u8g2.begin();

}

void loop() {
  // put your main code here, to run repeatedly:
    draw_auth(1);
    delay(100);

}
