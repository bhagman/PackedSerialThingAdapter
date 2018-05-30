/*
|| LEDWithMillis
|| - An example Project Things Oryng Wiring/Arduino sketch demonstrating multiple Things
||   on a single board.
||
|| Things:
|| - LED - "on"
|| - Time - "millis", and "micros"
||
|| More notes at the bottom.
*/

#include <PackedSerialThingAdapter.h>

#define LEDPIN LED_BUILTIN

// This is our adapter which does all the important communication with the gateway.
PackedSerialThingAdapter adapter = PackedSerialThingAdapter("LEDMillis", "Oryng Demoboard 1");

// Our Things
ThingDevice ledThing = ThingDevice("LED", "Twinkly LED", ONOFFLIGHT);
ThingDevice timeThing = ThingDevice("Time", "OUTATIME", THING);

// Property for LED
ThingPropertyBoolean ledOn = ThingPropertyBoolean("on", "LED ON/OFF");

// Properties for Millis
ThingPropertyNumber millisTime = ThingPropertyNumber("millis", "Current millis");
ThingPropertyNumber microsTime = ThingPropertyNumber("micros", "Current micros");

void setup()
{
  // Set up our LED
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  ledOn.value = false;

  // Add our one and only property to ledThing
  ledThing.addProperty(ledOn);

  // Add the two properties to timeThing
  timeThing.addProperty(millisTime);
  timeThing.addProperty(microsTime);

  // Finally, add the two things to the adapter
  adapter.addDevice(ledThing);
  adapter.addDevice(timeThing);

  // ... and start the adapter.
  adapter.begin(); // Automatically sets up Serial to default bit rate
}

void loop()
{
  static uint32_t lastChange = 0;

  // Take care of Adapter communication
  adapter.update();

  // Only update the Thing property once every second
  if ((millis() - lastChange) > 1000)
  {
    millisTime.setValue(millis());
    microsTime.setValue(micros());

    lastChange = millis();
  }

  // Keep our LED up to date
  digitalWrite(LEDPIN, ledOn.value);
}

/*
||
|| @author         Brett Hagman <bhagman@roguerobotics.com>
|| @url            http://roguerobotics.com/
|| @url            http://oryng.org/
||
|| @description
|| | An example Project Things Oryng Wiring/Arduino sketch demonstrating multiple Things
|| | on a single board.
|| #
||
|| @notes
|| |
|| #
||
|| @todo
|| |
|| #
||
|| @license Please see LICENSE.
||
*/
