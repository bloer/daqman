/** @file FitTH1F.hh
    @brief Defines the TH1F class to store a fitresult with each TH1F
    @author jbrodsky
    @ingroup daqman
*/

#ifndef FITTH1F_h
#define FITTH1F_h

#include "TH1F.h"
#include "TFitResultPtr.h"

/** @class FitTH1F
    @brief A small extensin of TH1F to store a FitResults.
    @ingroup cuts
*/
class FitTH1F : public TH1F{
public:
  FitTH1F(const char *name,const char *title,Int_t nbins,Double_t xlow,Double_t xup);
  FitTH1F():TH1F(){}
  /// Destructor 
  virtual ~FitTH1F(){}
  
  TFitResultPtr fitResult;
	
};


#endif
