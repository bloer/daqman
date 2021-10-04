/** @file BaseDaq.hh
    @brief Defines the BaseDaq abstract DAQ inteface class
    @author bloer
    @ingroup daqman
*/

#ifndef BASEDAQ_h
#define BASEDAQ_h

#include <queue>
#include <string>
#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "RawEvent.hh"


/** @class BaseDaq
 *  @brief Abstract class to define a DAQ hardware interface
 *
 *  This class is an interface between the main DAQ program and the 
 *  underlying subprocesses which handle threads, etc. 
 *  Throws error if instantiated more than once at a time.
 *
 *  @ingroup daqman
*/
class BaseDaq 
{
public:
  /** @enum STATUS
   *  @brief defines possible status conditions of the daq
   */
  enum STATUS{ 
    NORMAL=0, ///< enum value: Status is normal
    INIT_FAILURE, ///< enum value: Initialization Failure
    BUS_ERROR, ///< enum value: internal BUS Error
    COMM_ERROR, ///< enum value: Communication Error
    GENERIC_ERROR ///< enum value: Generic Error
  };
  

  ///Default constructor. Throws exception if more than one constructed     
  BaseDaq() throw(std::runtime_error);
  
  /// Virtual destructor
  virtual ~BaseDaq();
  
  /// Get the status of the DAQ 
  STATUS GetStatus(){ return _status; }
  
  /// Resets Status to normal
  void ResetStatus(){ _status=NORMAL; }

  /**
   *  @brief Initializes hardware for a run.
   * 
   *  Must be overriden by concrete implementation
   *  Returns 0 if no error occurred.
   */
  virtual int Initialize() = 0;
  
  /**
   *  Updates the parameters after initlialization has occurred but before 
   *  starting the run.
   *  Must be overridden by concrete implementation
   *   Returns 0 if no error encountered
   */
  virtual int Update() = 0;
  
  /** 
   *   Performs generic calibration tasks.  Returns 0 if no errors
   */
  virtual int Calibrate() {return 0;}
  
  /** Prepares for run, launches seperate thread which (calls operator()() 
   *  which calls DataAcquistionLoop().
   *  Clear all memory buffers, get the hardware into a ready state,
   *  and wait for a trigger
   *  Returns 0 if no failure encountered.
  */
  virtual int StartRun();
   
  /** Checks to see if the next event is ready, and if so, returns it.
     If not ready, it sleeps the current thread and waits for notification 
     that a new event is ready.  If timeout (microsec) expires, returns null
  */
  RawEventPtr GetNextEvent(int timeout=-1);
  
  ///Queries how many events are waiting in the memory buffer
  int GetEventsReady(){ return _events_queue.size(); }
  
  /**
     Run is aborted. 
     
     Statistics on the total amount of live time, data downloaded,
     triggers handled, dead time, etc. are made available to the DAQ.
     All threads, instances, etc. created by StartRun are cleared.
     All digitizers are reset.
  */
  virtual int EndRun(bool force=false);


  /// called ONLY by the std thread; should not be called directly ever
  void operator()(){ DataAcquisitionLoop(); }

protected:
  
  /**
     This is the main acquisition loop which must be overridden by the 
     concrete implementation. Called by StartRun.  
     Incoming events should be pushed into the _events_queue
  */
  virtual void DataAcquisitionLoop()=0;
  /// Send a new RawEvent to whoever is waiting
  void PostEvent(RawEventPtr event);
  
  static bool _is_constructed; ///< does an instance already exist?
  STATUS _status; ///< the status of the daq
  
  bool _is_running; ///< is the daq running?
  std::thread _daq_thread; ///< thread controlling the daq
  std::queue<RawEventPtr> _events_queue; ///< queue of raw events
  std::mutex _queue_mutex; ///< mutex governing the events queue
  std::condition_variable _event_ready; ///<  condition signalling new event
  std::condition_variable _event_taken; ///< signal a spot ready in queue
  static const size_t MAX_QUEUE_SIZE = 10; ///< max events allowed in queue
  int _n_queuesize_warnings;   ///< number of queue overflow warnings generated
};

#endif
