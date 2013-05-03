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
    @brief string with overloaded iostream operators to read whitespace
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
/// Read from the stream until we reach the end quotes (")
inline std::istream& operator>>(std::istream& i, phrase& s){
  s.clear();
  char next;
  int nquotes = 0;
  while( (i.get(next) ) && nquotes < 2 ){
    if( next == '"') nquotes++;
    else if(nquotes) s.append(1,next);
  }
  return i;
}

/// Surround the string body with quotes
inline std::ostream& operator<<(std::ostream& o, phrase& s){
  return o<<'"'<<(std::string&)(s)<<'"';
}
  

#endif
