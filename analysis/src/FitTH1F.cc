#include "FitTH1F.hh"
 
FitTH1F::FitTH1F(const char *name,const char *title,Int_t nbins,Double_t xlow,Double_t xup) :
  TH1F(name,title,nbins,xlow,xup)
{
	fitResult=0;
}
