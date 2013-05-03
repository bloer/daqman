/** @file utilities.hh
    @brief Useful functions for loading trees/files/etc within root
    @author bloer
    @ingroup daqroot
*/

#ifndef UTILITIES_h
#define UTILITIES_h

//have the makefile add these functions to the root lib:
//ClassDef

class TFile;
class TTree;
class TGraph;
class TGraphErrors;
class TPad;
class TChain;
class TObject;

#include "TCanvas.h"
#include "TCut.h"
#include "ChannelData.hh"

#include <string>
#include <vector>

/// @addtogroup daqroot
/// @{

/// Open a file, and make sure it actually did open properly
TFile* OpenFile(const char* filename, const char* option = "READ");
/// Load the 'Events' tree from a given file
TTree* GetEventsTree(const char* filename);
/// Load the 'average_channel#' graph from a given file for channel n
TGraph* GetAverageGraph(const char* filename, int channel);
/// Load the 'average_channel#' graph from the official file for run run_no and channel n
TGraphErrors* GetAverageGraph(int run_no, int channel); 
/// Get the baseline of a TGraph by averagint the first npts points
double GetBaseline(TGraph* g, int npts=100, bool subtract = false);
/// Decide how to divde a pad based on the number of plots
int DividePad(TPad* p, int nplots);

/** @function SetAliases
    @brief Set some useful aliases for drawing spectra from the 'Events' tree.
    
    Uses the database interface to find the appropriate laser calibration run
    to scale the main region of interest for each channel. 
    Creates the following aliases:
    - npe# : number of photoelectrons for channel #
    - topspec : number of photoelectrons detected by all top pmts
    - botspec : number of photoelectrons detected by all bottom pmts
    - min : the minimum value of the waveform in regions[1]
    - max : the maximum value of the waveform in regions[1]
    - pmt : channel_id + 1, just to confuse things =)
*/
void SetAliases(TTree* Events);

/// Break a string at char delim into subtstrings in the out vector
int explode_string(const std::string& s, char delim, 
		   std::vector<std::string>& out);

/// Break a compound TCut into it's individual pieces
std::vector<std::string> explode_cut(const TCut& cut);

/// Draw TBoxes on a graph where we did mechanical operations on the detector
void DrawOperationsBoxes(bool drawbubble=true, bool drawrecirc=true);

/// Get the response for the given channel and event in file as a TGraph
TGraph* GetRealEvent(const char* filename, int eventnum, int channel);

/// Get the pointer to ChannelData the the given event and channel
ChannelData* GetChannelData(const char* filename, int eventnum, int channel);

/// Calculate the correlation coefficient between two vectors
double CorrelationCoefficient(int npts, double* x, double* y);

///Get a TCut with the standard analysis cuts
TCut GetStandardCuts();

///Get a TCut with standard cuts used for two-phase runs
TCut GetTwoPulseCuts(); 

///Get a TCut with standard cuts used for single-phase runs
TCut GetOnePulseCuts(bool onlyone=true);

///Plot relevant plots for 2-pulse events
TCanvas* TwoPulsePlots(TTree* Events, bool queryfit=false, TCut cuts="",TCanvas* c = new TCanvas);

///Plot relevant plots for 1-pulse events
TCanvas* OnePulsePlots(TTree* Events, TCut cuts="",TCanvas* c = new TCanvas);

///Plot the single photoelectron distribution from in-run events
TCanvas* PlotSpeDistributions(TTree* Events, bool normalize=true);

/// Get the electron lifetime for a run
double ElectronLifetime(TTree* Events, bool newcanvas=false, 
			bool defaultlimits=true);
/// Print a TH1 bin contents to a text file
void SaveHistoToFile(TObject* c);
/// Add menus to TH1{F,D}
void CustomizeHistogramMenus();

/// @}
#endif
