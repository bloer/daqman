//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

#include "ParameterList.hh"
#include "Parameter.hh"
#include <sstream>
#include "Message.hh"
#include <stdexcept>
#include <iomanip>

ParameterList::~ParameterList()
{

}

ParameterList::ParameterList(const ParameterList& right) :
  VParameterNode(right._default_key, right._helptext)
{
  _node_type = PARAMETER_LIST;
}

ParameterList& ParameterList::operator=(const ParameterList& right)
{
  _deleter.clear();
  _parameters.clear();
  return *this;
}

VParameterNode* const ParameterList::GetParameter(const std::string& key){
  ParMap::iterator it = _parameters.find(key);
  if(it != _parameters.end()) 
    return it->second;
  else 
    return 0;
}

std::istream& ParameterList::ReadFrom(std::istream& in, bool single)
{
  if(_parameters.empty())
    InitializeParameterList();
  char next = ' ';
  //peek at the next character
  while ( in.get(next) ){
    //if it's ')', or '}' exit
    if( next == ')' || next == '}' )
      return in;
    //if it's space, \t, \n, or comma, or opening parneth. ignore it and move on
    if( next == ' ' || next == ',' || next == '\t' || next == '\n' 
	|| next == '(' || next == '{' || next == '|' || next == ';')
      continue;
    //if it's #, it's a comment, so kill the line and move on
    if( next =='#' ){
      std::string dummy;
      std::getline(in,dummy);
      continue;
    }
    //if it's @, it marks the start of a function statement
    if( next == '@' ){
      std::string func;
      in>>func;
      if(func == "include"){
	std::string fname;
	in>>fname;
	Message(DEBUG)<<"including file "<<fname<<std::endl;
	ReadFromFile(fname.c_str());
	continue;
      }
      else if (func == "copy"){
	std::string par1, par2;;
	in>>par1>>par2;
	Message(DEBUG)<<"Copying all settings of "<<par1<<" to "<<par2<<"\n";
	ParMap::iterator it1 = _parameters.find(par1);
	ParMap::iterator it2 = _parameters.find(par2);
	if(it1 == _parameters.end() || it2 == _parameters.end()){
	  Message(EXCEPTION)<<"Unable to find one of the parameters "<<par1
			    <<" or "<<par2<<" to copy!\n";
	  throw std::invalid_argument(par1+","+par2);
	}
	std::stringstream temp;
	(it1->second)->WriteTo(temp);
	(it2->second)->ReadFrom(temp);
	continue;
      }
      else{
	Message(WARNING)<<"Ignoring unknown command: @"<<func<<std::endl;
	continue;
      }
    }
    //anything else should be the start of a key 
    //so put it back and read in the key
    in.unget();
    std::string bigkey;
    if( !(in>>bigkey) ){
      //somethings messed up.  Throw an exception and die
      throw std::invalid_argument("Unabled to read parameter list");
      return in;
    }
    
    //bigkey may actually be a list of keys like key1,key2,key3
    std::vector<std::string> keylist;
    size_t searchstart=0;
    size_t pos;
    while( (pos = bigkey.find(',',searchstart)) != std::string::npos){
      keylist.push_back(bigkey.substr(searchstart,(pos-searchstart)));
      searchstart = pos+1;
      if(searchstart >= bigkey.size()) break;
    }
    //should be one last key
    keylist.push_back(bigkey.substr(searchstart));
    
    //read all the keys listed
    std::streampos start = in.tellg();
    for(size_t ikey = 0; ikey<keylist.size(); ++ikey){
      in.seekg(start);
      std::string key = keylist[ikey];
      //see if the key has '.' in it
      bool sendsingle = false;
      std::string::size_type pos = key.find('.',0);
      if( pos != std::string::npos){
	//only allow if we're the last key!
	if(ikey != keylist.size()-1){
	  Message(EXCEPTION)<<"Configuration parameters with '.' are "
			    <<"only allowed last in a comma set!\n";
	  throw std::invalid_argument(key);
	}
	//tell the next node to return after the first value is read
	sendsingle=true;
	//put back the rest of the "key" for the child to read
	in.seekg( in.tellg() - std::streampos(key.length()-pos - 1));
	//now we only care about the part before the '.'
	key = key.substr(0,pos);
      }
      
      //look for wildcards at the end ('*')
      if( key.at(key.length()-1) == '*'){
	key.resize( key.size() - 1 );
	int wildcardkeysfound = 0;
	for(int i=0; i<100; ++i){ //100 is totally arbitrary
	  std::stringstream keystream;
	  keystream<<key<<i;
	  if(_parameters.find(keystream.str()) == _parameters.end())
	    break;
	  keylist.push_back(keystream.str());
	  ++wildcardkeysfound;
	}
	//make sure there was at least one
	if(wildcardkeysfound==0){
	  Message(EXCEPTION)<<"No keys matching wildcard '"<<key<<"*'!\n";
	  throw std::invalid_argument(key+"*");
	}
	continue;
      }
      
      //now look for the actual parameter and read it
      ParMap::iterator mapit;
      mapit = _parameters.find(key);
      if( mapit == _parameters.end() ){
	//the key wasn't listed. throw an exception and abort
	Message e(EXCEPTION);
	e<<"Key "<<key<<" is not a valid parameter for ParameterList '"
	 <<GetDefaultKey()<<"'"<<std::endl;
	throw std::invalid_argument(e.str());
	return in;
      }
      VParameterNode* child = (mapit->second);
      child->ReadFrom(in,sendsingle);
    }
    if(single) 
      return in;
  }
  return in;

}

