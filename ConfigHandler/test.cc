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

  ConfigHandler* config = ConfigHandler::GetInstance();
  config->SetProgramUsageString("test [<options>]");
  config->SetProgramDescription("This program tests some ConfigHandler features");
  
  ParameterList pl1("myparams","A sample parameter list");
  double x1;
  int y;
  std::string z, z2;
  phrase p;
  pl1.RegisterParameter("x",x1=3.14159,"A double precision value");
  pl1.RegisterParameter("y", y=-2, "An integer");
  pl1.RegisterParameter("z",z="HelloWorld","A string");
  pl1.RegisterParameter("p1", p = "I am a longer string with spaces!",
			"A string that may contain spaces");
  pl1.RegisterParameter("empty", z2="", "An empty string");
  config->RegisterParameter("myparams",pl1, "Something else");

  try{
    config->ProcessCommandLine(argc,argv);
  }
  catch(...){}
  
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
