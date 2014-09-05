#include "MongoDBInterface.hh"
#include <sstream>

typedef std::auto_ptr<mongo::DBClientCursor> Cursor;
typedef std::map<std::string, std::string> stringmap;

using mongo::BSONObj;
using mongo::BSONElement;
using mongo::BSONObjBuilder;
using mongo::Query;

static runinfo RuninfoFromBSONObj(BSONObj obj);
static BSONObj RuninfoToBSONObj(runinfo* info);

//register with factory:
static VDatabaseInterface::Factory<MongoDBInterface> __mongofactory("MongoDB");

MongoDBInterface::MongoDBInterface(const std::string& db,
				   const std::string& coll,
				   const std::string& hostname,
				   int hostport,
				   const std::string& username,
				   const std::string& pass) : 
  VDatabaseInterface("MongoDB")
{
  Configure(db, coll, hostname, hostport, username, pass);
  
  RegisterParameter("host", host,
		    "Hostname where mongod server is running");
  RegisterParameter("port",port,
		    "Port on which mongod is running on remote host");
  RegisterParameter("database",database,
		    "Name of database where runinfo is stored");
  RegisterParameter("collection",collection,
		    "Name of collection where runinfo is stored");
  RegisterParameter("user",user,
		    "Username for authentication; leave blank to skip");
  RegisterParameter("password",password,
		    "Password for authentication");
}

MongoDBInterface::~MongoDBInterface() {}

void MongoDBInterface::Configure(const std::string& db,
				 const std::string& coll,
				 const std::string& hostname,
				 int hostport,
				 const std::string& username,
				 const std::string& pass)
{
  database = db;
  collection = coll;
  host = hostname;
  port = hostport;
  user = username;
  password = pass;
}


int MongoDBInterface::Connect()
{
  if(_connected)
    return 0;
  std::string err = "";
  try{
    _dbconnection.connect(mongo::HostAndPort(host,port));
     if(user != "")
      _dbconnection.auth(database,user,password, err);
  
  }
  catch(const std::exception& e){
     Message(ERROR)<<"MongoDBInterface::Connect caught exception "
		  <<e.what()<<" while trying to connect.\n"
		  <<"Error message: "<<err<<"\n";
     return 1;
  }
   Message(DEBUG)<<"Successfully connected to mongo database "<<database
		  <<" at "<<host<<":"<<port<<"\n";
   _connected = true;
   return 0;
}

int MongoDBInterface::Disconnect() //no-op!
{
  //_connected = false;
  return 0;
}

runinfo MongoDBInterface::LoadRuninfo(long runid)
{
  std::stringstream ss;
  ss<<"{ runid: "<<runid<<" }";
  return LoadRuninfo( ss.str() );
}

runinfo MongoDBInterface::LoadRuninfo(const std::string& query)
{
  std::vector<runinfo> v;
  if(LoadRuninfo(v,query) >= 0 && v.size() > 0)
    return v[0];
  return runinfo(-1);
}

int MongoDBInterface::LoadRuninfo(std::vector<runinfo>& vec, 
				  const std::string& query)
{
  return LoadRuninfo(vec, Query(query));
}
  
runinfo MongoDBInterface::LoadRuninfo(const mongo::Query& query)
{
  std::vector<runinfo> v;
  if(LoadRuninfo(v,query) <= 0 && v.size() > 0)
    return v[0];
  return runinfo(-1);
}
  
int MongoDBInterface::LoadRuninfo(std::vector<runinfo>& vec, 
				  const Query& query)
{
  if(!_connected && Connect() != 0)
    return -1;
  vec.clear();
  Cursor cursor = _dbconnection.query(GetNS(),query);
  while(cursor->more()){
    BSONObj obj = cursor->next();
    runinfo info = RuninfoFromBSONObj(obj);
    vec.push_back(info );
  }
  return vec.size();
}

