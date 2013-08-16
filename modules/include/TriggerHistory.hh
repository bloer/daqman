#ifndef TRIGGERHISTORY_h
#define TRIGGERHISTORY_h

#include "BaseModule.hh"
#include "stdint.h"

class TCanvas;
class TGraphErrors;
class TMultiGraph;
class TLegend;
class RootGraphix; 

class TriggerHistory : public BaseModule{
public:
  TriggerHistory(const std::string& name=GetDefaultName());
  ~TriggerHistory();

  static std::string GetDefaultName(){ return "TriggerHistory"; }
  
  int Initialize();
  int Process(EventPtr evt);
  int Finalize();
  
private:
  RootGraphix* _graphix;
  TCanvas* _canvas;
  TGraphErrors* _triggergraph;
  TGraphErrors* _eventgraph;
  TMultiGraph* _multigraph;
  TLegend* _legend;
  uint32_t _last_update_time;
  uint32_t _last_triggerid;
  uint32_t _last_eventid;
  int _update_interval;
  int _max_points;
  bool _draw_errors_x;
  bool _draw_errors_y;
  bool _connect_points;
};


#endif
