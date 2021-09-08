/** @file AveragePSD.hh
    @brief module calulcates average PSDs
    @author bloer
    @ingroup modules
*/

#ifndef AVERAGEPSD_h
#define AVERAGEPSD_h

#include "BaseModule.hh"
#include <map>
class TH1D;
class THStack;
class ChannelModule;
class RootGraphix;
class TCanvas;

class AveragePSD : public BaseModule{ 
public:
  AveragePSD();
  ~AveragePSD();
  
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  int Process(ChannelData* chdata);

  void Reset();
  
  static const std::string GetDefaultName(){ return "AveragePSD"; }

  //parameters
  double _min_time;
  double _max_time; 
  bool _logy;
  bool _logx;
  
protected:
  TH1D* GetPlot(ChannelData* chdata);
  TH1D* AddPlot(ChannelData* chdata);

private:
  std::map<int, TH1D*> _plots;
  THStack* _plotstack;
  RootGraphix* _graphix;
  TCanvas* _canvas;
};

#endif
