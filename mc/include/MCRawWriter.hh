/** @file MCRawWriter.hh
    @brief Defines the MCRawWriter module
    @author bloer
    @ingroup modules
*/

#ifndef MCRAWWRITER_h
#define MCRAWWRITER_h

#include <fstream>
#include <string>
#include "BaseModule.hh"
#include "Reader.hh"
#include "MCEvent.hh"

/** @class MCRawWriter
    @brief Stores the raw data buffer onto disk in gzip'ped format
    @ingroup modules
*/
class MCRawWriter : public BaseModule{
public:
  MCRawWriter();
  ~MCRawWriter();
  
  //module functions
  int Initialize();
  int Process(EventPtr event) {return 0;};
  int Process(MCEventPtr event);
  int Finalize();
  static const std::string GetDefaultName(){ return "MCRawWriter"; }
  //access functions
  /// Get the name of the output file
  const std::string& GetFilename(){ return _filename; }
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

  void SetRunID(int rr) { _mchead.run_id = rr; }

  //  RunDB::runinfo* GetRunInfo() {return &_mcinfo;}
  ParameterList* GetRunInfo() {return &_mcinfo;}
  Reader::global_header* GetGHeadInfo() {return &_mchead;}
private:
  ///Save config file to go with raw data
  void SaveConfigFile();
  int OpenNewFile();
  int CloseCurrentFile();
  
  std::string _filename;   
  std::string _directory;
  int _compression;
  bool _save_config;
  std::ofstream _fout;
  //gzFile _fout;
  bool _ok;
  long long _bytes_written;
  uint32_t _max_file_size;
  int _max_event_in_file;
  
  Reader::global_header _mchead;
  //  MC_RunInfo _mcinfo;
  ParameterList _mcinfo;

};

#endif
