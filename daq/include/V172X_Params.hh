/** @file V172X_Params.hh
    @brief defines the V172X_Params, V172X_BoardParams, V172X_ChannelParams
    classes and some useful enums.
    @author bloer
    @ingroup daqman
*/

#ifndef V172X_PARAMS_h
#define V172X_PARAMS_h
#ifndef N_V172X_BOARDS
#define N_V172X_BOARDS 5
#endif

#include "ParameterList.hh"
#include <string>
#include <iostream>
#include <fstream>
#include "stdint.h"

/** @addtogroup daqman
    @{
*/

/** @enum SIGNAL_LOGIC
    @brief logic levels available for V172X digitizers, NIM or TTL
*/
enum SIGNAL_LOGIC { NIM=0, TTL=1 };
/** @enum ZERO_SUPPRESSION_TYPE
    @brief defines zero suppression modes available for V172X digitizers
*/
enum ZERO_SUPPRESSION_TYPE {NONE=0, ZS_INT = 1, ZLE = 2, ZS_AMP = 3};
/** @enum TRIGGER_POLARITY
    @brief defines trigger logic types
*/
enum TRIGGER_POLARITY {TP_RISING = 0, TP_FALLING = 1};
/** @enum BOARD_TYPE
    @brief defines the available models of V172X digitzer
*/
enum BOARD_TYPE { V1724 = 0, V1721 = 1, V1720 = 3 , V1751 = 5, V1730 = 11, 
		  OTHER = 256};

/** @enum TRGOUT_MODE
    @brief defines available settings for the TRGOUT front panel connector
*/
enum TRGOUT_MODE { TRIGGER, BUSY};

//need iostream operators for all enums
/// SIGNAL_LOGIC ostream overload
std::ostream& operator<<(std::ostream& out, const SIGNAL_LOGIC& logic);
/// SIGNAL_LOGIC istream overload
std::istream& operator>>(std::istream& in, SIGNAL_LOGIC &logic);

/// ZERO_SUPPRESSION_TYPE ostream overload
std::ostream& operator<<(std::ostream& out, const ZERO_SUPPRESSION_TYPE& zs);
/// ZERO_SUPPRESSION_TYPE istream overload
std::istream& operator>>(std::istream& in, ZERO_SUPPRESSION_TYPE &zs);

/// TRIGGER_POLARITY ostream overload
std::ostream& operator<<(std::ostream& out, const TRIGGER_POLARITY& pol);
/// TRIGGER_POLARITY istream overload
std::istream& operator>>(std::istream& in, TRIGGER_POLARITY &pol);

/// BOARD_TYPE ostream overload
std::ostream& operator<<(std::ostream& out, const BOARD_TYPE& type);
/// BOARD_TYPE istream overload
std::istream& operator>>(std::istream& in, BOARD_TYPE &type);

/// TRGOUT_MODE ostream overload
std::ostream& operator<<(std::ostream& out, const TRGOUT_MODE& m);
/// TRGOUT_MODE istream overload
std::istream& operator>>(std::istream& in, TRGOUT_MODE& m);

/** @class V172X_ChannelParams
    @brief parameter list for each channel on a V172X digitizer
*/
class V172X_ChannelParams : public ParameterList {
public:
  /// Default constructor
  V172X_ChannelParams();
  /// Destructor does nothing
  ~V172X_ChannelParams(){}
  //bool SetDefaults();
  bool enabled;                  ///< is this channel enabled?
  bool enable_trigger_source;    ///< can this channel trigger this board?
  bool enable_trigger_out;       ///< can this channel generate triggers?
  uint16_t threshold;            ///< trigger threshold in counts
  double thresh_time_us;         ///< time signal must be above/below threshold
  uint16_t dc_offset;            ///< dc offset applied to signal
  TRIGGER_POLARITY zs_polarity;  ///< polarity for zero suppression threshold 
  uint32_t zs_threshold;         ///< zero suppression level in counts
  double zs_thresh_time_us;      ///< time signal must be above zs threshold
  uint32_t zs_pre_samps;         ///< number of samples to save before zle block
  uint32_t zs_post_samps;        ///< number of samples to save after ZLE block
  std::string label;             ///< descriptive label for this channel
};

/** @class V172X_BoardParams
    @brief parameter list for each V172X digitizer in the crate
*/
class V172X_BoardParams : public ParameterList
{
public:
  /// Default constructor
  V172X_BoardParams();
  /// Destructor does nothing
  ~V172X_BoardParams(){}
  /// Update calcuated variables after changing parameters
  int UpdateBoardSpecificVariables();
  //bool SetDefaults();
  //Data members

