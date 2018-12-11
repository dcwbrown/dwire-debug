
#include "uartsoft.hpp"

#include <avr/wdt.h>
#include <string.h>
#include <util/delay.h>

int main() {
  uartSoft.begin();

  for (;;) {
    wdt_reset();
    _delay_ms(100);

    // get the data
    char* s;
    uint8_t n = uartSoft.read(&s);
    if (!n) continue;

    // send it back
    char buf[32] = "got: ";
    strcat(buf, s);
    strcat(buf, "\n");
    _delay_ms(100);
    uartSoft.write(buf, strlen(buf));
  }
}
