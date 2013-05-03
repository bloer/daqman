/** @file FitOverROI.hh
    @brief defines FitOverROI namespace and functions for fitting laser data.
    @author jbrodsky et Al.
    @ingroup daqroot
*/

#ifndef FITOVERROI_h
#define FITOVERROI_h
#include <vector>
#include <string>

#include "TFitResultPtr.h"
#include "FitTH1F.hh"
#include "ChanFitSettings.hh"

//forward declarations


/** @file FitOverROI.hh
 *  @brief mysql database interface utilities
 *
 */

//make sure this function gets included in the root dictionary:
//



/** @namespace FitOverROI
    @brief Functions to fit laser pulses
*/
namespace FitOverROI{
enum PARAMETERS {CONSTANT, LAMBDA, MEAN, SIGMA, AMP_E, P_E , SHOTNOISE, PEDMEAN,  NPAR };
Double_t background_func(Double_t* x, Double_t* params);  
Double_t gauss_func(Double_t* x, Double_t* params);  
Double_t response_0(Double_t* x, Double_t* params);
Double_t response_1(Double_t* x, Double_t* params);
Double_t response_2(Double_t* x, Double_t* params);
Double_t m_n(const Double_t* params);
Double_t sigma_n(const Double_t* params);
Double_t response_multi(Double_t* x, Double_t* params);
Double_t SPEFunc(Double_t* x, Double_t* params);
TFitResultPtr FitSPE(FitTH1F* spe, ChanFitSettings& CFS, int ntriggers, bool allow_bg = true, bool force_old = false);
double pdfmean_error_corr(TFitResultPtr& fitresult);
    
};


#endif