  bool enabled;                        ///< is this board enabled?
  int id;                              ///< unique id number for this board
  uint32_t address;                    ///< this board's VME address
  int link;			       ///< the caenvmelib link number for this board or the vme bridge
  bool usb;                            ///< connect through usb instead of optical link? 
  int chainindex;                      ///< Order of the board on a daisy chain fiber
  BOARD_TYPE board_type;               ///< type of V172X digitizer
  double v_full_scale;                 ///< full scale range of input
  int stupid_size_factor;              ///< determines packing of samples in mem
  int sample_bits;                     ///< sampling resolution
  double max_sample_rate;              ///< samples per microsecond
  long mem_size;                       ///< mem size per channel, in bytes
  int bytes_per_sample;                ///< size of a single sample (calculated)
  uint32_t event_size_bytes;           ///< expected size of a single event
  uint64_t ns_per_clocktick;           ///< tick rate of timestamp clock
  bool enable_software_trigger;        ///< do we allow softwre triggers?
  bool enable_software_trigger_out;    ///< do soft triggers generate sig out?
  bool enable_external_trigger;        ///< do we allow external triggers?
  bool enable_external_trigger_out;    ///< do we repeat external trigs out?
  uint16_t local_trigger_coincidence;  ///< need n+1 channels to trigger locally
  double pre_trigger_time_us;          ///< pulse length to store before trigger
  double post_trigger_time_us;         ///< pulse length to store after trigger
  uint32_t downsample_factor;          ///< NOT USED; kept for compaitibility 
  int almostfull_reserve;              ///< assert busy if <= n buffers free
  TRIGGER_POLARITY trigger_polarity;   ///< trigger on rising or falling signals
  bool count_all_triggers;             ///< count triggers that overlap?
  ZERO_SUPPRESSION_TYPE zs_type;       ///< do any zero suppression?
  bool enable_trigger_overlap;         ///< generate partial triggers windows? 
  //  uint32_t interrupt_on_event;     ///< wait n events before interrupt  
  SIGNAL_LOGIC signal_logic;           ///< use NIM or TTL signals?
  TRGOUT_MODE trgout_mode;             ///< send trgout or busy on front panel?
  bool enable_test_pattern;            ///< generate a test pattern internally?
  uint32_t acq_control_val;            ///< determines startup mode
  static const int MAXCHANS = 8;       ///< max hardware channels per board?
  int nchans;                          ///< actual num channels on this unit
  static const int Nth_factor = 4;     ///< how many samples in the trigger time
  V172X_ChannelParams channel[MAXCHANS]; ///< parameters for each channel

  //utility functions
  /// Get the sample rate used, in samples per microsecond
  double GetSampleRate() const; 
  /// Get the number of samples before the trigger
  uint32_t GetPreNSamps() const;
  /// Get the number of samples after the trigger
  uint32_t GetPostNSamps() const;
  /// Get the total number of samples read
  uint32_t GetTotalNSamps() const;
  /// Get the sample number at which the trigger occurred
  int GetTriggerIndex() const;
  /// Get the total number of buffers
  uint32_t GetTotalNBuffers() const;
  /// number of triggers that can be stored in buffer is 2^n
  uint32_t GetBufferCode() const; 
  /// Get the value to write to the custom size register
  uint32_t GetCustomSizeSetting() const;
  /// Get the value to write to the post samples register
  uint32_t GetPostTriggerSetting() const;
  /// Get the total time elapsed before the timestamp counter resets
  uint64_t GetTimestampRange() const;
  
};

/** @class V172X_Params
    @brief parameters relevant to all boards in the run
*/
class V172X_Params : public ParameterList
{
public:
  //Access functions
  /// Default Constructor
  V172X_Params();
  /// Destructor does nothing
  ~V172X_Params(){}
  
  //public data members
  
  bool align64;                   ///< do we need to read an extra sample? 
  int trigger_timeout_ms;         ///< how long to wait for trigger interrupt
  long max_mem_size;              ///< total system memory we're allowed to use
  bool no_low_mem_warn;           ///< suppress warning about low memory? 
  bool send_start_pulse;          ///< synchronize start of run on all boards?
  bool auto_trigger;              ///< allow computer to generate triggers?
  //caluclated values
  int event_size_bytes;           ///< total size of events
  int enabled_boards;             ///< number of boards enabled in this run
  int enabled_channels;           ///< number of channels enabled in this run 
  int vme_bridge_link;		  ///< the caenvmelib link number for the VME bridge
  static const int nboards = N_V172X_BOARDS; ///< max number of boards allowed
  V172X_BoardParams board[nboards];          ///< parameters for each board
  //utility functions
  /// Get the expected total event size in bytes
  int GetEventSize(); 
  /// Get the number of boards enabled in this run
  int GetEnabledBoards();
  /// Get the number of channels enabled in this run
  int GetEnabledChannels();
  
};


/// @}
#endif
  
  
  
