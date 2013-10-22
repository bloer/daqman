//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

#include "MessageHandler.hh"
#include "Message.hh"
#include <time.h>
#include <cctype>
#include <algorithm>
#include <exception>
#include <stdexcept>
#ifndef SINGLETHREAD
#include "boost/ref.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#endif

#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"

namespace MessageLevelConversion{
  std::string EnumToString(MESSAGE_LEVEL level)
  {
    switch(level){
    case DEBUG3: return "DEBUG3";
    case DEBUG2: return "DEBUG2";
    case DEBUG:  return "DEBUG";
    case INFO:   return "INFO";
    case WARNING: return "WARNING";
    case ERROR:   return "ERROR";
    case CRITICAL: return "CRITICAL";
    case EXCEPTION: return "EXCEPTION";
    default:
      std::cerr<<"Unknown MESSAGE_LEVEL passed: "<<(int)(level)<<"!\n";
    }
    //only get here in case of error
    return "";
  }
  
  void convert_to_upper(char& c){ c = std::toupper(c); }
  
  MESSAGE_LEVEL StringToEnum(std::string s)
  {
    //convert the whole string to upper case
    std::for_each(s.begin(),s.end(),convert_to_upper);
    //let DEBUG1 = DEBUG
    if(s == "DEBUG1") s = "DEBUG";
    for(int i=0; i<N_MESSAGE_LEVELS; ++i){
      if(s == EnumToString((MESSAGE_LEVEL(i))) )
	return MESSAGE_LEVEL(i);
    }
    //if we got here, there was an error
    MESSAGE_LEVEL newlevel = 
      MessageHandler::GetInstance()->GetDefaultMessageThreshold();
    std::cerr<<"ERROR: Unknown message level "<<s
	<<"; setting to "<<newlevel<<"\n";
    return newlevel;
  }
}
  

//operator overloads for MESSAGE_LEVEL
std::ostream& operator<<(std::ostream& out, const MESSAGE_LEVEL& level)
{
  return out<<MessageLevelConversion::EnumToString(level);
}

std::istream& operator>>(std::istream& in, MESSAGE_LEVEL& level)
{
  std::string temp;
  in>>temp;
  level = MessageLevelConversion::StringToEnum(temp);
  return in;
}

MESSAGE_LEVEL& operator++(MESSAGE_LEVEL& level)
{
  if( level == (N_MESSAGE_LEVELS-1) )
    return level;
  else 
    return level = (MESSAGE_LEVEL)( (int)level + 1 );
}

MESSAGE_LEVEL& operator--(MESSAGE_LEVEL& level)
{
  if( (int)level == 0 )
    return level;
  else
    return level = (MESSAGE_LEVEL)( (int)level - 1 );
}

typedef MessageHandler::PrintToStream PrintToStream;

//some helpers

						
MessageHandler MessageHandler::_instance;

void PrintToStream::operator()(const std::string& s, MESSAGE_LEVEL level,
			    time_t t)
{
  std::string color = "", header = "";
  if(level <= DEBUG){
    header = "DEBUG: ";
    //color = "\x1B[33m";
  }  
  else if(level == WARNING){
    //color = "\x1B[1m\x1B[33m";
    color = "\x1B[33m";
    header = "WARNING: ";
  }
  else if(level >= ERROR){
    color = "\x1B[31m";
    header = MessageLevelConversion::EnumToString(level)+": ";
  }
  
  
  const char* set_norm = "\x1B[0m";
  
  struct tm * timeinfo;
  char timestamp[12];
  timeinfo = localtime( &t );
  strftime(timestamp,12,"%X ",timeinfo);
  if(_use_color) 
    _stream<<color;
  _stream << timestamp << header << s;
  if(_use_color)
    _stream<<set_norm<<std::flush;
}
  
MessageHandler::MessageHandler() : _default_threshold(INFO)
{
  AddMessenger(INFO,PrintToStream());
#ifndef SINGLETHREAD
  _kill_thread=false;
  _inbox_mutex = new boost::mutex;
  _message_waiting = new boost::condition_variable;
  _delivery_thread = new boost::thread(boost::ref(*this));
#endif
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->RegisterParameter("verbosity", _default_threshold,
			    "Threshold below which messages are suppressed");
  config->AddCommandSwitch('v',"verbose","increment verbosity",
			   CommandSwitch::Decrement<MESSAGE_LEVEL>
			   (_default_threshold));
  config->AddCommandSwitch('q',"quiet","decrement verbosity",
			   CommandSwitch::Increment<MESSAGE_LEVEL>
			   (_default_threshold));
  config->AddCommandSwitch(' ',"verbosity","set verbosity to <level>",
			   CommandSwitch::DefaultRead<MESSAGE_LEVEL>
			   (_default_threshold), "level");
}

void MessageHandler::End()
{
#ifndef SINGLETHREAD
  _kill_thread = true;
  _message_waiting->notify_one();
  //_delivery_thread->interrupt();
  _delivery_thread->join();
#endif
}  

MessageHandler::~MessageHandler()
{
  End();
#ifndef SINGLETHREAD
  delete _delivery_thread;
  delete _inbox_mutex;
  delete _message_waiting;
#endif
  for(std::set<VMessenger*>::iterator it = _messengers.begin();
      it!=_messengers.end(); ++it)
    delete *it;
  _messengers.clear();
}

void MessageHandler::RemoveMessenger(void* messenger)
{
  VMessenger* m = (VMessenger*)messenger;
  if(_messengers.erase(m))
    delete m;
}

void MessageHandler::SetThreshold(MESSAGE_LEVEL thresh, void* messenger)
{
  if(messenger){
    VMessenger* m = (VMessenger*)messenger;
    if(_messengers.count(m))
      m->SetThreshold(thresh);
    else
      std::cerr<<"No messenger known at pointer "<<messenger<<std::endl;
  }
  else{
    for(std::set<VMessenger*>::iterator it = _messengers.begin();
	it != _messengers.end();
	++it){
      (*it)->SetThreshold(thresh);
    }
  }
}

void MessageHandler::Deliver(std::ostringstream* msg, MESSAGE_LEVEL level,
			     time_t t)
{
  for(std::set<VMessenger*>::iterator it = _messengers.begin();
      it != _messengers.end(); it++){
    if( *it != 0)
      (*it)->Deliver(msg, level, t);
  }
  delete msg;
}
  
#ifndef SINGLETHREAD

void MessageHandler::Post(std::ostringstream* msg, MESSAGE_LEVEL level)
{
  boost::unique_lock<boost::mutex> lock(*_inbox_mutex);
  _inbox.push( MsgData(msg,level));
  _message_waiting->notify_one();
}

void MessageHandler::operator()()
{
  //return;
  while(1){
    boost::unique_lock<boost::mutex> lock(*_inbox_mutex);
    while(_inbox.empty() && !_kill_thread ){
      _message_waiting->wait(lock);
      //mutex is unlocked while waiting
    }
    if( _kill_thread && _inbox.empty()) break;
    //mutex is locked again once the condition return
    MsgData next = _inbox.front();
    _inbox.pop();
    lock.unlock();
    Deliver(next.msg, next.level, next.t);
  }
}

#else 
void MessageHandler::Post(std::ostringstream* msg, MESSAGE_LEVEL level)
{
  Deliver(msg, level);
}
#endif
