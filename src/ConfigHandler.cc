//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "MessageHandler.hh"
#include <iostream>
#include <exception>
#include <stdexcept>

#include <sys/ioctl.h>
#include <stdio.h>

class ParamHelpPrinter{
  const VParameterNode* p;
public:
  ParamHelpPrinter(const VParameterNode* node) : p(node){}
  int operator()(const char*){ p->PrintHelp(); exit(0); }
};

int PrintProgramUsage(const char* dummy = "")
{
  ConfigHandler::GetInstance()->PrintSwitches(true);
  return 0;
}

ConfigHandler::ConfigHandler() : 
  ParameterList("ConfigHandler","Global container for all parameters"), 
  _program_usage(""), _notes(), _default_cfg_file(), _saved_cfg()
{
  AddCommandSwitch(' ',"cfg","Load global configuration data from <file>",
		   CommandSwitch::LoadConfigFile(this),
		   "file");
  AddCommandSwitch(' ',"saved-config",
		   "Load saved configuration from previous run from <file>",
		   CommandSwitch::DefaultRead<std::string>(_saved_cfg),
		   "file");
  AddCommandSwitch(' ',"show-parameters",
		   "Print information about available configuration parameters",
		   ParamHelpPrinter(this) );
  AddCommandSwitch('h',"help","Display this help page",
		   PrintProgramUsage) ;
  RegisterParameter("notes",_notes, "Generic notes about this run, etc");
  RegisterParameter("saved-config",_saved_cfg,
		    "Previously saved configuration file");
}

ConfigHandler::~ConfigHandler()
{
  //delete the switches
  for(SwitchSet::iterator it = _switches.begin(); it != _switches.end(); it++){
    delete (*it);
  }
}

int ConfigHandler::RemoveCommandSwitch(char shortname, 
				       const std::string& longname){
  
  for(SwitchSet::iterator it = _switches.begin(); it != _switches.end(); it++){
    if( (*it)->shortname == shortname && (*it)->longname == longname){
      delete (*it);
      _switches.erase(it);
      return 0;
    }
  }
  std::cerr<<"Unable to find switch "<<shortname<<","<<longname<<std::endl;
  return 1;
}

void ConfigHandler::PrintSwitches(bool quit)
{
  if(quit)
    MessageHandler::GetInstance()->End();
  std::cout<<"Usage: "<<_program_usage<<std::endl;
  size_t maxlong = 0;
  size_t maxpar = 0;
  //struct winsize w;
  //ioctl(0,TIOCGWINSZ, &w);
  //size_t termsize=w.ws_col;
  size_t termsize = 80;
    
  
  
  for(SwitchSet::iterator it = _switches.begin(); it != _switches.end(); it++){
    maxlong = (maxlong > (*it)->longname.size() ? 
		   maxlong : (*it)->longname.size() );
    maxpar = (maxpar > (*it)->parameter.size() ? 
		   maxpar : (*it)->parameter.size() );
  }
  std::cout<<"Available Options:"<<std::endl;
  for(SwitchSet::iterator it = _switches.begin(); it != _switches.end(); it++){
    VCommandSwitch* cmd = *it;
    std::cout<<" ";
    if( cmd->shortname != ' ')
      std::cout<<'-'<<cmd->shortname;
    else
      std::cout<<"  ";
    
    if( cmd->shortname != ' ' && cmd->longname != "")
      std::cout<<',';
    else 
      std::cout<<' ';
    if( cmd->longname != "" ) 
      std::cout<<"--"<<cmd->longname;
    else
      std::cout<<"  ";
    for(size_t i=0; i < maxlong - cmd->longname.size(); i++)
      std::cout<<' ';
    if( cmd->parameter != "" )
      std::cout<<" <"<<cmd->parameter<<'>';
    else 
      std::cout<<"   ";
    for(size_t i=0; i < maxpar - cmd->parameter.size(); i++)
      std::cout<<' ';
    std::cout<<"  ";
    //insert line-breaks into helptext
    int offset=1+2+1+2+maxlong+3+maxpar+2+2; //final 2 for indent
    std::string spaces;
    spaces.assign(offset,' ');
    size_t length=termsize-offset;
    size_t mypos = 0;
    while(cmd->helptext.size()-mypos > length ){
      //look for a space and replace it with a line-break
      std::string chunk = cmd->helptext.substr(mypos, length);
      size_t spacepos = chunk.rfind(' ');
      if(spacepos == std::string::npos)
	break;
      std::cout<<chunk.substr(0,spacepos)<<"\n"<<spaces;
      mypos += spacepos+1;
    }
    std::cout<<cmd->helptext.substr(mypos)<<std::endl;
  }
  if(quit)
    exit(0);
}

