/** @file PulseFit.hh
    @brief Defines the PulseFit storage class
    @author rsaldanha
    @ingroup modules
    @ingroup daqroot
*/

#ifndef PULSEFIT_h
#define PULSEFIT_h

#include "TMath.h"
#include "TObject.h"

class TF1;

/** @class PulseFit
    @brief Defines a function to fit scintillation pulses and store results
    @ingroup modules
    @ingroup daqroot
*/
class PulseFit{
  static const int nparams = 9;  ///< number of parameters in the fit
public:
  /// Constructor
  PulseFit();
  /// Destructor
  virtual ~PulseFit();
  /// Get a pointer to the fit function
  TF1* GetTF1(); 
  /// Store parameters from the supplied TF1
  void StoreParams(TF1* func);
  /// Reset all variables to defaults
  void Clear();
  //fit info:
  bool fit_done;     ///< was the fit completed?
  int fit_result;    ///< the minuit result of the fit
  double chi2;       ///< chi-squared from the fit
  int ndf;           ///< degrees of freedom from the fit
  int start_index;   ///< index marking the start of the fit
  int end_index;     ///< index for the end of the fit
  double range_low;  ///< low end of fitting/drawing range
  double range_high; ///< high end of fitting/drawing range
  //fit parameters:
  double amplitude;  ///< Amplitude fit parameter
  double c1;         ///< Fraction of fast component fit parameter
  double tau1;       ///< Fast Component lifetime fit parameter
  double tau2;       ///< Slow Component lifetime fit parameter
  double sigma;      ///< PMT Jitter fit parameter
  double decay;      ///< Decay time of integrator fit parameter
  double baseline;   ///< Baseline fit parameter
  double t0;         ///< Event start time fit parameter
  double rc;         ///< Decay time of RC circuit fit parameter
  
  /// needed to generate the TF1
  Double_t operator()(Double_t* x, Double_t* par);
  
  //helper functions
  
  Double_t common_func1 (double t, double t_0, double sig, double tau)
  {
    return (1 + TMath::Erf((-sig*sig + (t-t_0)*tau)/(TMath::Sqrt(2)*sig*tau)));
  }
  
  Double_t common_func2 (double t, double t_0, double sig, double tau)
  {
    return TMath::Exp((sig*sig - 2*(t-t_0)*tau)/(2*tau*tau));
  }

  Double_t common_func3 (double t, double t_0, double sig, double tau_a, double tau_b)
  {
    return common_func2(t, t_0, sig, tau_a)*common_func1(t, t_0, sig, tau_a)/(tau_b - tau_a);
  }
  ClassDef(PulseFit,1)
};

#endif
