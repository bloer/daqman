/** @file LightYieldCorrection.hh
    @brief Monte Carlo simulation of a single pulse to determine the light yield correction factor used when integrating
    @author bloer
    @ingroup daqroot
*/

// Add this to the daqroot environment: ClassDef

class TGraph; 

/// Create fake pulse by repeating singlepe several times from exp distributions
TGraph* MakeFakePulse(TGraph* singlepe, int nphotons, TGraph* output=0,
		      double spe_resolution = 0.0, double t0 = -0.18, 
		      double fprompt = 0.25, double tslow = 1.4,
		      double tfast = 0.007, double jitter = 0.,
		      bool use_binomial = true);


///Get a TGraph representing the average single photoelectron response
TGraph* GetNormalizedSPETemplate(int channel);

///Vary the number of photons while testing the light yield correction
void TestCorrectionWithNphotons(int channel, int ntrials, int npoints, 
				int min_photons, int photons_step);

/// Vary the timing parameters while testing the light yield correction
void TestCorrectionWithTimeParams(int channel, int ntrials,
				  double t0center, double t0width,
				  double fpromptcenter, double fpromptwidth,
				  double tslowcenter, double tslowwidth);

class TGraphErrors;
/// Plot and fit the light yield vs npe
TGraphErrors* PlotYieldvsNPE(const char* file, int npoints, int min_photons,
			     int photons_step, const char* treename="yield");
