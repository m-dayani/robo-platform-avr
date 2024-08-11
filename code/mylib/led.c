#include "led.h"

static uchar ledStat = 0;

void initLED(void)
{
	LED_DDR |= 1 << LED_BIT;
}

void setLED(uchar newValue)
{
	ledStat = newValue;
	if (ledStat)
	{
		LED_PORT |= 1 << LED_BIT; // LED on
	}
	else
	{
		LED_PORT &= ~(1 << LED_BIT); // LED off
	}
}

void toggleLED(void)
{
	if (ledStat)
	{
		setLED(0);
	}
	else
	{
		setLED(1);
	}
}

void toggle_led(void) {
  if (ledStat == 0) {
    ledStat = 1;
  }
  else {
    ledStat = 0;
  }
#ifdef ARDUINO
  digitalWrite(LED_BUILTIN, ledStat);
#endif
}
