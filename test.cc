#include "Message.hh"
#include "ParameterList.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include <stdio.h>
#include <stdlib.h>
int PrintHelp(const char*)
{
  std::cerr<<"Usage: test [options] [<arg1>, <arg2> ... ]"<<std::endl;
  ConfigHandler::GetInstance()->PrintSwitches();
  exit(0);
}

int main(int argc, char** argv)
{
  MESSAGE_LEVEL verbosity = INFO;
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->AddCommandSwitch('v',"","increment verbosity",
			   CommandSwitch::Decrement<MESSAGE_LEVEL>(verbosity));
  config->AddCommandSwitch('q',"","decrement verbosity",
			   CommandSwitch::Increment<MESSAGE_LEVEL>(verbosity));
  config->AddCommandSwitch(' ',"verbosity","set verbosity to <verbosity>",
			   CommandSwitch::DefaultRead<MESSAGE_LEVEL>(verbosity),
			   "verbosity");
  config->AddCommandSwitch('h',"help","Display this help page",PrintHelp);
  
  try{
    config->ProcessCommandLine(argc,argv);
  }
  catch(...){}
  MessageHandler::GetInstance()->SetThreshold(verbosity);
  Message(DEBUG)<<"I will not print by default"<<std::endl;
  Message(INFO)<<"I am plain"<<std::endl;
  Message(WARNING)<<"I am yellow!"<<std::endl;
  Message(ERROR)<<"I am red!"<<std::endl;
  Message(CRITICAL)<<"This is bad!"<<std::endl;
  Message(EXCEPTION)<<"This is worse!"<<std::endl;
  
  bool enable = true;
  unsigned int address = 0xEEEE0000;
  double time = 4.103;
  ParameterList l;
  l.RegisterParameter("address",address);
  l.RegisterParameter("time",time);
  l.RegisterParameter("enable",enable);
  Message(INFO)<<"The list is: "<<l;
  Message m(INFO);
  m<<"The command line is ";
  for(int i=0; i<argc; i++){
    m<<argv[i]<<" ";
  }
  m<<std::endl;
  Message(INFO)<<"Enter new values for address, enable, or time;"
	       <<" ')' or ctrl-d to end."<<std::endl;
  try{
    std::cin>>l;
  }
  catch(...){
    Message(WARNING)<<"Exception occurred!"<<std::endl;
  }
  Message(INFO)<<"l is now "<<l;
  
  ParameterList a;
  ParameterList b;
  int a1=1, a2 = 2, a3 = 3;
  int b1 = 2, b2 = 4, b3 = 6;
  a.RegisterParameter("a1",a1);
  a.RegisterParameter("a2",a2);
  a.RegisterParameter("a3",a3);
  b.RegisterParameter("b1",b1);
  b.RegisterParameter("b2",b2);
  b.RegisterParameter("b3",b3);
  ConfigHandler * cfg = ConfigHandler::GetInstance();
  cfg->RegisterParameter("time",time);
  cfg->RegisterParameter("a",a);
  cfg->RegisterParameter("",b);
  double x = 2.120;
  cfg->RegisterParameter("x",x);
  
  std::cout<<*cfg<<std::endl;
  
  return 0;
}
