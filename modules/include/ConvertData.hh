/** @file ConvertData.hh
    @brief defines the ConvertData module
    @author bloer, pablo, jingkexu
    @ingroup modules
*/

#ifndef CONVERTDATA_h
#define CONVERTDATA_h

#include "BaseModule.hh"
#include <map>

class V172X_Params;
/** @class ConvertData
    @brief Convert the raw data pointer to useable variables
    @ingroup modules
*/

class runinfo;
class ConvertData : public BaseModule
{
public:
  ConvertData();
  ~ConvertData();

  int Initialize();
  int Finalize();
  int Process(EventPtr event);
  
  static const std::string GetDefaultName(){ return "ConvertData";}
  
  void SetHeadersOnly(bool setval = true){ _headers_only = setval; }
  bool GetHeadersOnly() const { return _headers_only; }
  
  void SetChOffset(int chan, double offset){ _offsets[chan] = offset; }
  double GetChOffset(int chan){ return _offsets[chan]; }
  std::map<int,double>* GetChOffsetMap(){ return &_offsets;}

private:
  int DecodeV172XData(const unsigned char* rawdata, uint32_t datasize, 
		       EventDataPtr data);
  
  uint64_t start_time;           ///< start time of the run
  uint64_t previous_event_time;  ///< time at which the previous event occurred
  std::map<int,double> _offsets;   ///< software offset time in us
  V172X_Params* _v172X_params;     ///< saved info for a v172x event
  runinfo* _info;                  ///< database information for this run
  long _id_mismatches;             ///< Number of events with ID mismatch
  bool _headers_only;              ///< Only process data headers, not the bulk
  std::set<int> _invert_channels;  ///< Invert converted data for a channel?
public:

};

#endif
