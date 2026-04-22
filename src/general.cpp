#include <Arduino.h>
#include "definitions.h"

#ifdef LED
int led_state = LED_STATE_IDLE;
bool led_toggle = false;

void blink_led(int num_times, int blink_duration) {
  for (int i = 0; i < num_times; i++) {
    digitalWrite(LED, HIGH);
    delay(blink_duration / 2);
    digitalWrite(LED, LOW);
    delay(blink_duration / 2);
  }
}

void update_led_status() {
  switch (led_state) {
    case LED_STATE_IDLE:
      digitalWrite(LED, LOW);
      break;
    case LED_STATE_SCANNING:
      digitalWrite(LED, led_toggle ? HIGH : LOW);
      led_toggle = !led_toggle;
      break;
    case LED_STATE_ATTACKING:
      digitalWrite(LED, HIGH);
      delay(50);
      digitalWrite(LED, LOW);
      delay(50);
      break;
    case LED_STATE_EVIL_TWIN:
      digitalWrite(LED, led_toggle ? HIGH : LOW);
      led_toggle = !led_toggle;
      delay(200);
      break;
    case LED_STATE_SUCCESS:
      blink_led(3, 100);
      led_state = LED_STATE_IDLE;
      break;
    case LED_STATE_ERROR:
      blink_led(5, 50);
      led_state = LED_STATE_IDLE;
      break;
  }
}

void set_led_state(int state) {
  led_state = state;
  led_toggle = false;
}
#endif
