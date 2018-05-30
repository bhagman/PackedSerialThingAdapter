/*
|| SliderThing
|| - An example Project Things Oryng Wiring/Arduino sketch demonstrating multiple Things
||   on a single board. (An LED and a slider potentiometer - or any potentiometer)
||
|| Things:
|| - Slider - "value"
|| - LED - "on"
|| - Millis - "millis", and "micros"
||
|| More notes at the bottom.
*/


#include <PackedSerialThingAdapter.h>

#define SLIDERPIN A0
#define LEDPIN LED_BUILTIN

// This is our adapter which does all the important communication with the gateway.
PackedSerialThingAdapter adapter = PackedSerialThingAdapter("SliderThing", "Oryng SliderThing");

// Our Things
ThingDevice ledThing = ThingDevice("LED", "Slider LED", ONOFFLIGHT);
ThingDevice sliderThing = ThingDevice("Slider", "Wheeee!", THING);

// Property for LED
ThingPropertyBoolean ledOn = ThingPropertyBoolean("on", "LED ON/OFF");

// Property for Slider
ThingPropertyNumber sliderProperty = ThingPropertyNumber("value", "Slider value");


// A helper class for working with the trim pot
class SlidePot
{
  public:
    SlidePot(int pinNumber, int segmentCount = 100)
    {
      _pin = pinNumber;
      _segments = segmentCount;
    }

    bool changed(void)
    {
      sample();

      // Check for a stable value, +/- 2 jitter
      if (map(constrain(_rawValue + 2, 0, 1023), -1, 1024, 0, _segments - 1) == map(constrain(_rawValue - 2, 0, 1023), -1, 1024, 0, _segments - 1))
      {
        if (map(_rawValue, 0, 1023, 0, _segments - 1) != _currentValue)
          _currentValue = map(_rawValue, 0, 1023, 0, _segments - 1);
      }

      // if accepted, then check if it is different
      if (_currentValue != _lastValue)
        return true;
      else
        return false;
    }

    uint16_t value(void)
    {
      _lastValue = _currentValue;
      return _lastValue;
    }

    void sample(void)
    {
      _rawValue = analogRead(_pin);
    }

  private:
    int _pin;
    uint16_t _segments;
    //uint32_t _lastUpdate; TODO -- ensure that sample rate is below threshold
    int16_t _rawValue;
    uint16_t _lastValue;
    uint16_t _currentValue;
};


// Our SlidePot instance
SlidePot sp = SlidePot(SLIDERPIN, 20);


void setup()
{
  // Set up our LED
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  ledOn.value = false;

  // Add our one and only property to LED Thing
  ledThing.addProperty(ledOn);

  // Add the sliderProperty to the sliderThing
  sliderThing.addProperty(sliderProperty);

  // Finally, add the two things to the adapter
  adapter.addDevice(ledThing);
  adapter.addDevice(sliderThing);

  // ... and start the adapter.
  adapter.begin(); // Automatically sets up Serial to default bit rate
}

void loop()
{
  static uint32_t lastChange = 0;

  // Take care of Adapter communication
  adapter.update();

  // Only update the sliderThing property twice every second (every 500ms)
  if ((millis() - lastChange) > 500)
  {
    if (sp.changed())
    {
      sliderProperty.setValue(sp.value());
    }

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
