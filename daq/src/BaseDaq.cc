/*----------------------------------------------------------
  BaseDaq.cc
  
  These are the definitions of functions declared in
  BaseDaq.hh
  For blackbox explanations of these functions, please see
  the header file
  

  ---------------------------------------------------------*/


#include "BaseDaq.hh"
#include "RawEvent.hh"
#include <iostream>
#include <stdexcept>
#include <boost/ref.hpp>
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "Message.hh"

bool BaseDaq::_is_constructed = false;

BaseDaq::BaseDaq() throw(std::runtime_error): 
  _status(NORMAL), _is_running(false)
{
  if(_is_constructed){
    //only one instance allowed!
    Message(EXCEPTION)<<"BaseDaq constructor called, but only one instance is allowed!"<<std::endl;
    throw std::runtime_error("multiple BaseDaq construction");
  }
  _is_constructed=true;
  _n_queuesize_warnings = 0;
}

BaseDaq::~BaseDaq()
{
//what needs to be deleted here??
  if(_is_running) EndRun();
  _is_constructed=false;
}


int BaseDaq::StartRun()
{
  if (_is_running)
    {
      Message(ERROR)<<"Cannot start a new run without terminating the previous one!"<<std::endl;
      return -1;
    }
  else 
    _is_running = true;
  
  _n_queuesize_warnings = 0;
  while(!_events_queue.empty())
    _events_queue.pop();
  //start new thread and run collect data
  Message(DEBUG)<<"Starting daq thread..."<<std::endl;
  _daq_thread = boost::thread(boost::ref(*this));
  
  return 0;
}

int BaseDaq::EndRun(bool force)
{
    if (!_is_running)
    {
      Message(ERROR)<<"Cannot terminate non-existant run.";
      return 1;
    }
    else _is_running = false;
    if(force) _daq_thread.interrupt();
    _daq_thread.join();
    /*while(!_events_queue.empty()){
      RawEventPtr next = _events_queue.front();
      next->GetThreadPointer()->join();
      _events_queue.pop();
      }*/
    
    return 0;    
}

RawEventPtr BaseDaq::GetNextEvent(int timeout)
{
  if(GetStatus() != NORMAL){
    return RawEventPtr();
  }
  //lock the mutex guarding the processed event queue
  boost::mutex::scoped_lock lock(_queue_mutex);
  if(!_is_running && _events_queue.empty())
    return RawEventPtr();
  //loop until an event is ready
  while(_events_queue.empty() ){
    if(timeout < 0)
      _event_ready.wait(lock);
    else{
      if(!_event_ready.timed_wait(lock,boost::posix_time::microsec(timeout)))
	return RawEventPtr();
    }
  }
  RawEventPtr next(_events_queue.front());
  _events_queue.pop();
  lock.unlock();
  _event_taken.notify_all();
  return next;
}

void BaseDaq::PostEvent(RawEventPtr event)
{
  boost::mutex::scoped_lock lock(_queue_mutex);
  do{
    if(_events_queue.size() < MAX_QUEUE_SIZE){
      _events_queue.push(event);
      _event_ready.notify_all();
      break;
    }
    //if we get here, the event queue is full
    if(_n_queuesize_warnings++ < 1){
      Message(WARNING)<<_events_queue.size()<<" events waiting "
		      <<"to be processed; trigger rate may be too high.\n"
		      <<"\tThere will be deadtime in this run.\n";
    }
    _event_taken.timed_wait(lock, boost::posix_time::microsec(1000));
    
  }while(_is_running);
    
  
}

