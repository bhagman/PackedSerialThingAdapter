/*
|| PackedSerialThingAdapter.h - Oryng Wiring Framework Library to interface with the Mozilla Things Gateway.
||
||
|| More notes at the bottom.
*/

#pragma once

#include <Arduino.h>
#include <Thing.h>
#include <PackedSerial.h>

// TODO: We fix the bitrate for now -- see @todo below for more info
#define THINGBITRATE 115200


enum ThingAdapterRequest
{
  DEFINEADAPTER       = 0x00,
  DEFINETHINGBYIDX    = 0x01,
  DEFINEPROPERTYBYIDX = 0x02,
  DEFINEEVENTBYIDX    = 0x03,
  DEFINEACTIONBYIDX   = 0x04,
  SETPROPERTY         = 0x05,
  GETPROPERTY         = 0x06,
  PAIR                = 0xfd, // Enable Thing communication with host
  UNPAIR              = 0xfe
};


enum ThingAdapterResponse
{
  DETAILADAPTER       = 0x00,
  DETAILTHINGBYIDX    = 0x01,
  DETAILPROPERTYBYIDX = 0x02,
  DETAILEVENTBYIDX    = 0x03,
  DETAILACTIONBYIDX   = 0x04,
  PROPERTYSTATUS      = 0x05,
  PAIRED              = 0xfd,
  UNPAIRED            = 0xfe,
  ERROR               = 0xff
};


enum PackedSerialThingAdapterError
{
  ERROR_THINGIDX_OUTOFRANGE    = 0x01,
  ERROR_PROPERTYIDX_OUTOFRANGE = 0x02,
  ERROR_THING_NULLPTR          = 0x03,
  ERROR_PROPERTY_NULLPTR       = 0x04,
  ERROR_NOT_PAIRED             = 0xff
};


// PackedSerialThingAdapter - maintains connection between devices and Gateway.
class PackedSerialThingAdapter : public ThingAdapter, public IPacketReceiver
{
  public:
    // constructor
    // NOTES: adapterName is the name used for the board on the gateway.
    PackedSerialThingAdapter(const char *adapterName, const char *adapterDescription)
      : ThingAdapter(adapterName, adapterDescription),
        connected(false)
    {
    }


