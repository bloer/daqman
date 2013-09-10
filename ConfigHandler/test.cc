#include "Message.hh"
#include "ParameterList.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include <stdio.h>
#include <stdlib.h>

using namespace std;
int main(int argc, char** argv)
{
  
  //ParameterList pl("mylist");
  string s;
  Parameter<string> sp(s,"s");
  cout<<"Enter a single word string: "<<endl;
  cin>>sp;
  cout<<"You entered "<<sp<<endl;
  cout<<"Enter a multiline string enclosed in quotes"<<endl;
  cin>>sp;
  cout<<"You entered "<<sp<<endl;
  
  
  return 0;
}
