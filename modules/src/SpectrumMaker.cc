#include "SpectrumMaker.hh"
#include "ProcessedPlotter.hh"
#include "SumChannels.hh"
#include "RootWriter.hh"
#include "EventHandler.hh"

#include "Integrator.hh"
#include "EvalRois.hh"

#include "TFile.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TTree.h"
#include "TBranch.h"
#include "TTreeFormula.h"
#include "TClass.h"
#include "TList.h"
#include "TClassMenuItem.h"
#include "TTreeFormulaManager.h"
#include "TTreePlayer.h"

SpectrumMaker::SpectrumMaker(const std::string& name) : 
  BaseModule(name, "Histogram variables from data") ,
  _histo(0), _canvas(0), _tree(0), _branch(0) , _graphix(0)
  //, _xform(0), _yform(0), _cutform(0)
{
  RegisterParameter("xvar", _xvar = "", "Variable to plot on x axis");
  RegisterParameter("nbinsx",_nbinsx = 200, "Number of bins in the x axis");
  RegisterParameter("xmin", _xmin = 0, "Minimum x range of the histogram");
  RegisterParameter("xmax", _xmax = 0, "Maximum x range of the histogram");
  
  RegisterParameter("yvar", _yvar = "", "Variable to plot on y axis");
  RegisterParameter("nbinsy",_nbinsy = 200, "Number of bins in the y axis");
  RegisterParameter("ymin", _ymin = 0, "Minimum y range of the histogram");
  RegisterParameter("ymax", _ymax = 0, "Maximum y range of the histogram");
  
  RegisterParameter("cut", _cut = "", "Cut to test before drawing");
  RegisterParameter("title", _title="", "Title for the histogram");
  RegisterParameter("xtitle", _xtitle="", "Title for the x axis");
  RegisterParameter("ytitle", _ytitle="", "Title of the yaxis");
  RegisterParameter("logx", _logx = false, "Plot on logarythmic x axis?");
  RegisterParameter("logy", _logy = false, "Plot on logarythmic y axis?");
  RegisterParameter("logz", _logz = false, "Plot on log z axis?");
  
}

SpectrumMaker::~SpectrumMaker()
{
  Finalize();
}


int SpectrumMaker::Initialize()
{
  if(_xvar == "" || _nbinsx == 0 || _xmin == _xmax || 
     (_yvar != "" && (_nbinsy==0 || _ymin == _ymax)) ){
    //can't do anything
    Message(ERROR)<<"Incorrect axis or variable specifications for "
		  <<GetName()<<"; disabling.\n";
    return 1;
  }
  
  _graphix = EventHandler::GetInstance()->GetModule<RootGraphix>();
  
  if(_yvar == ""){
    _histo = new TH1D(GetName().c_str(), _title.c_str(),
		      _nbinsx, _xmin, _xmax);
    _histo->SetFillColor(kGreen);
  }
  else
    _histo = new TH2D(GetName().c_str(), _title.c_str(),
		      _nbinsx, _xmin, _xmax,
		      _nbinsy, _ymin, _ymax);
  _histo->SetXTitle(_xtitle.c_str());
  _histo->SetYTitle(_ytitle.c_str());
  _histo->ResetBit(TH1::kCanRebin);
  
  //add Reset to the histogram's popup title
  if(std::string(_histo->Class()->GetMenuList()->First()->GetTitle()) != 
     "Reset"){
    TClass* cl = _histo->Class();
    cl->GetMenuList()
      ->AddFirst(new TClassMenuItem(TClassMenuItem::kPopupUserFunction,cl,
				    "Reset","Reset",0,"",-1,1));
  }
			         
  if(_graphix && _graphix->enabled){
    _canvas = _graphix->GetCanvas(GetName().c_str());
    _canvas->SetLogy(_logy);
    _canvas->SetLogx(_logx);
    _canvas->SetLogz(_logz);
    
    _canvas->cd();
    _histo->Draw( _yvar.empty() ? "" : "colz");
  }
  
  _tree = new TTree((GetName()+"tree").c_str(),"");
  
  
  EventData* ptr = new EventData;
  _branch = _tree->Branch(EventData::GetBranchName(),&ptr);
  _tree->SetDirectory(0);
  _tree->SetCircular(1);
  
  /*
  _xform = new TTreeFormula((GetName()+"xform").c_str(),_xvar.c_str(),_tree);
  //_xform->SetQuickLoad(true);
  if(!_yvar.empty()){
    _yform = new TTreeFormula((GetName()+"yform").c_str(),_yvar.c_str(),_tree);
    //_yform->SetQuickLoad(true);
  }
  
  if(!_cut.empty()){
    _cutform = new TTreeFormula((GetName()+"cutform").c_str(),_cut.c_str(),
				_tree);
    //_cutform->SetQuickLoad(true);
  }
  */
  _branch->ResetAddress();
  delete ptr;
       
  
  _draw_cmd = _yvar;
  if(!_yvar.empty())
    _draw_cmd += " : ";
  _draw_cmd += _xvar;
  
  _draw_cmd += " >>+";
  _draw_cmd += _histo->GetName();
  return 0;
}

int SpectrumMaker::Finalize()
{
  if( gFile && gFile->IsOpen() && _histo)
    _histo->Write();
  if(_histo) delete _histo;
  _histo = 0;
  // RootGraphix will delete the canvas
  _canvas = 0;
  delete _tree;
  _tree = 0;
  _branch = 0;
  /*
  delete _xform;
  _xform = 0;
  delete _yform;
  _yform = 0;
  delete _cutform;
  _cutform = 0;
  */
  return 0;
}

int SpectrumMaker::Process(EventPtr evt)
{
  RootGraphix::Lock glock = _graphix->AcquireLock();
  EventDataPtr data = evt->GetEventData();
  EventData* ptr = data.get();
  _branch->SetAddress( &ptr );
  _tree->Fill();
  _branch->ResetAddress();
  _tree->Draw(_draw_cmd.c_str(), _cut.c_str(),"goff",1,0);
  //std::cerr<<s->GetOption()<<" "<<s->GetAction()<<" "<<s->GetCleanElist()<<" "<<s->GetDimension()<<" "<<s->GetMultiplicity()<<" "<<s->GetNfill()<<" "<<s->GetSelectedRows()<<" "<<std::endl;
  /*
  for(int i=0; i<_xform->GetNdata(); ++i){
    if(_cutform){
      if(_cutform->GetNdata() <= i)
	break;
      if(_cutform->EvalInstance(i)==0)
	continue;
    }
    double xval = _xform->EvalInstance(i);
    if(_yform){
      if(_yform->GetNdata() <= i)
	break;
      double yval = _yform->EvalInstance(i);
      ((TH2D*)(_histo))->Fill(xval,yval);
    }
    else
      _histo->Fill(xval);
  }
  */
  
  if(_canvas)
    _canvas->Modified();
  _branch->SetAddress(0);
  return 0;
}
