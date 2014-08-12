#ifndef DATABASECONFIGURATOR_h
#define DATABASECONFIGURATOR_h

#include "VDatabaseInterface.hh"
#include "phrase.hh"
#include <iostream>

/**@class DatabaseConfigurator
   @brief Utility class to configure a plugin database connetion at runtime
*/

class DatabaseConfigurator{
  phrase _dbname;
  VDatabaseInterface* _db;
public:
  DatabaseConfigurator() : _dbname("NONE"), _db(0) {}
  ~DatabaseConfigurator(){ delete _db; }
  VDatabaseInterface* GetDB() const { return _db; }
  
  std::ostream& WriteTo(std::ostream& out) const;
  std::istream& ReadFrom(std::istream& in);
};

inline std::ostream& operator<<(std::ostream& out,const DatabaseConfigurator& d)
{ return d.WriteTo(out); }
inline std::istream& operator>>(std::istream& in, DatabaseConfigurator& d)
{ return d.ReadFrom(in); }

#endif
