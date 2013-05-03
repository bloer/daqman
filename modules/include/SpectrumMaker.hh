#ifndef SPECTRUMMAKER_h
#define SPECTRUMMAKER_h

#include "BaseModule.hh"
#include "RootGraphix.hh"
#include "phrase.hh"

class TH1;
class TCanvas;
class TTree;
class TBranch;
class TTreeFormula;

class SpectrumMaker : public BaseModule{
public:
  SpectrumMaker(const std::string& name = GetDefaultName());
  ~SpectrumMaker();
  
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  
  static std::string GetDefaultName(){ return "SpectrumMaker"; }
private:

  TH1* _histo;            ///< Underlying histogram object
  TCanvas* _canvas;       ///< Canvas on which the histogram is drawn
  TTree* _tree;           ///< Dummy tree used to parse variables
  TBranch* _branch;
  TTreeFormula* _xform;
  TTreeFormula* _yform;
  TTreeFormula* _cutform;
  std::string _draw_cmd;   ///<actual draw command passed to tree

  phrase _cut;            ///< cut determines whether to draw
  phrase _xvar;           ///< What to plot on the x axis?
  int _nbinsx;            ///< Number of bins on the x axis 
  double _xmin;           ///< Minimum range of histogram
  double _xmax;           ///< Maximum range of histogram
  phrase _yvar;           ///< What to plot on the y axis?
  int _nbinsy;            ///< Number of bins on the y axis 
  double _ymin;           ///< Minimum range of histogram
  double _ymax;           ///< Maximum range of histogram
  
  phrase _title;          ///< Title of the histogram
  phrase _xtitle;         ///< Title of x axis
  phrase _ytitle;         ///< Title of y axis
  bool _logy;             ///< Should we use logararithmic y axis?  
  bool _logx;             ///< Should we use logarithmic x axis?
};
  
#endif
