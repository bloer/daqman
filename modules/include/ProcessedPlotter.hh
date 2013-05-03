/** @file ProcessedPlotter.hh
    @brief Defines ProcessedPlotter module
    @author bloer
    @ingroup modules
*/

#ifndef PROCESSEDPLOTTER_h
#define PROCESSEDPLOTTER_h 

#include "BaseModule.hh"
#include "RootGraphix.hh"

#include <stdint.h>
#include <time.h>
#include <vector>

//Forward declarations
class TCanvas;
class TMultiGraph;
class V1724EventMaker;
class TLegend;

/** @class ProcessedPlotter
    @brief Plots raw waveforms and some processed results for each channel
    @ingroup modules
*/
class ProcessedPlotter : public BaseModule{
public:
  ProcessedPlotter();
  ~ProcessedPlotter();
  
  int Initialize();
  int Finalize();
  int Process(EventPtr event);
  
  /// Skip processing new events until unpaused
  void Pause(){ _paused = true; }
  /// Resume processing events as they come in
  void Unpause() { _paused = false; }
  /// Toggle the paused state
  void TogglePause() { _paused = !_paused; }
  
  static const std::string GetDefaultName(){ return "ProcessedPlotter"; }
  
  /// Get the main drawing canvas
  const TCanvas* GetCanvas(int i=0) const;
  
  //plot parameters
  //bool multi_canvas;
  int chans_per_pad;   ///< How many channels on a single pad?
  bool overlay_analysis; ///< draw the analysis output on the raw pulses?
  bool multi_color;    ///< Use different colors?
  bool draw_legend;    ///< Include a legend?
  bool draw_title;     ///< Draw the title on the pulse plot?
  bool autoscalex;     ///< automatically scale the x axis?
  bool autoscaley;     ///< automatically scale the y axis?
  double xmin;         ///< Minimum value on the x axis to plot
  double xmax;         ///< Maximum value on the x axis to plot
  double ymin;         ///< Minimum value on the y axis to plot
  double ymax;         ///< Maximum value on the y axis to plot
  bool subtract_baseline; ///< Show the raw data or after baseline subtraction?
  int downsample;      ///< factor by which to downsample waveform
  bool drawpulses;     ///< Display the plot of raw pulses?
  bool drawpmtweights; ///< Display the plot of PMT weights?
  bool scale_pmts_sum; ///< Scale PMT weights to sum or max value?
  
private:
  enum CANVASES { PULSES=0, PMTWEIGHTS=1, NCANVASES};
  TCanvas* _canvas[NCANVASES];
  RootGraphix* _graphix;
  
  bool _paused;
  
};

#endif
