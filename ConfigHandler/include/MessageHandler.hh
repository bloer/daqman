//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file MessageHandler.hh
    @brief defines the global MessageHandler stream redirector
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef MESSAGE_HANDLER_h
#define MESSAGE_HANDLER_h 1

#include <iostream>
#include <sstream>
#include <set>
#include <queue>
#include <time.h>

//forward declarations needed for boost threads
namespace boost{
  class thread;
  class mutex;
  class condition_variable;
};

/// @addtogroup ConfigHandler
/// @{

/** @enum MESSAGE_LEVEL
    @brief enumerate the different severity of messages
*/
enum MESSAGE_LEVEL {DEBUG3=0, DEBUG2, DEBUG, INFO, WARNING, ERROR,
		    CRITICAL, EXCEPTION, N_MESSAGE_LEVELS};
//MESSAGE_LEVEL needs streaming and increment/decrement operators
/// ostream overload for MESSAGE_LEVEL
std::ostream& operator<<(std::ostream& out, const MESSAGE_LEVEL& level);
/// istream overload for MESSAGE_LEVEL
std::istream& operator>>(std::istream& in, MESSAGE_LEVEL& level);
/// increment operator for MESSAGE_LEVEL
MESSAGE_LEVEL& operator++(MESSAGE_LEVEL& level);
/// decrement operator for MESSAGE_LEVEL
MESSAGE_LEVEL& operator--(MESSAGE_LEVEL& level);



class Message; //forward declaration

/** @class MessageHandler
    @brief Global class which accepts messages from different threads and 
    sends them to different receivers
*/
class MessageHandler{
public:
  /// Singleton access
  static MessageHandler* GetInstance(){ return &_instance; }
  /// Finish with messages, so you can print help dialogues to cout
  void End();
  /// This handles incoming Messages
  void Post(std::ostringstream* mgs, MESSAGE_LEVEL level);
  
  ///Allow the user to change message delivery; returns id of new Messenger
  template<class MsgAction> 
  void* AddMessenger(MESSAGE_LEVEL thresh,MsgAction act)
  { 
    VMessenger* m = new Messenger<MsgAction>(thresh,act);
    _messengers.insert(m); 
    return m; 
  } 
  /// Remove an already registered messenger
  void RemoveMessenger(void* m);
  /// Set the severity threshold for which messages to print
  void SetThreshold(MESSAGE_LEVEL thresh, void* messenger=0);
  /// Update the severity threshold if changed in config file
  void UpdateThreshold(){ SetThreshold(_default_threshold); }
  /// Get the default message threshold
  MESSAGE_LEVEL GetDefaultMessageThreshold(){return _default_threshold;}

  /** @class PrintToStream
      @brief A useful message function for cout, fstream
  */
  class PrintToStream{
    std::ostream& _stream;
    bool _use_color;
  public:
    PrintToStream(std::ostream& out=std::cout, bool use_color=true) : 
      _stream(out),  _use_color(use_color) {}
    void operator()(const std::string& s, MESSAGE_LEVEL level, time_t t);
  };
  
private:
  //private constructor and destructor as per singelton model
  MessageHandler();
  ~MessageHandler();
  
  MessageHandler(const MessageHandler&      ) {}
  MessageHandler& operator=(const MessageHandler& ) {return *this;}
  
  void Deliver(std::ostringstream* msg, MESSAGE_LEVEL level, time_t t=time(0));
  
  //private virtual messenger class; concrete instances hold pointers/objects
  //of delivery functions
  class VMessenger{
  public:
    VMessenger(MESSAGE_LEVEL thresh) : _thresh(thresh) {}
    virtual ~VMessenger() {}
    virtual void Deliver(std::ostringstream*, MESSAGE_LEVEL, time_t) = 0;
    void SetThreshold(MESSAGE_LEVEL thresh){ _thresh = thresh; }
  protected:
    MESSAGE_LEVEL _thresh;
  };
  
  template<class MsgAction> class Messenger : public VMessenger{
  public:
    Messenger(MESSAGE_LEVEL thresh, MsgAction action) : VMessenger(thresh), 
							_action(action) {}
    ~Messenger() {}
    void Deliver(std::ostringstream* msg, MESSAGE_LEVEL level, time_t t)
    { if (level >= _thresh ) _action(msg->str(),level,t);  }
  private:
    MsgAction _action;
  };
  
  std::set<VMessenger*> _messengers;
  static MessageHandler _instance;
  MESSAGE_LEVEL _default_threshold;

  //special members only needed for threading
private:
  struct MsgData{ 
    std::ostringstream* msg; 
    MESSAGE_LEVEL level;
    time_t t; 
    MsgData(std::ostringstream* m, MESSAGE_LEVEL l) : msg(m), level(l) 
    {time(&t);}
  };
  std::queue<MsgData> _inbox;
  bool _kill_thread;
  boost::mutex* _inbox_mutex;
  boost::condition_variable* _message_waiting;
  boost::thread* _delivery_thread;
public:
  void operator()(); ///< should only be called by a boost thread

};

/// @}

#endif
