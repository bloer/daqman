#include "DatabaseConfigurator.hh"
#include "Message.hh"

std::ostream& DatabaseConfigurator::WriteTo(std::ostream& out) const
{
  out<<_dbname<<" ";
  if(_db)
    out<<*_db;
  return out;
}

std::istream& DatabaseConfigurator::ReadFrom(std::istream& in)
{
  if(_db){
    Message(DEBUG)<<"Removing already configured database "<<_dbname<<"\n";
    delete _db;
    _db = 0;
    _dbname = "";
  }
  in>>_dbname;
  if(_dbname != "" && _dbname != "none" && _dbname != "NONE")
    _db = VDatabaseInterface::GetConcreteInstance(_dbname);
  if(_db)
    in>>*_db;
  else{
    _dbname = "NONE";
    //in.clear(std::istream::failbit);
  }
  return in;
}
