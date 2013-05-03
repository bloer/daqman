#include "PulseFit.hh"
#include "TF1.h"
PulseFit::PulseFit()
{ 
  Clear(); 
}

PulseFit::~PulseFit()
{
  //std::cerr<<"Deleting pulse fit object.\n";
}

void PulseFit::Clear()
{
  fit_done = false;
  fit_result = -1;
  start_index = -1;
  end_index = -1;
  range_low = -1;
  range_high = -1;
  
  amplitude = 1;
  c1 = 0.5;
  tau1 = 1;
  tau2 = 1;
  sigma = 1;
  decay = 1;
  baseline = 1;
  t0 = 1;
  rc = 1;
}

TF1* PulseFit::GetTF1()
{
  TF1* func = new TF1("pulsefit", this, 0,100,nparams,(char*)"PulseFit");
  func->SetParNames("Amplitude", "C1", "tau1", "tau2", "Sigma", 
		    "Decay Constant", "Offset", "Delay", "RC");
  func->SetParameters(amplitude, c1, tau1, tau2, sigma, decay, baseline, t0, rc);
  func->SetRange(range_low, range_high);
  return func;
}

void PulseFit::StoreParams(TF1* func)
{
  amplitude = func->GetParameter(0);
  c1 = func->GetParameter(1);
  tau1 = func->GetParameter(2);
  tau2  = func->GetParameter(3);
  sigma = func->GetParameter(4);
  decay = func->GetParameter(5);
  baseline = func->GetParameter(6);
  t0 = func->GetParameter(7);
  rc = func->GetParameter(8);
  func->GetRange(range_low, range_high);
}
  

Double_t PulseFit::operator()(Double_t* x, Double_t* par)
{
  Double_t t = x[0];
  amplitude = par[0];//Amplitude
  c1 = par[1]; //Fraction of fast component
  tau1 = par[2];//Fast Component lifetime
  tau2 = par[3];//Slow Component lifetime
  sigma = par[4];//PMT Jitter
  decay = par[5];//Decay time of integrator
  baseline = par[6];//Baseline
  t0 = par[7];//Event start time
  rc = par[8];//Decay time of RC circuit
  
  double result = baseline - amplitude * 
    ( c1*rc*decay/(2*(rc - decay))*
      (common_func3(t, t0, sigma, tau1, rc) + common_func3(t, t0, sigma, rc, tau1) -
       common_func3(t, t0, sigma, tau1, decay) - common_func3(t, t0, sigma, decay, tau1))
     
      +(1-c1)*rc*decay/(2*(rc - decay))*
      (common_func3(t, t0, sigma, tau2, rc) + common_func3(t, t0, sigma, rc, tau2) -
       common_func3(t, t0, sigma, tau2, decay) - common_func3(t, t0, sigma, decay, tau2))
      );

  return result;
}

