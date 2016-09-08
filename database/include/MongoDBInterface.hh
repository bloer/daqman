#ifndef MONGODBINTERFACE_h
#define MONGODBINTERFACE_h

#include "VDatabaseInterface.hh"

#ifndef __CINT__
#include "mongo/client/dbclient.h"
#endif 

/**@class MongoDBInterface
   @brief Interface to save/load runinfo from a mongodb nosql database
*/
class MongoDBInterface : public VDatabaseInterface{
public:
  MongoDBInterface(const std::string& db="daqman", 
		   const std::string& coll="runinfo",
		   const std::string& hostname="localhost",
		   int hostport = 27017,
		   const std::string& username="",
		   const std::string& pass="");
  ~MongoDBInterface();
  
  void Configure(const std::string& db="daqman", 
		 const std::string& coll="runinfo",
		 const std::string& hostname="localhost",
		 int hostport = 27017,
		 const std::string& username="",
		 const std::string& pass="");
  
public:

  int Connect();
  int Disconnect();
  runinfo LoadRuninfo(long runid);
  runinfo LoadRuninfo(const std::string& query);
  int LoadRuninfo(std::vector<runinfo>& vec, const std::string& query);
  int StoreRuninfo(runinfo* info, STOREMODE mode=UPSERT);
  
#ifndef __CINT__
  ///Use the MongoDB native Query object for a single query
  runinfo LoadRuninfo(const mongo::Query& query);
  
  ///Use the MongoDB native Query object for the multi query
  int LoadRuninfo(std::vector<runinfo>& vec, const mongo::Query& query);
#endif 

  ///Get the full database.collection namespace for operations
  std::string GetNS() const { return database+"."+collection; }

public:
  std::string host;
  int port;
  std::string database;
  std::string collection;
  std::string user;
  std::string password;
private:
#ifndef __CINT__
  mongo::DBClientConnection _dbconnection;
#endif

private:
  MongoDBInterface& operator=(const MongoDBInterface& right)
  { return *this; }
  MongoDBInterface(const MongoDBInterface& right) {}
  
};

#endif //MONGODBINTERFACE_h