    uint8_t preparePropertyStatusMessage(uint8_t *messageBuffer, uint8_t index, uint8_t thingIdx, uint8_t propertyIdx)
    {
      ThingDevice *thing = this->getDevice(thingIdx);

      if (thing != nullptr)
      {
        ThingProperty *property = thing->getProperty(propertyIdx);

        if (property != nullptr)
        {
          index = SimplePack::writeUInt8(messageBuffer, ThingAdapterResponse::PROPERTYSTATUS, index);
          index = SimplePack::writeUInt8(messageBuffer, thingIdx, index);
          index = SimplePack::writeUInt8(messageBuffer, propertyIdx, index);

          switch ((ThingPropertyDatatype) property->type)
          {
            case BOOLEAN:
              index = SimplePack::writeUInt8(messageBuffer, ((ThingPropertyBoolean *)property)->getValue(), index);
              break;
            case NUMBER:
              index = SimplePack::writeInt32BE(messageBuffer, ((ThingPropertyNumber *)property)->getValue(), index);
              break;
            case STRING:
              index = SimplePack::writeString(messageBuffer, ((ThingPropertyString *)property)->getValue(), index);
              break;
            default:
              // TODO: invalid datatype
              break;
          }

          return index;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        return 0;
      }
    }


    void onPacketReceive(const uint8_t *data, size_t len)
    {
      // TODO: check len -- Need to keep track of index into data against data required

      uint8_t inputIndex = 0;
      uint8_t resp[32];  // TODO: response size
      uint8_t index = 0;
      uint8_t thingIdx;
      uint8_t propertyIdx;
      boolean formedResponse = false;

      uint8_t request = SimplePack::readUInt8(data, inputIndex);
      inputIndex += 1;

      // Interpret our request
      switch ((ThingAdapterRequest) request)
      {
        case PAIR:
        case UNPAIR:
          // Pair/Unpair incoming parameters:
          //  uint8 - thingIdx
          //
          // Pair/Unpair response:
          //  uint8  - PAIRED/UNPAIRED
          //  uint8  - thingIdx

          // Get thingIdx from request
          thingIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;

          if (thingIdx < this->thingCount)
          {
            ThingDevice *thing = this->getDevice(thingIdx);

            if (thing != nullptr)
            {
              uint8_t responseValue;

              if (((ThingAdapterRequest) request) == ThingAdapterRequest::PAIR)
              {
                thing->paired = true;
                responseValue = ThingAdapterResponse::PAIRED;
              }
              else
              {
                thing->paired = false;
                responseValue = ThingAdapterResponse::UNPAIRED;
              }

              // Now prepare response
              // Response type
              index = SimplePack::writeUInt8(resp, responseValue, index);
              // thingIdx
              index = SimplePack::writeUInt8(resp, thingIdx, index);

              formedResponse = true;
            }
            else
            {
              // Error while getting the device - return an error
              index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
              index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THING_NULLPTR, index);

              formedResponse = true;
            }
          }
          else
          {
            // thingIdx out of range
            index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
            index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THINGIDX_OUTOFRANGE, index);

            formedResponse = true;
          }
          break;
        case DEFINEADAPTER:
          // DefineAdapter incoming parameters:
          // - none
          //
          // DefineAdapter response:
          //  uint8  - DETAILADAPTER
          //  string - adapterName
          //  string - adapterDescription
          //  uint8  - thingCount

          index = SimplePack::writeUInt8(resp, ThingAdapterResponse::DETAILADAPTER, index);
          index = SimplePack::writeString(resp, this->name, index);
          index = SimplePack::writeString(resp, this->description, index);
          index = SimplePack::writeUInt8(resp, this->thingCount, index);

          formedResponse = true;
          break;
        case DEFINETHINGBYIDX:
          // DefineThingByIdx incoming parameters:
          //  uint8  - thingIdx
          //
          // DefineThingByIdx response:
          //  uint8  - DETAILTHINGBYIDX
          //  uint8  - thingIdx
          //  uint8  - thingType
          //  string - thingName
          //  string - thingDescription
          //  uint8  - propertyCount
          //  uint8  - eventCount
          //  uint8  - actionCount

          // Get thingIdx from request
          thingIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;

          if (thingIdx < this->thingCount)
          {
            ThingDevice *thing = this->getDevice(thingIdx);

            if (thing != nullptr)
            {
              // Now prepare response
              // Response type
              index = SimplePack::writeUInt8(resp, ThingAdapterResponse::DETAILTHINGBYIDX, index);
              // thingIdx, id, name, type, description, propertyCount, eventCount, actionCount
              index = SimplePack::writeUInt8(resp, thingIdx, index);
              index = SimplePack::writeUInt8(resp, thing->type, index);
              index = SimplePack::writeString(resp, thing->name, index);
              index = SimplePack::writeString(resp, thing->description, index);
              index = SimplePack::writeUInt8(resp, thing->propertyCount, index);
              index = SimplePack::writeUInt8(resp, thing->eventCount, index);
              index = SimplePack::writeUInt8(resp, thing->actionCount, index);

              formedResponse = true;
            }
            else
            {
              // Error while getting the device - return an error
              index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
              index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THING_NULLPTR, index);

              formedResponse = true;
            }
          }
          else
          {
            // thingIdx out of range
            index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
            index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THINGIDX_OUTOFRANGE, index);

