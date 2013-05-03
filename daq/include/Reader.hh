/** @file Reader.hh
    @brief Defines the reader class which loads raw data from a saved file
    @author bloer
    @ingroup daqman
*/

#ifndef READER_h
#define READER_h

#include <zlib.h>
#include <string>
#include "RawEvent.hh"
#include "VParameterNode.hh"


/** @class Reader
    @brief reads raw events from a data file for processing
*/
class Reader{
public:
  /// Constructor takes a raw data filename
  Reader(const std::string& filename);
  /// Destructor
  ~Reader();
public:
  //access functions
  ///Check if we're ok to read
  bool IsOk(){ return _ok; } 
  ///Check if we've reached the end of the file
  bool eof(){ return _end_last_file || ( gzeof(_fin) && OpenNextFile() ); }
  //All event getters return null pointer if error
  /// Get the next event in the file
  RawEventPtr GetNextEvent(bool read_header = true);   
  /// Search for the event located at number <index> in the file
  RawEventPtr GetEventWithIndex(int index);
  /// Search for an event with internal id <id>
  RawEventPtr GetEventWithID(uint32_t id);
  /// Find the last event in the file
  RawEventPtr GetLastEvent();
  /// Return the index of the current event in the file
  int GetCurrentIndex(){ return _current_index; }
  /// Load the parameter <par> from the saved config file
  bool GetAssociatedParameter(VParameterNode* par, 
			      std::string key="");

  // headers for version control
public:
  static const uint32_t magic_number = 0xdec0ded1; 
  static const uint32_t latest_global_version = 1;
  static const uint32_t latest_event_version = 1;
  struct global_header{
    uint32_t magic_num_check;
    uint32_t global_header_size;
    uint32_t global_header_version;
    uint32_t event_header_size; 
    uint32_t event_header_version;
    uint32_t file_size;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t run_id;
    uint32_t file_index;
    uint32_t nevents;
    uint32_t event_id_min;
    uint32_t event_id_max;
    global_header() : magic_num_check(magic_number), 
		      global_header_size(sizeof(global_header)),
		      global_header_version(1),
		      event_header_size(sizeof(event_header)),
		      event_header_version(1) {}
    
  };
  struct event_header{
    uint32_t event_size; 
    uint32_t event_id;
    uint32_t timestamp;
    uint32_t nblocks;
    event_header() : event_size(sizeof(event_header)){}
    void reset(){ event_size=0; event_id=0; timestamp=0; nblocks=0; }
  };

  struct datablock_header{
    uint32_t total_blocksize_disk;
    uint32_t datasize;
    uint32_t type;
  };
  
  struct event_header_v0{
    uint32_t event_size;
    uint32_t event_id;
    uint32_t timestamp;
  };

private:
  const std::string _filename; ///< raw filename
  gzFile _fin; ///< gzip file that we are reading from 
  bool _ok; ///< status of the reader/file
  long _current_index;  ///< index of the current event
  long _current_id; ///< ID of current event
  global_header _ghead; ///< header for the current file
  event_header _ehead; ///< header for the current event
  RawEventPtr _current_event; ///< Pointer to the current event
  uint32_t _current_file_index; ///< index of current file
  std::string _current_file_name; ///< Name of current file in series
  static const uint32_t _unset_file_index = 0xFFFFFFFF;
  bool _end_last_file; ///< have we reached the end of the last file?
  
  /// See if the last read operation completed successfully
  bool ErrorCheck(int bytes_read, int bytes_requested); 
  
  /// Reset the file to the initial state
  int Reset();
  /// Read the next header
  int ReadNextHeader();
  /// Skip over this event to the next one
  z_off_t SkipNextEvent(bool skip_header = true);
  
  /// Open the next file in the series
  int OpenNextFile();
  /// Close the current file
  int CloseCurrentFile();
  
};

#endif
