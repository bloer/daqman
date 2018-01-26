#include "V172X_Event.hh"
#include "V172X_Params.hh"
#include "Message.hh"
#include <bitset>

V172X_BoardData::V172X_BoardData(const unsigned char* const raw_data) : 
  event_size( *((uint32_t*)raw_data) & 0x0FFFFFFF),//32-bit words
  channel_mask(  ((*((uint32_t*)(raw_data+11))&0xFF)>>16)
                 | (*((uint32_t*)(raw_data+4))&0xFF) ),
  pattern( *((uint16_t*)(raw_data+5)) ),
  zle_enabled( *(raw_data+7) & 1),
  board_id( (*((uint8_t*)(raw_data+7))) >>3 ),
  event_counter( *((uint32_t*)(raw_data+8)) & 0x00FFFFFF ),
  timestamp( *((uint32_t*)(raw_data+12)) & 0x7FFFFFFF ) 
{
  /*
  std::cerr<<std::hex;
  for(uint32_t word=0; word < 100; word++){
  std::cerr<<*((uint32_t*)(raw_data) + word)<<std::endl;
  }
  std::cerr<<std::dec;
  */
  std::fill_n(channel_start,nchans,(char*)0);
  std::fill_n(channel_end,nchans,(char*)0);
  
  std::bitset<V172X_BoardParams::MAXCHANS> enabled_chans(channel_mask);
  
  int bytes_per_chan = 0;
  if(enabled_chans.count()) 
    bytes_per_chan = (event_size*4 - 16) / enabled_chans.count();
  int offset = 16;
  for(int i=0; i<nchans; i++){
    if(!enabled_chans[i])
      continue;
    channel_start[i] = (char*)(raw_data + offset);
    
    if(!zle_enabled)
      offset += bytes_per_chan;
    else
      offset += 4 * *((uint32_t*)(channel_start[i]));
    
    channel_end[i] = (char*)(raw_data + offset);
    
  }
}
V172X_BoardData::~V172X_BoardData()
{}


/*V172X_Event::V172X_Event(RawEventPtr base_event, V172X_Params* params) : 
  _base(base_event), _params(params)
{
  if(_base){
    //For now, we'll assume no ZLE
    unsigned int offset = 0;
    while(offset < _base->GetDataSize() - _base->GetHeaderSize()){
      V172X_BoardData board(_base->GetRawBufferAfterHeader()+offset);
      offset += board.event_size*4;
      _boards.push_back(board);
    }   
  }
}
*/

V172X_Event::V172X_Event(const unsigned char* raw_buffer, 
			 const unsigned int size, V172X_Params* params) :
  _buffer(raw_buffer), _size(size), _params(params)
{
  if(_buffer){
    //for now, assume no ZLE
    unsigned int offset = 0;
    while(offset < size){
      V172X_BoardData board(_buffer+offset);
      offset +=board.event_size*4;
      _boards.push_back(board);
    }
  }
}

V172X_Event::~V172X_Event() 
{}