            formedResponse = true;
          }
          break;
        case DEFINEPROPERTYBYIDX:
          // DefinePropertyByIdx incoming parameters:
          //  uint8 - thingIdx
          //  uint8 - propertyIdx
          //
          // DefinePropertyByIdx response:
          //  uint8  - DETAILPROPERTYBYIDX
          //  uint8  - thingIdx
          //  uint8  - propertyIdx
          //  uint8  - propertyType
          //  string - propertyName
          //  string - propertyDescription
          //  x      - value

          // Get thingIdx from request
          thingIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;
          // Get propertyIdx from request
          propertyIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;

          if (thingIdx < this->thingCount)
          {
            ThingDevice *thing = this->getDevice(thingIdx);

            if (thing != nullptr)
            {
              if (propertyIdx < thing->propertyCount)
              {
                ThingProperty *property = thing->getProperty(propertyIdx);

                if (property != nullptr)
                {
                  // TODO: check datatype validity -- respond with error if invalid
                  // Now prepare response
                  // Response type
                  index = SimplePack::writeUInt8(resp, ThingAdapterResponse::DETAILPROPERTYBYIDX, index);
                  // thingIdx, propertyIdx, name, type, value
                  index = SimplePack::writeUInt8(resp, thingIdx, index);
                  index = SimplePack::writeUInt8(resp, propertyIdx, index);
                  index = SimplePack::writeUInt8(resp, property->type, index);
                  index = SimplePack::writeString(resp, property->name, index);
                  index = SimplePack::writeString(resp, property->description, index);
                  // Identify type and cast to get value
                  switch ((ThingPropertyDatatype) property->type)
                  {
                    case BOOLEAN:
                      index = SimplePack::writeUInt8(resp, ((ThingPropertyBoolean *)property)->getValue(), index);
                      break;
                    case NUMBER:
                      index = SimplePack::writeInt32BE(resp, ((ThingPropertyNumber *)property)->getValue(), index);
                      break;
                    case STRING:
                      index = SimplePack::writeString(resp, ((ThingPropertyString *)property)->getValue(), index);
                      break;
                    default:
                      // TODO: invalid datatype
                      break;
                  }

                  formedResponse = true;
                }
                else
                {
                  // Error while getting the property - return an error
                  index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
                  index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_PROPERTY_NULLPTR, index);

                  formedResponse = true;
                }
              }
              else
              {
                // propertyIdx out of range
                index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
                index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_PROPERTYIDX_OUTOFRANGE, index);

                formedResponse = true;
              }
            }
            else
            {
              // Error while getting the device - return an error
              index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
              index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THING_NULLPTR, index);

              formedResponse = true;
            }
          }
          else
          {
            // thingIdx out of range
            index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
            index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THINGIDX_OUTOFRANGE, index);

            formedResponse = true;
          }

          break;
        case SETPROPERTY:
          // SetProperty incoming parameters:
          //  uint8 - thingIdx
          //  uint8 - propertyIdx
          //  x     - value
          //
          // SetProperty response:
          //  uint8 - PROPERTYSTATUS
          //  uint8 - thingIdx
          //  uint8 - propertyIdx
          //  x     - value
        case GETPROPERTY:
          // getProperty incoming parameters:
          //  uint8 - thingIdx
          //  uint8 - propertyIdx
          //
          // getProperty response:
          //  uint8 - PROPERTYSTATUS
          //  uint8 - thingIdx
          //  uint8 - propertyIdx
          //  x     - value

          // Get thingIdx from request
          thingIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;
          // Get propertyIdx from request
          propertyIdx = SimplePack::readUInt8(data, inputIndex);
          inputIndex += 1;

          if (thingIdx < this->thingCount)
          {
            ThingDevice *thing = this->getDevice(thingIdx);

            if (thing != nullptr)
            {
              if (thing->paired)
              {
                if (propertyIdx < thing->propertyCount)
                {
                  ThingProperty *property = thing->getProperty(propertyIdx);

                  if (property != nullptr)
                  {
                    // First, if we are setting...
                    if ((ThingAdapterRequest) request == SETPROPERTY)
                    {
                      // Get data and set the property value
                      switch ((ThingPropertyDatatype) property->type)
                      {
                        case BOOLEAN:
                          ((ThingPropertyBoolean *) property)->setValue(SimplePack::readUInt8(data, inputIndex) ? true : false);
                          inputIndex += 1;
                          break;
                        case NUMBER:
                          ((ThingPropertyNumber *) property)->setValue(SimplePack::readInt32BE(data, inputIndex));
                          inputIndex += 4;
                          break;
                        case STRING:
                          // TODO: Un-klugify... bah getValue!  Need to use setValue...?
                          // char newS[32];  // This uses up more RAM
                          // SimplePack::readString(newS, data, inputIndex);
                          // ((ThingPropertyString *) property)->setValue(newS);
                          SimplePack::readString(((ThingPropertyString *) property)->getValue(), data, inputIndex);
                          inputIndex += strlen(((ThingPropertyString *) property)->getValue());
                          // inputIndex += strlen(newS);
                          break;
                        default:
                          // TODO: invalid datatype
                          break;
                      }

                      // Since we fall through here, we need to invalidate the changed signal, so
                      // update() doesn't pick it up again later.
                      property->changed = false;
                    }

                    // Next (or if GetProperty)... build and send PropertyStatus
                    index = preparePropertyStatusMessage(resp, index, thingIdx, propertyIdx);
                    formedResponse = true;
                  }
                  else
                  {
                    // Error while getting the property - return an error
                    index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
                    index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_PROPERTY_NULLPTR, index);

                    formedResponse = true;
                  }
                }
                else
                {
                  // propertyIdx out of range
                  index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
                  index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_PROPERTYIDX_OUTOFRANGE, index);

                  formedResponse = true;
                }
              }
              else
              {
                // Not paired
                index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
                index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_NOT_PAIRED, index);

                formedResponse = true;
              }
            }
            else
            {
              // Error while getting the device - return an error
              index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
              index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THING_NULLPTR, index);

              formedResponse = true;
            }
          }
          else
          {
            // thingIdx out of range
            index = SimplePack::writeUInt8(resp, ThingAdapterResponse::ERROR, index);
            index = SimplePack::writeUInt8(resp, PackedSerialThingAdapterError::ERROR_THINGIDX_OUTOFRANGE, index);

            formedResponse = true;
          }
          break;
        default:
          break;
      }

      if (formedResponse)
        serialConn.send(resp, index);
    }


    void begin(Stream &stream)
    {
      // Register our packet processing function with PackedSerial
      serialConn.setStream(stream);
      // serialConn.setPacketHandler([this](const uint8_t* buffer, size_t size) { this->onPacketReceive(buffer, size); });
      serialConn.setPacketReceiver(this);

      // TODO: other preps?
    }


    void begin(uint32_t bps)
    {
      Serial.begin(bps);
      #if ARDUINO >= 100
      while (!Serial);
      #endif
      begin(Serial);
    }


    void update()
    {
      // 1. check if there is incoming data
      // 2. handle request
      // 3. check for changes on all properties of devices
      // 3a. if something changed, send PropertyStatus message

      serialConn.update();  // This handles incoming requests

      // Now check for changes
      // iterate through all properties and preparePropertyStatusMessage for "changed" properties
      ThingDevice *thing = this->firstDevice;
      uint8_t thingIdx = 0;

      while (thing != nullptr)
      {
        if (thing->paired)
        {
          ThingProperty *property = thing->firstProperty;
          uint8_t propertyIdx = 0;

          while (property != nullptr)
          {
            if (property->changed)
            {
              // property has changed, send PropertyStatus message, and reset.
              uint8_t message[32];  // TODO: message size
              uint8_t index = 0;

              index = this->preparePropertyStatusMessage(message, index, thingIdx, propertyIdx);
              serialConn.send(message, index);

              property->changed = false;
            }
            property = property->next;
            propertyIdx++;
          }
        }

        thing = thing->next;
        thingIdx++;
      }
    }


  private:
    PackedSerial serialConn;
    boolean connected;
    // uint32_t lastCommunication; // TODO: might need this to be a unix timestamp
};


