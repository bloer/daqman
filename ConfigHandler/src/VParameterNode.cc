//Copyright 2013 Ben Loer
//This file is part of the ConfigHandler library
//It is released under the terms of the GNU General Public License v3

#include "VParameterNode.hh"
#include "Message.hh"
#include <fstream>
#include <typeinfo>
bool VParameterNode::SaveToFile(const char *fname, bool showhelp)
{
  std::ofstream fout(fname);
  if(fout.fail()){
    Message(ERROR)<<"Unable to open file "<<fname<<" for writing."<<std::endl;
    return false;
  }
  return WriteTo(fout, showhelp);
}

bool VParameterNode::ReadFromFile(const char *fname, const std::string& key,
				  bool suppress_errs)
{
  if(key != "")
    Message(DEBUG)<<"Searching for '"<<key<<"' in file "<<fname<<std::endl;
  std::ifstream fin(fname);
  if(fin.fail()){
    if(!suppress_errs)
      Message(ERROR)<<"Unable to open file "<<fname<<" for reading."<<std::endl;
    return false;
  }
  bool fail = ReadFromByKey(fin, key,suppress_errs);
  fin.close();
  return fail;
}

std::istream& VParameterNode::ReadFromByKey(std::istream& in, 
					    const std::string& key,
					    bool suppress_errs)
{
  if(key == "")
    return ReadFrom(in);
  std::string next("empty");
  while( in >> next && next != key ) {}
  if (in.eof() && !suppress_errs){
    Message(ERROR)<<"Key "<<key<<" was not found.\n";
    return in;
  }
  else if(in.fail() && !suppress_errs){
    Message(ERROR)<<"Problem encountered reading '"<<key<<"' from stream.\n";
    return in;
  }
  
  return ReadFrom(in);
}

int VParameterNode::PrintHelp(const std::string& myname) const
{
  std::cout<<"--------------------------------------------------------\n";
  std::cout<<"Parameter name: "<<myname<<"\n"
	   <<"Default name:   "<<_default_key<<"\n";
  if(_helptext == "")
    std::cout<<"No description available for this parameter.\n";
  else
    std::cout<<"Description:    "<<_helptext<<std::endl;

  return 0;
}
