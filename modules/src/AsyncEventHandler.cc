#include "AsyncEventHandler.hh"
#include "BaseModule.hh"
#include "EventHandler.hh"

#ifndef SINGLETHREAD
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"
#endif

AsyncEventHandler::AsyncEventHandler() : _running(false), _sleeptime(0), 
					 _blocking(false)
{}

AsyncEventHandler::~AsyncEventHandler()
{
  if(_running){
    //this really shouldn't happen, but make sure we end properly...
    StopRunning();
  }
}

void AsyncEventHandler::Reset()
{
  _modules.clear();
  _receivers.clear();
}

int AsyncEventHandler::AddModule(BaseModule* mod,
				 bool register_to_eventhandler,
				 bool register_parameters)
{
  if(mod){
    _modules.push_back(mod);
    if(register_to_eventhandler) {
      EventHandler::GetInstance()->AddModule(mod, false, register_parameters);
    }
  }
  return 0;
}

int AsyncEventHandler::AddReceiver(AsyncEventHandler* receiver)
{
  if(receiver)
    _receivers.push_back(receiver);
  return _receivers.size();
}

int AsyncEventHandler::Process(EventPtr evt)
{
  //noop if no multithread
#ifdef SINGLETHREAD
  Message(WARNING)<<"Attempt to use AsyncEventHandler with multithreading disabled!\n";
#else
  if(_running){
    boost::mutex::scoped_lock lock(_event_mutex);
    while(_blocking && _next_event){
      //only go if _next_event is not filled
      lock.unlock();
      boost::this_thread::sleep(boost::posix_time::millisec(1));
      lock.lock();
    }
    _next_event = evt; 
    _event_ready.notify_one();
  }
#endif
  return 0;
}

int AsyncEventHandler::StartRunning()
{
#ifndef SINGLETHREAD
  //if we have no enabled modules, don't run
  int enabled_modules = 0;
  for(size_t i=0; i<_modules.size(); ++i){
    if(_modules[i]->enabled)
      ++enabled_modules;
  }
  if(enabled_modules == 0){
    Message(DEBUG)<<"AsyncEventHandler not running: no enabled modules.\n";
    return 1;
  }
  //start a thread
  _running = true;
  typedef boost::shared_ptr<boost::thread> _tp;
  _threadptr = _tp(new boost::thread(boost::ref(*this)));
  Message(DEBUG)<<"AsyncEventHandler running on thread "<<_threadptr->get_id()
		<<" with sleeptime "<<_sleeptime<<" ms "
		<<" contains "<<_modules.size()<<" modules.\n";
  return 0;
#else
  Message(WARNING)<<"Attempt to use AsyncEventHandler with multithreading disabled!\n";
  return 1; //only here if singlethread mode compiled
#endif
}

int AsyncEventHandler::StopRunning()
{
  if(!_running)
    return 1;
  _running = false;
#ifndef SINGLETHREAD
  //wake up the thread if it's sleeping
  Message(DEBUG)<<"Ending AsyncEventHandler on thread "<<_threadptr->get_id()
		<<"...\n";
  Process(EventPtr());
  _threadptr->join();
#endif
  return 0;
  
}

void AsyncEventHandler::operator()()
{
#ifndef SINGLETHREAD
  //need a dummy mutex for condition_variable to work
  boost::mutex dummy_mutex;
  EventPtr current_event;
  while(_running){
    boost::mutex::scoped_lock lock(_event_mutex);
    if(!_next_event || _next_event == current_event){
      _event_ready.timed_wait(lock,boost::posix_time::millisec(1000));
      continue;
    }
    //if we get here, process the event
    current_event = _next_event;
    if(!_blocking)
      lock.unlock();
    for(size_t i=0; i<_modules.size(); ++i){
      if(_modules[i]->enabled){
	_modules[i]->HandleEvent(current_event);
	//boost::this_thread::sleep(boost::posix_time::microsec(2) );
      }
    }
    //done processing, hand off to receivers
    for(size_t i=0; i<_receivers.size(); ++i){
      _receivers[i]->Process(current_event);
    }
    if(!_blocking){
      if(_sleeptime>0){
	boost::this_thread::sleep(boost::posix_time::millisec(_sleeptime));
      }
      else
	boost::this_thread::yield();
    }
    else{
      _next_event = EventPtr();
    }
  }
#endif
}