int ConfigHandler::ProcessCommandLine(int& argc, char** argv)
{
  int status = 0;
  try{
    //first, see if the --cfg switch was specified; if not, load the default
    bool cfgswitchfound = false;
    for(int arg = 1; arg<argc; arg++){
      if(std::string(argv[arg]) == "--cfg"){
	cfgswitchfound = true;
	break;
      }
    }
    if(!cfgswitchfound){
      if(_default_cfg_file != ""){
	Message(INFO)<<"No --cfg switch found; reading default file "
		     <<_default_cfg_file<<"...\n";
	ReadFromFile(_default_cfg_file.c_str());
      }
      else{
	Message(INFO)<<"No --cfg switch found and no default file specified; "
		     <<"using compiled defaults.\n";
      }
    }
    
    for(int arg = 1; arg<argc; arg++){
      if(status != 0){
	Message(ERROR)<<"Problem encountered while processing command line "
		      <<std::endl;
	return status;
      }
      if( argv[arg][0] != '-' ){
	//we're done with switches
	for( int i=arg; i<argc; i++){
	  _cmd_args.push_back(argv[i]);
	  argv[1 + i - arg] = argv[i];
	}
	//reset argc, argv to remove switches
	argc = 1 + argc - arg;
	break;
	//return _cmd_args.size();
      }
      
      if( argv[arg][1] == '-' ){
	//we are processing a long switch
	bool switchfound = false;
	std::string key(argv[arg]+2);
	for(SwitchSet::iterator it = _switches.begin(); it != _switches.end(); 
	    it++){
	  if( (*it)->longname != key)
	    continue;
	  
	  switchfound = true;
	  //check to see if we need a parameter
	  bool need_parameter = ((*it)->parameter != "");
	  bool next_is_parameter = ( arg != argc-1 && argv[arg+1][0] != '-');
	  if( !need_parameter ){
	    status = (*it)->Process(""); //needs dummy char*
	    break;
	  }
	  else if( next_is_parameter){
	    status = (*it)->Process(argv[++arg]);
	    break;
	  }
	  else{
	    //oops! we didn't get the parameter we need!
	    Message e(EXCEPTION);
	    e <<"Switch --" << key
	      <<" is missing required parameter "
	      << (*it)->parameter<<std::endl;
	    throw std::invalid_argument(e.str());
	  }
	}
	if(!switchfound){
	  Message e(EXCEPTION);
	  e<<argv[arg]<<" is not a valid switch!\n";
	  throw std::invalid_argument(e.str());
	}
      }
      else{
	//we are dealing with short switches
	std::string shortkeys(argv[arg]+1);
	for(size_t i=0; i<shortkeys.size(); i++){
	  bool switchfound = false;
	  char key = shortkeys.at(i);
	  for(SwitchSet::iterator it = _switches.begin(); 
	      it != _switches.end(); it++){
	    if( (*it)->shortname != key)
	      continue;
	    switchfound = true;
	    //check to see if we need a parameter
	    bool need_parameter = ((*it)->parameter != "");
	    bool next_is_parameter = ( i == shortkeys.size() - 1 &&
				       arg != argc-1 && 
				       argv[arg+1][0] != '-' );
	    if( !need_parameter ){
	      status = (*it)->Process(""); //needs dummy char*
	      break;
	    }
	    else if( next_is_parameter){
	      status = (*it)->Process(argv[++arg]);
	      break;
	    }
	    else{
	      //oops! we didn't get the parameter we need!
	      Message e(EXCEPTION);
	      e <<"Switch -" << key
		<<" is missing required parameter "
		<< (*it)->parameter<<std::endl;
	      throw std::invalid_argument(e.str());
	    }
	  }
	  if(!switchfound){
	    Message e(EXCEPTION);
	    e <<"-"<<key<<" is not a valid switch!\n";
	    throw std::invalid_argument(e.str());
	  }
	}
      }
    }
  } //end try block
  catch(std::exception& e){
    Message(EXCEPTION)<<"An error occurred processing command line arguments:\n"
		      <<e.what()<<std::endl;
    PrintSwitches(true);
  }
  if(status == 0 ) 
    argc = _cmd_args.size() + 1;
  else if(status < 0) return status;
  
  MessageHandler::GetInstance()->UpdateThreshold();
  return _cmd_args.size();
}

bool ConfigHandler::OrderCommandSwitchPointers::operator()
  (VCommandSwitch* a, VCommandSwitch* b )
{
  //true is a < b; i.e. a comes before b
  //if shortname is blank, use the first letter of longname
  char shorta = (a->shortname == ' ' ? a->longname.at(0) : a->shortname);
  char shortb = (b->shortname == ' ' ? b->longname.at(0) : b->shortname);
  
  //first order by shortname
  if( shorta < shortb )
    return true;
  else if( shorta > shortb )
    return false;
  else {  
    //shortnames must be equal
    if( a->longname < b->longname)
      return true;
    else 
      return false;
  }
  //we shouldn't ever get here
  return false;
}
