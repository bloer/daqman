//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file Message.hh
    @brief Definition for Message class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef MESSAGE_h
#define MESSAGE_h 1

#include <iostream>
#include <sstream>
#include "MessageHandler.hh"

/** @class Message
    @brief Thread-safe streaming utility with settable threshold
    @ingroup ConfigHandler
*/
class Message {
public:
  /// Default constructor
  /// @parameter level the importance of this message
  Message(MESSAGE_LEVEL level=INFO) : _level(level),
				      _stream(new std::ostringstream) {}
  /// Destructor; sends the message to the stream handler
  ~Message() throw(){ MessageHandler::GetInstance()->Post(_stream,_level); }
  //Note it's up to the MessageHandler to delete the stream!
  
  
  //Streaming operators:
  //The last three are necessary to catch things like std::endl
  
  /// Redirect any stream output to the internal stringstream
  template<class T> std::ostream& operator<< (const T& t){ return *_stream<<t;}
  std::ostream& operator<< (std::ostream& ( *pf )(std::ostream&))
  { return *_stream<<pf; }
  std::ostream& operator<< (std::ios& ( *pf )(std::ios&)){return *_stream<<pf; }
  std::ostream& operator<< (std::ios_base& ( *pf )(std::ios_base&))
  { return *_stream<<pf; }
  
  ///Get the string used in the message
  std::string str(){ return _stream->str(); }
private:
  MESSAGE_LEVEL _level;         ///< The importance of this message
  std::ostringstream* _stream;  ///< Internal stream holder
    
  /// Copy operator private
  Message(const Message& right) :  _level(right._level), 
				   _stream(right._stream) {}
  /// Assignment operator private
  Message& operator=(const Message& right){
    if (this != &right){
      _level=right._level;
      _stream=right._stream;
    }
    return *this;
  }
};
  
#endif
