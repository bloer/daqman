/** @file V172X_Event.hh
    @brief defines the V172X_Event and V172X_BoardData classes
    @author bloer
    @ingroup daqman
*/

#ifndef V172X_EVENT_h
#define V172X_EVENT_h

#include <vector>
#include <stdint.h>
#include <memory>
#include "V172X_Params.hh"

//forward declaration
class V172X_Params;

/** @class V172X_BoardData
    @brief contains data retreived from each board for a single trigger
    @ingroup daqman
*/
class V172X_BoardData{
public:
  ///< constructor, must take pointer to raw data location
  V172X_BoardData(const unsigned char* raw_data);
  ///< destructor
  ~V172X_BoardData();
  uint32_t event_size;      ///< size of this board's event (in 32-bit words)
  uint32_t channel_mask;     ///< mask of enabled channels
  uint16_t pattern;         ///< pattern stored by LVDS I/O channels 
  bool zle_enabled;         ///< is zero length encoding enabled?
  uint8_t board_id;         ///< id number of this board
  uint32_t event_counter;   ///< counter of triggers received so far 
  uint32_t timestamp;       ///< trigger time (interp. depends on board type)
  static const int nchans = V172X_BoardParams::MAXCHANS; ///< total number of channels per board
  char* channel_start[nchans]; ///< start of each channel's data
  char* channel_end[nchans];   ////< end of each channel's data
};


/** @class V172X_Event
    @brief contains data for the event and vector of board info for a trigger
    @ingroup daqman
*/
class V172X_Event{
public:
  /// Constructor. Takes pointer to the raw data location and total data size
  V172X_Event(const unsigned char* raw_data, const unsigned int size,
	      V172X_Params* params = 0);
  ///Destructor 
  ~V172X_Event();
  /// Get the number of boards active in this event
  int GetNBoards(){ return _boards.size(); }
  /// Get the data pertaining to board with index (not necessarily id!) <n>
  const V172X_BoardData& GetBoard(int n){ return _boards.at(n); }
  //uint32_t GetID(){ return _base->GetID(); }
  //uint32_t GetTimestamp(){ return _base->GetTimestamp(); }  
  
  /// Get the parameters defined for this run
  const V172X_Params* GetParameters() const { return _params; }
  /// Set <params> as the parameter list for this run
  void SetParameters(V172X_Params* params){ _params=params; }
private:
  const unsigned char* _buffer;           ///< location of the raw data in mem
  const unsigned int _size;               ///< raw data size
  V172X_Params* _params;                  ///< parameter list for this run
  std::vector<V172X_BoardData> _boards;   ///< vector of board data
};

/** @typedef V172XEventPtr
    @brief wrap a V172X_Event* into a boos shared_ptr
    @ingroup daqman
*/
typedef std::shared_ptr<V172X_Event> V172XEventPtr;
#endif
