#include "VDatabaseInterface.hh"
#include <iostream>
#include <sstream>


void VDatabaseInterface::Configure(const std::string& s)
{
  std::stringstream ss(s);
  ReadFrom(ss);
}


std::map<std::string,VDatabaseInterface::VFactory*>*
VDatabaseInterface::_factories = 0;

void VDatabaseInterface::RegisterFactory(const std::string& name, VFactory *f)
{
  if(!_factories) 
    _factories = new std::map<std::string,VDatabaseInterface::VFactory*>;
  if(_factories->count(name)){
    std::cerr<<"Error registering database interface factory with name "
	     <<name<<"; instance already exists!"<<std::endl;
  }
  else
    (*_factories)[name]=f;
}


void VDatabaseInterface::RemoveFactory(VFactory* f)
{
  if(!_factories) 
    _factories = new std::map<std::string,VDatabaseInterface::VFactory*>;
  std::map<std::string, VFactory*>::iterator it = _factories->begin();
  while(it != _factories->end()){
    if( it->second == f ) _factories->erase(it++);
    else ++it;
  }
}


VDatabaseInterface* 
VDatabaseInterface::GetConcreteInstance(const std::string& name)
{ 
  if(_factories && _factories->count(name) ) 
    return (*( (*_factories)[name]))();
  std::cerr<<"Error: Unknown database instance name: "<<name<<std::endl;
  return 0; 
}
