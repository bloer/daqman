/** @file RunDB.hh
    @brief defines RunDB namespace and functions for accessing database.
    @author bloer
    @ingroup daqroot
*/

#ifndef RUNDB_h
#define RUNDB_h
#include "phrase.hh"

//hide this from ROOT
#ifndef __CINT__
#include "ParameterList.hh"
#endif 
#include "Rtypes.h"
#include <vector>
#include <map>
#include <string>
#include <time.h>

//forward declarations
class TH1F;
class FitTH1F;
class TGraph;

/** @file RunDB.hh
 *  @brief mysql database interface utilities
 *
 */

//make sure this function gets included in the root dictionary:
//ClassDef

///@addtogroup daqroot
///@{

/// Get the mean of the single photoelectron peak for channel n 
/// Searches for the nearest laser run by time, optionally restricted by the 
/// number of filters
double GetSPEScaleFactor(int run, int channel, int filters=-1,  bool isMC = false);

/// Make a plot of two variables in the database
TGraph* PlotDatabaseVars(const char* varx, const char* vary, const char* table,
			 const char* cut=0);


///@}

/** @namespace RunDB
    @brief Some classes to read/write the DB. ROOT doesn't know about them
*/

namespace RunDB{

  ///Utility functions

  ///Get the number of the run that was nearest to the time specified. 
  ///if datestr=="", use today.  should be YYYY-MM-DD [HH:MM[:SS]]
  int GetRunNearDate(const std::string& datestr="",  bool isMC=false);
  
  
  
  class laserinfo;
  //need to specify RunDB namespace since rootcint is retarded
  ///Insert several laser channels at once
  int InsertLaserInfo(const std::vector<RunDB::laserinfo>& info);
  ///Insert a single channel's laser info
  int InsertLaserInfo(const RunDB::laserinfo& info);
  
  /** @class laserinfo
   *  @brief struct containing all of the variables in the laserruns tree.
   */
  class laserinfo{
  public:
    int runid;               ///< id of the run
    int channel;             ///< id of the channel
    time_t runtime;          ///< timestamp of the start of the run
    int filters;             ///< number of filters on the laser used in the run
    double roi_start;        ///< Start time of the laser region
    double roi_end;          ///< end time of the laser region
    int nbins;               ///< number of bins in the spectrum histogram
    double xmin;             ///< minimum counts*samps of the spectrum histo
    double xmax;             ///< maximum counts*samps of the spectrum histo
    double fitmin;           ///< minimum range to fit the spectrum
    double fitmax;           ///< maximum range to fit the spectrum
    int entries;             ///< number of valid entries in the events tree
    //fit parameters
    double constant, lambda, spe_mean, spe_sigma, amp_E, p_E;
    double shotnoise, pedmean;
    double constant_err_low, lambda_err_low, spe_mean_err_low;
    double spe_sigma_err_low, amp_E_err_low, p_E_err_low, shotnoise_err_low;
    double pedmean_err_low;
    double constant_err_high, lambda_err_high, spe_mean_err_high;
    double spe_sigma_err_high, amp_E_err_high, p_E_err_high;
    double shotnoise_err_high, pedmean_err_high;
    double pdfmean, pdfmean_err;
    double pdfsigma;
    double valley;
	
    double chi2;             ///< chi-squared of the fit
    int ndf;                 ///< number of degrees of freedom in the fit
    time_t inserttime;       ///< time the entry was inserted in the database
    time_t updatetime;       ///< time the entry was updated in the database
    std::string fitsettings;  ///< all options used to make the fit converge
    