int MongoDBInterface::StoreRuninfo(runinfo* info, STOREMODE mode)
{
  if(!_connected && Connect() != 0)
    return -1;

  BSONObj obj = RuninfoToBSONObj(info);
  if(obj.isEmpty())
    return -2;
  try{
    switch(mode){
    case INSERT:
      _dbconnection.insert(GetNS(),obj);
      break;
    case UPDATE: //note: update and replace are the same here!
    case REPLACE:
      _dbconnection.update(GetNS(), QUERY("runid"<<(int)info->runid), obj, 
			   false);
      break;
    case UPSERT:
      _dbconnection.update(GetNS(), QUERY("runid"<<(int)info->runid), obj, 
			   true);
      break;
    default:
      Message(WARNING)<<"Unknwon db update mode supplied! Doing nothing.\n";
    }
  }
  catch(std::exception& e){
    Message(ERROR)<<"MongoDBInterface::StoreRuninfo caught exception "
		  <<e.what()<<std::endl;
    return -3;
  }
  return 0;
}

runinfo RuninfoFromBSONObj(BSONObj obj)
{
  runinfo info;
  try{
    info.runid = obj.getIntField("runid");
    if(obj.hasField("starttime"))
      info.starttime = obj.getField("starttime").Date() / 1000;
    if(obj.hasField("endtime"))
      info.endtime = obj.getField("endtime").Date() / 1000;
    if(obj.hasField("triggers"))
      info.triggers = obj.getIntField("triggers");
    if(obj.hasField("events"))
      info.events = obj.getIntField("events");
  
    if(obj.hasField("metadata")){
      BSONObj metadata = obj.getObjectField("metadata");
      for(BSONObj::iterator i = metadata.begin(); i.more(); ){
	BSONElement e = i.next();
	std::string s = e.toString(false);
	if(s[0]=='"' && s[s.length()-1] == '"')
	  s = s.substr(1,s.length()-2);
	info.metadata[e.fieldName()] = s;
      }
    }
  
    if(obj.hasField("channel_metadata")){
      BSONObj chans = obj.getObjectField("channel_metadata");
      for(BSONObj::iterator i = chans.begin(); i.more(); ){
	BSONElement e = i.next();
	std::string fname = e.fieldName();
	if(fname[0]=='"' && fname[fname.length()-1] == '"')
	  fname = fname.substr(1,fname.length()-2);
	int chan = atoi(fname.c_str());
	stringmap& cmeta = info.channel_metadata[chan];
	for(BSONObj::iterator j = e.Obj().begin(); j.more(); ){
	  BSONElement e2 = j.next();
	  std::string s = e2.toString(false);
	  if(s[0]=='"' && s[s.length()-1] == '"')
	    s = s.substr(1,s.length()-2);
	  cmeta[e2.fieldName()] = s;
	}
      }
    }
  }
  catch(std::exception& e){
    Message(ERROR)<<"Unable to load runinfo: "<<e.what()<<"\n";
  }

  return info;
}

BSONObj RuninfoToBSONObj(runinfo* info)
{
  BSONObjBuilder b;
  b.append("runid",(int)info->runid);
  b.appendTimeT("starttime", info->starttime);
  b.appendTimeT("endtime", info->endtime);
  b.append("triggers", (int)info->triggers);
  b.append("events",(int)info->events);
  
  BSONObjBuilder metadata;
  for(stringmap::iterator it = info->metadata.begin(); 
      it != info->metadata.end(); ++it){
    metadata.append( it->first, it->second );
  }
  b.append("metadata",metadata.obj());
  
  BSONObjBuilder channel_metadata;
  for(std::map<int,stringmap>::iterator cit = info->channel_metadata.begin();
      cit != info->channel_metadata.end(); ++cit){
    BSONObjBuilder cmetadata;
    for(stringmap::iterator it = (cit->second).begin(); 
	it != (cit->second).end(); ++it){
      cmetadata.append(it->first, it->second);
    }
    char num[10];
    sprintf(num,"%d",cit->first);
    channel_metadata.append(num, cmetadata.obj());
  }
  b.append("channel_metadata",channel_metadata.obj());
  
  return b.obj();

}
