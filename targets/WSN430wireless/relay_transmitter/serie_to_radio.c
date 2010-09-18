#include "target.h"
#include "serial_line.h"
#include "cc1100.h"

int main(void) {
  uint16_t i;

  /* Initialisation materiel serie, radio */
  hardware_init();
  cc1100_init();
  LED_ON(LED_RED);

  /* On envoie sur le lien serie */
  while(1) {
    int16_t from_serie;
    LED_OFF(LED_GREEN); /* Rx */
    LED_OFF(LED_BLUE); /* Tx */
    from_serie = dev_get();
    if (from_serie != -1) {
      LED_ON(LED_GREEN);
    } else {
      LED_ON(LED_RED);
    }

    for(i=0;i<20000;i++);
  }

  return 0;
}
