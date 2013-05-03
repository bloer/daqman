/** @file AverageWaveforms.hh
    @brief defines the AverageWaveforms module
    @author bloer
    @ingroup modules
*/

#ifndef AVERAGE_WAVEFORMS_h
#define AVERAGE_WAVEFORMS_h

#include "BaseModule.hh"
#include <map>
#include <string>
//#include <iostream>
#include <fstream>
using namespace std;

class TGraphErrors;

/** @class AverageWaveforms
    @brief Averages the signals for each channel over an entire run (with some basic cuts)
    @ingroup modules
*/
class AverageWaveforms : public BaseModule{
public:
  AverageWaveforms();
  ~AverageWaveforms();

  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  static const std::string GetDefaultName(){ return "AverageWaveforms";}

  bool use_event_list;
  string event_list_location;

  bool with_s2;
  double min_pulse_height;
  double max_pulse_height;
  double min_fprompt;
  double max_fprompt;
  bool align_by_peak;
  double sum_start_time;
  double sum_end_time;
  double min_s1_start_time;
  double max_s1_start_time;
  int bin_size;
  double min_s2_start_time;
  double max_s2_start_time;
  int number_of_base_groups;

  int current_event;
  int current_run;
  ifstream txt;

private:
  void Cleanup();
  std::map<int,int> _num_event;
  std::map<int,TGraphErrors*> _plots;
};

#endif
