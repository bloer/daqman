//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

/** @file phrase.hh
    @brief defines phrase utility class
    @author bloer
    @ingroup ConfigHandler
*/

#ifndef PHRASE_h
#define PHRASE_h

/** @class phrase
    @brief string with overloaded iostream operators to read whitespace and empty strings
    @ingroup ConfigHandler
*/
#include <string>
#include <iostream>

class phrase : public std::string{
public:
  phrase() : std::string() {}
  phrase(const char* s) : std::string(s) {}
  phrase(const std::string& s) : std::string(s) {}
  phrase& operator=(const char* s){ std::string::operator=(s); return *this; }
  phrase& operator=(const std::string& s)
  { std::string::operator=(s); return *this; }
};

/// Allow strings to be single words or double-quoted segments
inline std::istream& operator>>(std::istream& in, phrase& s){
  s.clear();
  char next=0;
  in >> next;
  if(next == '"'){
  while( in.get(next) && next != '"'){
      s.append(1,next);
    }
  }
  else{
    in.unget();
    in >> (std::string&)s;
  }
  
  return in;

}

/// Surround the string body with quotes
inline std::ostream& operator<<(std::ostream& o, const phrase& s){
  return o<<'"'<<(std::string&)(s)<<'"';
}
  

#endif
