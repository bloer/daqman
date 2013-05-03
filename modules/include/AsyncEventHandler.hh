/** @file AsyncEventHandler.hh
    @brief Defines class for asyncronous event processing
    @author bloer
    @ingroup modules
*/

#ifndef ASYNC_EVENT_HANDLER_h
#define ASYNC_EVENT_HANDLER_h

#ifndef SINGLETHREAD
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread.hpp"
#endif

#include "Event.hh"
#include <vector>

class BaseModule;

/** @class AsyncEventHandler
    @brief Class which processes events in asyncronous batches for real-time daq monitoring
    @ingroup modules
*/

class AsyncEventHandler{
public:
  AsyncEventHandler();
  ~AsyncEventHandler();
  
  /// Clear all registered modules and receivers
  void Reset();
  
  /// Add a module to be handled by this process
  int AddModule(BaseModule* mod, bool register_to_eventhandler = true);
  
  /// Register another handler to receive processed events from this batch
  int AddReceiver(AsyncEventHandler* receiver);
  
  /// Process one event
  int Process(EventPtr evt);
  
  /// Start running in a new thread
  int StartRunning();
  /// Stop running in a separate thread
  int StopRunning();
  /// Are we running right now?
  bool IsRunning(){ return _running; }
  
  ///Set the time to sleep in between event processing
  void SetSleepMillisec(int sleeptime){ _sleeptime = sleeptime;}
  ///Get the time to sleep between event processing
  int GetSleepMillisec() const { return _sleeptime; }
  
  ///Set the blocking status
  void SetBlockingStatus(bool blocking){ _blocking = blocking; }
  ///Get the blocking status
  bool GetBlockingStatus(){ return _blocking; }
  
  /// Should not be called directly; necessary for threading
  void operator()();  

private:
  bool _running;            ///< is our thread going?
  int _sleeptime;           ///< time to sleep in ms between events
  bool _blocking;           ///< Do we block new process requests if busy?
  std::vector<BaseModule*> _modules;          ///< modules to process with
  std::vector<AsyncEventHandler*> _receivers; ///< processors to receive events
  EventPtr _next_event;                  ///< next event ready for processing
#ifndef SINGLETHREAD
  boost::condition_variable _event_ready; ///< signal wakeup
  boost::mutex _event_mutex;      ///< control access to next_event
  boost::shared_ptr<boost::thread> _threadptr; ///< manage our own thread
#endif
};  

#endif