    ///default constructor
    laserinfo() {}
    ///constructor that takes a histogram
    laserinfo(int Runid, int Channel, time_t Runtime, int Filters,
	      double Roi_start, double Roi_end, FitTH1F* h, 
	      std::string fitsettings);
    /// specify all parameters except timestamps explicitly
    laserinfo(int Runid, int Channel, time_t Runtime, int Filters, 
	      double Roi_start, double Roi_end, int Nbins, 
	      double Xmin, double Xmax, double Fitmin, double Fitmax,
	      const std::string& /*Fitoptions*/,
	      int Entries, double Constant, double Lambda, double Spe_mean, 
	      double Spe_sigma, double Amp_E, double P_E, double Shotnoise ,
	      double Pedmean,double /*Shotnoise2*/,
	      double /*Pedweight*/, double Chi2, 
	      int Ndf, std::string Fitsettings) : 
      runid(Runid), channel(Channel), runtime(Runtime), filters(Filters),
      roi_start(Roi_start), roi_end(Roi_end), nbins(Nbins),
      xmin(Xmin), xmax(Xmax), fitmin(Fitmin), fitmax(Fitmax),
      entries(Entries), constant(Constant), lambda(Lambda), spe_mean(Spe_mean),
      spe_sigma(Spe_sigma), 
      amp_E(Amp_E), p_E(P_E), shotnoise(Shotnoise) ,pedmean(Pedmean),
      chi2(Chi2), ndf(Ndf),
      inserttime(0), updatetime(0),
      fitsettings(Fitsettings) {}
  };

  class speinfo;
  //need to specify RunDB namespace since rootcint is retarded
  ///Insert several channel specalibrations at once
  int InsertSpeInfo(const std::vector<RunDB::speinfo>& info);
  
  /** @class speinfo
   *  @brief struct containing all of the variables in the spefit.
   */
  class speinfo{
  public:
    int runid;               ///< id of the run
    int channel;             ///< id of the channel
    time_t runtime;          ///< timestamp of the start of the run
    double fitmin;           ///< minimum range to fit the spectrum
    double fitmax;           ///< maximum range to fit the spectrum
    //fit parameters
    double pdfmean, pdfmean_err; ///< spe mean and err
    double pdfsigma;         ///< spe sigma
    double chi2;             ///< chi-squared of the fit
    int ndf;                 ///< number of degrees of freedom in the fit
    double height1, height2, height3, height4; ///< constants of 1,2,3,4 pe gaussians
    
    ///default constructor
    speinfo() {}

    /// specify all parameters 
    speinfo(int Runid, int Channel, time_t Runtime, double Fitmin, double Fitmax, double Pdfmean, double Pdfmean_err, double Pdfsigma, double Chi2, int Ndf, double Height1, double Height2, double Height3, double Height4) : runid(Runid), channel(Channel), runtime(Runtime), fitmin(Fitmin), fitmax(Fitmax), pdfmean(Pdfmean), pdfmean_err(Pdfmean_err), pdfsigma(Pdfsigma), chi2(Chi2), ndf(Ndf), height1(Height1), height2(Height2), height3(Height3), height4(Height4){}
  };
  
  /** @class runinfo
   *  @brief struct containing all the variables of the daqruns tree
   */
  //hide ParameterList from root
#ifndef __CINT__
  class runinfo : public ParameterList{
#else
  class runinfo{
#endif
  public:
    
    ///default constructor does nothing
    runinfo();
    /// virtual destructor to make root happy
    virtual ~runinfo() {}
    /// Initialize parameters to default values
    void Init(bool reset=false);
  public:
    ///resets the io keys after a copy operation
    void InitializeParameterList(); 
    

    ///Insert this run into the database
    bool InsertIntoDatabase();
    /// Load calibration info from the laserruns database
    int LoadCalibrationInfo();
    /// Reset the statistics which can be read from the file
    void ResetRunStats();
    /// Get information from database, but don't overrwrite control/calibration
    bool SyncWithDatabase();
    
    /// utility class to verify run type when reading from file
    struct runtype : public std::string {
      runtype& operator=(const std::string& right){assign(right); return *this;}
      virtual ~runtype(){}
      ClassDef(runtype,1)
    };
    /// utility class to read/write time_t as string
    struct time_param{
      virtual ~time_param(){}
      time_t t;
      time_param& operator=(const time_t& right){ t = right; return *this; }
      operator time_t(){ return t; }
      ClassDef(time_param,1)
    };
    
