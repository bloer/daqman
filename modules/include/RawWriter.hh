/** @file RawWriter.hh
    @brief Defines the RawWriter module
    @author bloer
    @ingroup modules
*/

#ifndef RAWWRITER_h
#define RAWWRITER_h

#include <fstream>
#include <string>
#include "BaseModule.hh"
#include "Reader.hh"

/** @class RawWriter
    @brief Stores the raw data buffer onto disk in gzip'ped format
    @ingroup modules
*/
class RawWriter : public BaseModule{
public:
  RawWriter();
  ~RawWriter();
  
  //module functions
  int Initialize();
  int Process(EventPtr event);
  int Finalize();
  static const std::string GetDefaultName(){ return "RawWriter"; }
  //access functions
  /// Get the name of the output file
  const std::string& GetFilename(){ 
    if(_filename=="") _filename = GetDefaultFilename();
    return _filename; 
  }
  /// Get the level of gzip compression being used
  int GetCompressionLevel(){ return _compression; }
  /// Check the status of the output file 
  bool IsOK(){ return _ok; }
  /// Get the total number of uncompressed bytes written so far
  long long GetBytesWritten(){ return _bytes_written; }
  /// Get the default filename
  std::string GetDefaultFilename() const;
  std::string GetFilename() const { return _filename; }
  void SetFilename(const std::string& name){ _filename = name; }
  
  void SetSaveConfig(bool save) { _save_config = save; }
  bool GetSaveConfig(){ return _save_config; }
private:
  ///Save config file to go with raw data
  void SaveConfigFile();
  int OpenNewFile();
  int CloseCurrentFile();
  
  std::string _filename;
  std::string _directory;
  bool _create_directory;
  std::string _autonamebase;
  int _compression;
  bool _save_config;
  std::ofstream _fout;
  std::ofstream _logout;
  void* _log_messenger;
  //gzFile _fout;
  bool _ok;
  long long _bytes_written;
  uint32_t _max_file_size;
  int _max_event_in_file;
  
  Reader::global_header _ghead;
};

#endif
