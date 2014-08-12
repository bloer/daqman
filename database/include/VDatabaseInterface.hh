/** @file VDatabaseInterface.hh
    @author bloer
    @brief Defines ABC for database interfaces to load and store runinfo
**/

#ifndef VDATABASEINTERFACE_hh
#define VDATABASEINTERFACE_hh

#include "Rtypes.h"
#include "runinfo.hh"
#include <string>
#include <vector>
#include <map>

/** @class VDatabaseInterface
    @brief Defines an abstract class for storing a loading runinfo from a database 
**/
//hide ParameterList from cint
#ifndef __CINT__
#include "ParameterList.hh"
class VDatabaseInterface : public ParameterList{
#else
class VDatabaseInterface{
#endif 

public:
  VDatabaseInterface(const std::string& defkeyname="DatabaseInterface")
#ifndef __CINT__
    : ParameterList(defkeyname,"Store and load runinfo from a database")
#endif
  {}
  virtual ~VDatabaseInterface() { if(_connected) Disconnect(); }
  
protected:
  bool _connected;

public:
  ///Open a connection to the database
  virtual int Connect()=0;
  ///Close any open database connection
  virtual int Disconnect() { return _connected = 0; }
  ///Check connection status
  virtual bool IsConnected() const { return _connected; }

  ///Find a single runinfo object by id, returns runid=-1 on error
  virtual runinfo LoadRuninfo(long runid)=0;
  ///Find a single runinfo object by query, returns runid=-1 on error
  virtual runinfo LoadRuninfo(const std::string& query)=0;
  ///Find several runinfo, return n found or negative on error
  virtual  int LoadRuninfo(std::vector<runinfo>& vec, 
			   const std::string& query)=0;
  
  ///Save the runinfo to the database
  enum STOREMODE { INSERT, UPDATE, REPLACE, UPSERT };
  virtual int StoreRuninfo(runinfo* info, STOREMODE mode=UPSERT)=0;
  
  ///Give an interface to configure from string for within daqroot shell
  void Configure(const std::string &s);
  
  ///Get a concrete database instance by name
  static VDatabaseInterface* GetConcreteInstance(const std::string& name);
  

#ifndef __CINT__
  //factory for creating concrete instances
public:
  
  class VFactory{
  public:
    VFactory(const std::string& name) : _name(name)
    { VDatabaseInterface::RegisterFactory(name,this); }
    ~VFactory(){ VDatabaseInterface::RemoveFactory(this); }
    virtual VDatabaseInterface* operator()()=0;
  private:
    std::string _name;
  };
  
  template<class ConcreteDB> class Factory : public VFactory{
  public:
    Factory(const std::string& name) : VFactory(name){}
    VDatabaseInterface* operator()()
    { return static_cast<VDatabaseInterface*>(new ConcreteDB); }
  };
  friend class VFactory;
private:
  ///Register the factory function to the generator
  static void RegisterFactory(const std::string& name, VFactory *f);
  ///Remove a registered factory
  static void RemoveFactory(VFactory* f);

private:
  static std::map<std::string, VFactory*>* _factories;

#endif

 
  ClassDef(VDatabaseInterface,0)
};
#endif