    long runid;           ///< id of the run
    time_param starttime; ///< start timestamp of the run
    time_param endtime;   ///< end timestamp of the run
    long events;          ///< number of events in the tree 
    double livetime;      ///< runtime scaled by the number of accepted triggers
    runtype type;         ///< what class of run it is (laser, background, etc)
    std::string comment;       ///< generic info about the run
    double drift_hv;      ///< setting of the drift_hv field
    double extraction_hv; ///< setting of the extraction_hv field
    double trigger_veto;  ///< length of per-trigger veto in msec, -1 disables
    double pre_trigger_time_us; ///< length of pre-trigger digitization window
    double post_trigger_time_us; ///< length of post-trigger digitization window
    /** @struct channelinfo
     *  @brief this info is stored for each channel in each run
     */
    //hide ParameterList from root
#ifndef __CINT__
    class channelinfo : public ParameterList{
#else
    class channelinfo{
#endif
    public:
      channelinfo(int ch=-1);
      virtual ~channelinfo() {}
      void InitializeParameterList(); 
      /// Search the calibration database for scale factors
      bool LoadCalibrationInfo(time_param runtime,
			       bool isMC=false);
      
      int channel;               ///< id number of the channel
      int voltage;               ///< HV power supply voltage
      double amplification;      ///< total amplification before digitizer
      double spe_mean;           ///< single photoelectron mean from database
      double spe_sigma;          ///< std dev of spe response from db
      long calibration_run;      ///< run from which calibration data was loaded
      long calibration_channel;  ///< channel calibration data loaded from
      int cal_filters;           ///< use only calibration runs with n filters
      bool reload_cal;           ///< Use previous calibration or search db?
      bool reload_cal_run;       ///< Search for new optimum calibration run?
      
      ///comparison operator for channel number
      bool operator==(int chan) const{ return chan == channel; }
      ///comparison operator for channelinfo object
      bool operator==(const channelinfo& right) const
      { return right.channel==channel;}
      ///less than comparson for set storage
      bool operator<(const channelinfo& right) const
      { return channel<right.channel; }
      ClassDef(channelinfo,2);
    };
    
    std::set<channelinfo> channels;  ///< vector of channel info
    
    const channelinfo* GetChannelInfo(int channel); ///< Get the info for channel n
      
    ClassDef(runinfo,2);
  };

   /** @class campaigninfo
   *  @brief struct containing all the variables related to the different campaigns
   */
  
  class campaigninfo
  {
  public:
      
      ///default constructor does nothing
      campaigninfo();
      /// Initialize parameters to default values
      void Init();
  public:
      
      /// Load campaign info from the database based on start time of run
      int LoadCampaignInfo(time_t run_start_time);
          
      long campaign_id;           ///< id of the campaign
      long version_id;            ///< id of the version
      
      /** @struct pmtinfo
       *  @brief this info is stored for each pmt 
       */
      
      class pmtinfo
      {
      public:
	pmtinfo();
	  /// Search the campaign database for pmt information
	bool LoadCampaignInfo(long campaign_id, long version_id, int channel);

	  
	  phrase serial_id;             ///< serial number
	  double photocathode_x;        ///< x position of the center of the photocathode
	  double photocathode_y;        ///< y position of the center of the photocathode
	  double photocathode_z;        ///< z position of the center of the photocathode
	  double photocathode_r;        ///< r (cylindrical coord) position of the center of the photocathode
	  double photocathode_theta;    ///< theta (cylindrical coord) position of the center of the photocathode
	  double photocathode_area;     ///< area [cm2] of the photocathode
	  double photocathode_exp_area; ///< exposed fraction of photocathode area
	  double qe;                    ///< quantum efficiency
	  
      };
      std::map<int, pmtinfo> pmts;  ///< map of channel number to pmt info
      
      pmtinfo* GetPMTInfo(int channel); ///< Get the pmt info for channel n
      
  };
};
  
      std::istream& operator>>(std::istream& in, RunDB::runinfo::runtype& type);
      std::ostream& operator<<(std::ostream& out, const RunDB::runinfo::time_param& t);
      std::istream& operator>>(std::istream& in, RunDB::runinfo::time_param& t);
      
      
#endif
      
