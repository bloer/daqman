/** @file MCEvent.hh
    @brief Defines the basic MCEvent class
    @author bloer
    @ingroup modules
*/

#ifndef MCEVENT_h
#define MCEVENT_h

#ifndef SINGLETHREAD
#include <set>
#include <map>
#include "boost/thread/condition.hpp"
#include "boost/thread/mutex.hpp"
#endif

#include "MCRawEvent.hh"

#include "EventData.hh"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
typedef boost::shared_ptr<EventData> MCEventDataPtr;

class BaseModule;

/** @class MCEvent
    @brief Wrapper class that holds raw and processed data for a trigger.
    
    Handles processing of the MCEventData by individual modules when threaded.
    @ingroup modules
*/
class MCEvent : public boost::enable_shared_from_this<MCEvent>{
public:
  /// Constructor takes pointer to raw event
  MCEvent(MCRawEventPtr raw);
  /// Destructor 
  ~MCEvent();
  /// Get a pointer to the raw event portion
  MCRawEventPtr GetRawEvent(){ return _raw_event; }
  /// Get a pointer to the processed data portion
  MCEventDataPtr GetEventData(){ return _event_data; }
  /// Evaluate dependencies and possibly call processing for a module
  // int WaitForModule(BaseModule* mod);
  // /// Notify waiting modules that this one has finished processing
  // void ModuleHasFinished(BaseModule* mod);
    
private:
  MCRawEventPtr _raw_event;       ///< pointer to raw data segment
  MCEventDataPtr _event_data;     ///< pointer to processed data segment
// #ifndef SINGLETHREAD
//   std::set<BaseModule*> _finished_modules;  ///< Modules which have finished
//   boost::mutex _modules_mutex;  ///< mutex to lock the list of finished modules
//   /// Notify waiting modules when one finishes
//   std::map<BaseModule*,boost::condition_variable*> _processing_modules;
// #endif
};

typedef boost::shared_ptr<MCEvent> MCEventPtr;
#endif
