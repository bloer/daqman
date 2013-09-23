#include "RootGraphix.hh"
#include "TSystem.h"
#include "TCanvas.h"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "TCanvasImp.h"
#include "TRootCanvas.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGTableLayout.h"
#include "TRootEmbeddedCanvas.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TColor.h"

#include "utilities.hh"

const UInt_t fKeepRunning = 0x100000;

void* RunRootGraphix(void* mutexptr)
{
  TMutex* mutex = (TMutex*)(mutexptr);
  while(mutex->TestBit(fKeepRunning)){
    gSystem->Sleep(100);
    TLockGuard lock(mutex);
    if(gSystem->ProcessEvents())
      break;
  }
  return 0;
}

RootGraphix::RootGraphix() : 
  BaseModule("RootGraphix","Draw graphical canvases using the ROOT GUI"), 
  _app("app",0,0), _mutex(), _thread(RunRootGraphix), _mainframe(0)
{
  _mutex.SetBit(fKeepRunning,true);
  LoadStyle();
  //_app.ProcessFile(".rootstart.C");
  RegisterParameter("single_window",_single_window = false,
		    "Use a single window to contain all canvases");
  RegisterParameter("window_w",_window_w = 700,
		    "Width of single window in pixels");
  RegisterParameter("window_h", _window_h = 700,
		    "Height of single window in pixels");
  
}

RootGraphix::~RootGraphix() 
{
  Finalize();
  //_mutex.CleanUp();
}

int RootGraphix::Initialize()
{
  CustomizeHistogramMenus();
  _thread.Run(&_mutex);
  return 0;
}

int RootGraphix::Finalize()
{
  if(_thread.GetState() == TThread::kRunningState){
    _mutex.SetBit(fKeepRunning,false);
    _thread.Join();
  }
  if(_mainframe){
    _canvases.clear();
    _mainframe->Cleanup();
    delete _mainframe;
    _mainframe = 0;
  }

  for(size_t i=0; i < _canvases.size(); i++)
    delete _canvases[i];
  _canvases.clear();
  
  return 0;
}

int RootGraphix::Process(EventPtr )
{
  for(size_t i=0; i < _canvases.size(); i++){
    Lock glock = AcquireLock();
    _canvases[i]->Update();
  }
  return 0;
}


RootGraphix::Lock RootGraphix::AcquireLock()
{
  return std::auto_ptr<TLockGuard>(new TLockGuard(&_mutex));
}

TCanvas* RootGraphix::GetCanvas(const char* title, bool preventclose, 
				bool hidemenu)
{
  
  int n = _canvases.size();
  char name[100];
  sprintf(name, "canvas%d",n);
  TCanvas* canvas;
  Lock glock = AcquireLock();
  if(!_single_window){
    UInt_t wx = gClient->GetDisplayWidth();
    UInt_t wy = gClient->GetDisplayHeight();
    int topx = wx/2 * (n % 2) + 10*(n/4);
    int topy = wy/2 * (int)(n%4 > 1) + 10*(n/4);
    
    canvas = new TCanvas(name, (title == 0 ? name : title),
			 topx, topy, (int)(wx*0.49), (int)(wy*0.49));
    TRootCanvas* imp = (TRootCanvas*)(canvas->GetCanvasImp());
    if(imp && preventclose)
      imp->DontCallClose();
    if(imp && hidemenu)
      imp->ShowMenuBar(false);
  }
  else{
    if(!_mainframe){
      _mainframe = new TGMainFrame;
      _mainframe->DontCallClose();
      _mainframe->SetName("RootGraphixMainFrame");
      _mainframe->SetWindowName("daqman analysis display");
    }
    int nframes = n+1;
    int nx=1, ny=1;
    while(nx*ny<nframes){
      if(nx > ny)
	++ny;
      else
	++nx;
    }
    TGTableLayout* ml = new TGTableLayout(_mainframe,ny,nx,true);
    _mainframe->SetLayoutManager(ml);
    const int hints = kLHintsExpandX | kLHintsExpandY | 
      kLHintsFillX | kLHintsFillY |
      kLHintsShrinkX | kLHintsShrinkY | 
      kLHintsCenterX | kLHintsCenterY;
    
    //rearrange all of the existing frames
    int row=1, col=1;
    TList* subframes = _mainframe->GetList();
    if(subframes){
      TGFrameElement* el=0;
      TIter next(subframes);
      while( (el=(TGFrameElement*)next()) ){
	if(el->fLayout)
	  delete el->fLayout;
	el->fLayout = new TGTableLayoutHints(col-1,col,row-1,row,hints);
	if(col < nx)
	   ++col;
	else{
	  col=1;
	  ++row;
	}
      }
    }
    
    TRootEmbeddedCanvas* ec = new TRootEmbeddedCanvas(name,_mainframe);
    _mainframe->AddFrame(ec, new TGTableLayoutHints(col-1,col,row-1,row,hints));
    _mainframe->MapSubwindows();
    _mainframe->MapWindow();
    _mainframe->Resize();
    _mainframe->Resize(_window_w,_window_h);

    canvas = ec->GetCanvas();
  }
  _canvases.push_back(canvas);
  return canvas;
}

void RootGraphix::LoadStyle()
{
  TStyle *mystyle=new TStyle("mystyle","mystyle");
  *mystyle = *(gROOT->GetStyle("Plain"));
  mystyle->SetName("mystyle");
  gROOT->SetStyle("mystyle");
  mystyle->SetCanvasColor(kWhite);

  mystyle->SetTitleFillColor(kWhite);

  mystyle->SetFuncWidth(2);
  mystyle->SetHistLineWidth(2);
  mystyle->SetLegendBorderSize(0);
  //mystyle->SetOptFit(1111);
  mystyle->SetStatBorderSize(0);
  mystyle->SetTitleBorderSize(0);
  mystyle->SetDrawBorder(0);
  mystyle->SetLabelSize(.04,"xyz");
  mystyle->SetTitleSize(.04,"xyz");
  mystyle->SetLabelFont(102,"xyz");
  mystyle->SetOptStat("");
  mystyle->SetStatFont(102);
  mystyle->SetTitleFont(102,"xyz");
  mystyle->SetTitleFont(102,"pad");
  mystyle->SetStatStyle(0);
  mystyle->SetStatX(1);
  mystyle->SetStatY(1);
  mystyle->SetStatW(.2);
  mystyle->SetStatH(.15);
  mystyle->SetTitleStyle(0);
  mystyle->SetTitleX(.2);
  mystyle->SetTitleW(.65);
  mystyle->SetTitleY(.98);
  mystyle->SetTitleH(.07);
  mystyle->SetStatColor(0);
  mystyle->SetStatBorderSize(0);
  mystyle->SetFillColor(10);
  mystyle->SetFillStyle(0);
  mystyle->SetTextFont(102);
  mystyle->SetCanvasBorderMode(0);
  mystyle->SetPadBorderMode(1);
  mystyle->SetFrameBorderMode(0);
  mystyle->SetDrawBorder(0);
  
  mystyle->SetPalette(1,0);
  const Int_t NRGBs = 5;
  const Int_t NCont = 255;
  
  Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
  Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
  Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
  Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  mystyle->SetNumberContours(NCont);


}
