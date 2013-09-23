/** @file RootGraphix.hh
    @brief Defines the RootGraphix utility module
    @author bloer
    @ingroup modules
*/

#ifndef ROOTGRAPHIX_h
#define ROOTGRAPHIX_h

#include "BaseModule.hh"
#include "TMutex.h"
#include "TVirtualMutex.h"
#include "TThread.h"
#include "TApplication.h"
#include <memory>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
class TCanvas;
class TGMainFrame;

/** @class RootGraphix 
    @brief Displays ROOT canvases, etc, outside the ROOT interactive environment
    @ingroup modules
*/
class RootGraphix : public BaseModule{
public:
  RootGraphix();
  ~RootGraphix();
  int Process(EventPtr);
  int Initialize();
  int Finalize();
  //void operator()(){}
  static const std::string GetDefaultName(){ return "RootGraphix"; }
  
  /** @typedef Lock
      @brief wrap a TLockGuard into a std::auto_ptr
  */
  typedef std::auto_ptr<TLockGuard> Lock;
  /// Lock the mutex protecting graphics updates to make changes safely
  Lock AcquireLock();

  /// Get a new canvas to draw objects on
  TCanvas* GetCanvas(const char* title=0, bool preventclose=true, 
		     bool hidemenu=false);

private:
  void LoadStyle();

  TApplication _app;
  TMutex _mutex;
  TThread _thread;
  std::vector<TCanvas*> _canvases;
  TGMainFrame* _mainframe;
  bool _single_window;
  int _window_w;
  int _window_h;
};

#endif
