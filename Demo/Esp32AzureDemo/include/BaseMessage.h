
#ifndef __BASEMESSAGE_H__
#define __BASEMESSAGE_H__

#include <Arduino.h>
#include <ArduinoJson.h>

#define EVENT_TYPE_TOKEN  "eventType"
#define SOURCE_TOKEN      "source"
#define DATA_TOKEN        "data"

class BaseMessage 
{    

public:

    enum class eventType      { none, openDoor, closeDoor };

    BaseMessage()
    {
        clear();
    }

    BaseMessage& clear(void)
    {
        _eventType   = BaseMessage::eventType::none;
        _source.clear();
        _data.clear();
        _payload.clear();

        return *this;
    }

    BaseMessage& setEvent(BaseMessage::eventType evtType, String source, String data)
    {
        _eventType   = evtType;
        _source      = source;
        _data        = data;

        return *this;
    }
    
    BaseMessage::eventType getEventType()
    {
        return _eventType;
    }
    
    String getSource()
    {
        return _source;
    }
    
    String getData()
    {
        return _data;
    }
    
    String serialize()
    {        
        String ret;

        DynamicJsonDocument doc(256);

        doc[EVENT_TYPE_TOKEN]  = (int)_eventType;
        doc[SOURCE_TOKEN]      = _source;
        doc[DATA_TOKEN]        = _data;

        serializeJson(doc, ret);

        return ret;
    }

    bool deserialize(String buffer)
    {
        clear();

        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, buffer.c_str()) != DeserializationError::Code::Ok)
            return false;
        
        _eventType    = (BaseMessage::eventType)(int)doc[EVENT_TYPE_TOKEN];
        _source       = doc[SOURCE_TOKEN].as<String>();;
        _data         = doc[DATA_TOKEN].as<String>();;

       return true;
    }

    BaseMessage * clone()
    {
        BaseMessage *pRet = new BaseMessage();
        pRet->deserialize(this->serialize());

        return pRet;
    }
        
private:

    eventType   _eventType;
    String      _source;
    String      _data;    
    String      _payload;
};

#endif // __BASEMESSAGE_H__
