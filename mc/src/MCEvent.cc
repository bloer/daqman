#include "MCEvent.hh"
// #ifndef SINGLETHREAD
// #include "boost/thread/mutex.hpp"
// #endif
#include "BaseModule.hh"

MCEvent::MCEvent(MCRawEventPtr raw) :  _raw_event(raw), 
				 _event_data(new EventData) 
{}

MCEvent::~MCEvent() 
{
  // #ifndef SINGLETHREAD
  // std::map<BaseModule*,boost::condition_variable*>::iterator it;
  // for( it = _processing_modules.begin(); it != _processing_modules.end(); it++){
  //   delete it->second;
  // }
  // #endif
}

// int MCEvent::WaitForModule(BaseModule* mod)
// {
//   if( !mod->enabled) return -1;
// #ifndef SINGLETHREAD
//   boost::mutex::scoped_lock lock(_modules_mutex);
//   // is someone already processing this module?
//   bool processing = _processing_modules.find(mod) != _processing_modules.end();
//   if(!processing){
//     if(mod->noloop){
//       //no one's going to call this, so it's up to us
//       // make sure no one else tries to do it at the same time
//       _processing_modules[mod] = new boost::condition_variable;
//       lock.unlock();
//       return mod->HandleEvent(shared_from_this(),/* process_now =*/ true);
//     }
//     else{
//       // wait for the module's main loop to pick it up
//       _processing_modules[mod] = new boost::condition_variable;
//     }
//   }
//   // wait for it to finish processing elsewhere
//   while( _finished_modules.find(mod) == _finished_modules.end() ){
//     _processing_modules[mod]->wait(lock);
//   }
// #endif
//   return 0;
// }
    
// void MCEvent::ModuleHasFinished(BaseModule* mod)
// {
// #ifndef SINGLETHREAD
//   boost::mutex::scoped_lock lock(_modules_mutex);
//   _finished_modules.insert(mod);
//   if( _processing_modules.find(mod) != _processing_modules.end())
//     _processing_modules[mod]->notify_all();
// #endif
// }
	    
  