/*
||
|| @author         Brett Hagman <bhagman@roguerobotics.com>
|| @url            http://roguerobotics.com/
|| @url            http://oryng.org/
|| @contribution   James Hobin <github:hobinjk>
|| @contribution   Alexander Brevig <https://alexanderbrevig.com/>
||
|| @description
|| | PackedSerialThingAdapter Library
|| |
|| | Requests supported:
|| |  DEFINEADAPTER       = 0x00,
|| |  DEFINETHINGBYIDX    = 0x01,
|| |  DEFINEPROPERTYBYIDX = 0x02,
|| |  DEFINEEVENTBYIDX    = 0x03,
|| |  DEFINEACTIONBYIDX   = 0x04,
|| |  SETPROPERTY         = 0x05,
|| |  GETPROPERTY         = 0x06,
|| |  PAIR                = 0xfd,
|| |  UNPAIR              = 0xfe
|| |
|| | Responses:
|| |  DETAILADAPTER       = 0x00,
|| |  DETAILTHINGBYIDX    = 0x01,
|| |  DETAILPROPERTYBYIDX = 0x02,
|| |  DETAILEVENTBYIDX    = 0x03,
|| |  DETAILACTIONBYIDX   = 0x04,
|| |  PROPERTYSTATUS      = 0x05,
|| |  PAIRED              = 0xfd,
|| |  UNPAIRED            = 0xfe,
|| |  ERROR               = 0xff
|| |
|| #
||
|| @notes
|| |
|| | RE: Flash strings on AVR (and other Harvard architecture devices)
|| | Unfortunately, although the C compiler in GCC has the ability to use a __flash namespace for
|| | variables stored in flash memory, the same support will likely never make it into the C++
|| | compiler in GCC.  Hence, the kluge in this set of classes.
|| |
|| | http://ithare.com/modified-harvard-architecture-clarifying-confusion/#comment-3195
|| | https://www.avrfreaks.net/forum/avr-g-lacks-flash
|| |
|| #
||
|| @todo
|| |
|| | Set bitrate to common value on startup, then allow adapter to announce/negotiate speed change.
|| | (e.g. .begin(1000000) -- would allow adapter to switch up to 1000000 bps after initial communication)
|| | Bitrate is currently fixed to 115200.
|| #
||
|| @license Please see LICENSE.
||
*/
