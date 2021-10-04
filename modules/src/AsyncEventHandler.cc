#include "AsyncEventHandler.hh"
#include "BaseModule.hh"
#include "EventHandler.hh"

#ifndef SINGLETHREAD
#include <thread>
#include <mutex>
#include <chrono>
typedef std::unique_lock<std::mutex> scoped_lock;
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
    scoped_lock lock(_event_mutex);
    while(_blocking && _next_event){
      //only go if _next_event is not filled
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    if(_modules[i]->enabled){
      ++enabled_modules;
    }
  }
  if(enabled_modules == 0){
    Message(DEBUG)<<"AsyncEventHandler not running: no enabled modules.\n";
    return 1;
  }
  //start a thread
  _running = true;
  typedef std::shared_ptr<std::thread> _tp;
  _threadptr = _tp(new std::thread(std::ref(*this)));
  Message(DEBUG)<<"AsyncEventHandler running on thread "<<_threadptr->get_id()
		<<" with sleeptime "<<_sleeptime<<" ms "
		<<" contains "<<enabled_modules<<" enabled modules.\n";
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
  std::mutex dummy_mutex;
  EventPtr current_event;
  while(_running){
    scoped_lock lock(_event_mutex);
    if(!_next_event || _next_event == current_event){
      _event_ready.wait_for(lock,std::chrono::milliseconds(1000));
      continue;
    }
    //if we get here, process the event
    current_event = _next_event;
    if(!_blocking)
      lock.unlock();
    for(size_t i=0; i<_modules.size(); ++i){
      if(_modules[i]->enabled){
	_modules[i]->HandleEvent(current_event);
	//std::this_thread::sleep_for(std::chrono::microseconds(2) );
      }
    }
    //done processing, hand off to receivers
    for(size_t i=0; i<_receivers.size(); ++i){
      _receivers[i]->Process(current_event);
    }
    if(!_blocking){
      if(_sleeptime>0){
	std::this_thread::sleep_for(std::chrono::milliseconds(_sleeptime));
      }
      else
	std::this_thread::yield();
    }
    else{
      _next_event = EventPtr();
    }
  }
#endif
}
