#include "ParameterIOimpl.hh"
//#include "Message.hh"
#include <stdexcept>
#include "phrase.hh"

//allow hex base for unsigned ints
unsigned long ParameterIOimpl::ReadUnsignedInt(std::istream& in)
{
  std::string temp;
  in>>temp;
  if(temp[0] == '0' && (temp[1] == 'x' || temp[1] == 'X'))
    return std::strtoul(temp.c_str(),0,16);
  else
    return std::strtoul(temp.c_str(),0,10);
}



///Override std::strings to allow enclosing in "", but not require
std::istream& ParameterIOimpl::read(std::istream& in, std::string& s)
{
  phrase temp;
  in>>temp;
  s = temp;
  return in;
    
}
  
///Override std::string to always be quoted on output
std::ostream& ParameterIOimpl::write(std::ostream& out, const std::string& s, 
				     bool, int)
{
  phrase temp(s);
  return out<<temp;
}

//override bool to allow true and false
std::istream& ParameterIOimpl::read(std::istream& in, bool& b)
{
  std::string temp;
  in>>temp;
  if(temp == "1" || temp == "true" || temp == "TRUE")
    b = true;
  else if( temp == "0" || temp == "false" || temp == "FALSE" )
    b = false;
  else{
    //Message e(EXCEPTION);
    std::cerr<<"Expected boolean value, got "<<temp<<std::endl;
    throw std::invalid_argument("Expected boolean value");
  }
  return in;
}
