#ifndef PADZOOMER_h
#define PADZOOMER_h

#include "TExec.h"

class TVirtualPad;

class PadZoomer : public TExec{
public:
  PadZoomer(TVirtualPad* pad=0) : TExec("zoomer","return;")
  { Attach(pad); }
  virtual ~PadZoomer() {}
  //don't ever paint
  virtual void Paint(Option_t*) {}
  virtual void Exec(const char* x=0);
private:
  TVirtualPad* owner;
  void Attach(TVirtualPad* pad);

};


#endif /* PADZOOMER_h*/
