/** @file RawEvent.hh
    @brief Defines the RawEvent class which wrawp raw data and RawEventPtr
    @author bloer
    @ingroup daqman
*/

#ifndef RawEvent_INCLUDED
#define RawEvent_INCLUDED

#include <stdint.h>
#include <vector>
#include <memory>

/** @class RawEvent
    @brief Container for a block of raw data
*/
class RawEvent 
{
  
public:
  /// constructor, automatically increments event_id counter by default
  RawEvent(bool increment_id_counter = true);
  /// constructor, sets event_id, timestamp, and run_id explicitly
  RawEvent(uint32_t event_id, uint32_t timestamp, uint32_t run_id);
  ///Destructor 
  ~RawEvent();
  
  ///Access to data blocks
  /** @struct datablock
      @brief defines headers for blocks of data within an event
      Should only ever be used as a pointer
  */
  struct datablock{
    uint32_t datasize;
    uint32_t type;
    std::vector<unsigned char> data;
    datablock(uint32_t Type, uint32_t Size=0) : datasize(Size), type(Type) 
    {data.resize(Size); }
  };
  /** @enum datablock_type
      @brief lists pre-defined types of datablock
  */
public:
  enum datablock_type { CAEN_V172X=0 , MONTECARLO=1 };
  
  /// Get the total size of this event
  uint32_t GetDataSize() const { return _buffer_size; }
  
  /// Get the total number of data blocks in this event
  size_t GetNumDataBlocks() const { return _datablocks.size(); }
  /// Add new block at end of event, return index of that block (-1 if error)
  int AddDataBlock(uint32_t blocktype, uint32_t datasize=0);
  /// Remove the data block number n, return 0 if success
  int RemoveDataBlock(size_t block_n);
  
  /// Get a const pointer to data block object block_n
  const datablock* GetDataBlock(size_t block_n) const;
  /// Get a pointer to the block of raw data number block_n
  unsigned char* GetRawDataBlock(size_t block_n);
  
  /// Get the size of datablock n
  uint32_t GetDataBlockSize(size_t block_n) const;
  /// Set or reset the size of block n. returns 0 if success
  int SetDataBlockSize(size_t block_n, uint32_t newsize);
  
  /// Get the type of datablock n
  uint32_t GetDataBlockType(size_t block_n) const;
  /// Set or reset the type of block n, returns 0 on success
  int SetDataBlockType(size_t block_n, uint32_t newtype);

  
  /// Get the id of this event
  uint32_t GetID() const { return _event_id;}
  /// Set the id of this event
  void SetID(uint32_t evid) { _event_id = evid; }
  /// Get the timestamp of this event
  uint32_t GetTimestamp() const { return _timestamp; }
  /// Set the timestamp of this event
  void SetTimestamp(uint32_t stamp) { _timestamp = stamp; }
  /// Get the ID of the run this event belongs to
  uint32_t GetRunID() const { return _run_id; }
  /// Set the run ID this event belongs to
  void SetRunID(uint32_t runid) { _run_id = runid; }
  
  /// Get the total memory taken up by all events on the heap
  static const long GetTotalBufferSize(){ return _total_buffer_size;}
  /// Set the total number of events processed in this run
  static void SetEventCount(uint32_t count){ _event_count=count;}
  /// Get the total number of events processed in this run
  static uint32_t GetEventCount(){ return _event_count; }
  
 
private:
  static const uint32_t _extra_buffer_space = 12;
  std::vector<datablock> _datablocks;
  
  uint32_t _event_id;
  uint32_t _timestamp;
  uint32_t _run_id;
  
  uint32_t _buffer_size;
  
  static long _total_buffer_size;
  static uint32_t _event_count;  
  //copy constructors not allowed
  RawEvent(const RawEvent& right);
  RawEvent& operator=(const RawEvent& right);
};

/** @typedef RawEventPtr
    wrap a RawEvent* into a std shared_ptr
    
    @ingroup daqman
*/
typedef std::shared_ptr<RawEvent> RawEventPtr;
#endif
