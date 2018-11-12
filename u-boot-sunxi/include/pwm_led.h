#ifndef __PWM_LED_H__
#define __PWM_LED_H__

int pwm_led_init(void);
int pwm_led_set(int red, int blue, int green);
int pwm_led_test(void);

#endif