std::ostream& ParameterList::WriteTo( std::ostream& out, bool showhelp,
				      int indent)
{
  if(_parameters.empty())
    InitializeParameterList();
  std::stringstream dummy;
  dummy<<'\n';
  for(int i=0; i < indent; i++)
    dummy<<"  ";
  const std::string newline = dummy.str();

  //print an opening parenthesis to mark the beginning
  out<<"( "<<newline;
  ParMap::iterator mapit;
  mapit = _parameters.begin();
  //Loop over all the parameters in the map and pass the stream to them
  while( !out.fail() && mapit != _parameters.end() ){
    int node_type = mapit->second->GetNodeType();
    if(showhelp) out<<newline<<"# "<<mapit->second->GetHelpText()<<newline;
    if(node_type != FUNCTION)
	out<<(mapit->first)<<" ";
    mapit->second->WriteTo(out, showhelp, indent+1);
    out<<newline;
    ++mapit;
  }
  out<<(showhelp ? newline : "")<<")" << (showhelp ? " #end list" : "");
  out.flush();
  return out;
}

int ParameterList::PrintHelp(const std::string& myname) const
{
  // this function returns 1 to exit, 0 to print itself, and -n to go up n
  VParameterNode::PrintHelp(myname);
  std::cout<<"List of sub-parameters:\n";
  int parnumber=0;
  ParMap::const_iterator mapit; 
  for( mapit = _parameters.begin() ; mapit != _parameters.end(); ++mapit){
    std::cout<<" "<<std::left<<std::setw(5)<<std::setfill(' ')<<++parnumber
	     <<(mapit->first)<<std::endl;
  }
  std::cout<<"\nEnter n) for more information about parameter n"
	   <<"\n     -n) to go up n levels"
	   <<"\n      0) to quit the help browser"<<std::endl;
	   
  int response=0;
  std::cin >> response;
  if(response == 0)
    return 1;
  else if(response > (int)_parameters.size() ){
    std::cerr<<"The number you entered was too large!\n";
    return PrintHelp(myname);
  }
  else if(response > 0){
    mapit = _parameters.begin();
    for(int i=0; i < response-1; i++) ++mapit;
    response = (mapit->second)->PrintHelp(mapit->first);
  }
  //now response could be the user response or return from a sub-node
  if(response == 0)
    return PrintHelp(myname);
  if(response < 0)
    response++;
  
  return response;
}

    
