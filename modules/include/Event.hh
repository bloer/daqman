/** @file Event.hh
    @brief Defines the basic Event class
    @author bloer
    @ingroup modules
*/

#ifndef EVENT_h
#define EVENT_h

#include "RawEvent.hh"
#include "EventData.hh"
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<EventData> EventDataPtr;

class BaseModule;

/** @class Event
    @brief Wrapper class that holds raw and processed data for a trigger.
    
    Handles processing of the EventData by individual modules when threaded.
    @ingroup modules
*/
class Event{
public:
  /// Constructor takes pointer to raw event
  Event(RawEventPtr raw);
  /// Destructor 
  ~Event();
  /// Get a pointer to the raw event portion
  RawEventPtr GetRawEvent(){ return _raw_event; }
  /// Get a pointer to the processed data portion
  EventDataPtr GetEventData(){ return _event_data; }
      
private:
  RawEventPtr _raw_event;       ///< pointer to raw data segment
  EventDataPtr _event_data;     ///< pointer to processed data segment
};

typedef boost::shared_ptr<Event> EventPtr;
#endif
