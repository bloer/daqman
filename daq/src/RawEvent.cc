#include "RawEvent.hh"
#include "Message.hh"
#include <time.h>

//initialize all the statics
long RawEvent::_total_buffer_size = 0;
uint32_t RawEvent::_event_count = 0;

RawEvent::RawEvent(bool increment_id_counter) 
{
  _event_id = _event_count++;
  _timestamp = (uint32_t)(time(0));
  _run_id = -1;
  _buffer_size = 0;
}

RawEvent::RawEvent(uint32_t event_id, uint32_t timestamp, uint32_t run_id) : 
  _event_id(event_id), _timestamp(timestamp), _run_id(run_id)
{
  _buffer_size = 0;
}
  
RawEvent::~RawEvent()
{
  _total_buffer_size -= _buffer_size;
}

int RawEvent::AddDataBlock(uint32_t blocktype, uint32_t datasize)
{
  _datablocks.push_back(datablock(blocktype, datasize));
  _buffer_size += datasize;
  _total_buffer_size += datasize;
  return _datablocks.size() - 1;  
}

int RawEvent::RemoveDataBlock(size_t block_n)
{
  if(block_n >= _datablocks.size())
    return -1;
  uint32_t blocksize = _datablocks[block_n].data.size();
  _datablocks.erase(_datablocks.begin()+block_n);
  _buffer_size -= blocksize;
  _total_buffer_size -= blocksize;
  return 0;
}

const RawEvent::datablock* RawEvent::GetDataBlock(size_t block_n) const
{
  return &(_datablocks.at(block_n));
}

unsigned char* RawEvent::GetRawDataBlock(size_t block_n) 
{
  return &(_datablocks.at(block_n).data[0]);
}

uint32_t RawEvent::GetDataBlockSize(size_t block_n) const
{
  return _datablocks.at(block_n).datasize;
}

int RawEvent::SetDataBlockSize(size_t block_n, uint32_t newsize)
{
  if(block_n >= _datablocks.size()) 
    return -1;
  datablock& block = _datablocks[block_n];
  uint32_t bufsize = block.data.size();
  // expand the buffer if we need to, but don't bother shrinking
  if(newsize > bufsize){
    block.data.resize(newsize);
    _buffer_size += newsize-bufsize;
    _total_buffer_size += newsize-bufsize;
  }
  block.datasize = newsize;
  return 0;
}
  
uint32_t RawEvent::GetDataBlockType(size_t block_n) const
{
  return _datablocks.at(block_n).type;
}

int RawEvent::SetDataBlockType(size_t block_n, uint32_t newtype)
{
  if(block_n >= _datablocks.size())
    return -1;
  _datablocks[block_n].type = newtype;
  return 0;
}
  
