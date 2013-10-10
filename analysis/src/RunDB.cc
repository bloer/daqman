#include "RunDB.hh"
#include "FitTH1F.hh"
#include "utilities.hh"

#include "TH1F.h"
#include "TF1.h"
#include "TNamed.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TPad.h"
#include "FitOverROI.hh"
#include "TFitResult.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <memory>



RunDB::laserinfo::laserinfo(int Runid, int Channel, time_t Runtime, int Filters,
			    double Roi_start, double Roi_end, FitTH1F* h, std::string Fitsettings) : 
  runid(Runid), channel(Channel), runtime(Runtime), filters(Filters),
  roi_start(Roi_start), roi_end(Roi_end), inserttime(0), updatetime(0), fitsettings(Fitsettings) 
{
	using namespace FitOverROI;
  nbins = h->GetNbinsX();
  xmin = h->GetBinLowEdge(1);
  xmax = h->GetBinLowEdge(nbins+1);
  
  
  TF1* spefunc = h->GetFunction("spefunc");
	TFitResultPtr fitresult=h->fitResult;

  if(spefunc){
    spefunc->GetRange(fitmin, fitmax);
    entries =(int) h->GetEntries();
	constant= spefunc->GetParameter("CONSTANT");
	lambda= spefunc->GetParameter("LAMBDA");
	spe_mean= spefunc->GetParameter("SPE_MEAN");
	spe_sigma= spefunc->GetParameter("SPE_SIGMA");
	amp_E= spefunc->GetParameter("AMP_E");
	p_E= spefunc->GetParameter("P_E");
	shotnoise= spefunc->GetParameter("SHOTNOISE");
	pedmean= spefunc->GetParameter("PEDMEAN");
	
    chi2 = spefunc->GetChisquare();
    ndf = spefunc->GetNDF();
	
	valley = spefunc->GetMinimumX(0,spe_mean);
  }
  
  if(fitresult.Get()){
	constant_err_low= fitresult->LowerError(CONSTANT);
	lambda_err_low= fitresult->LowerError(LAMBDA);
	spe_mean_err_low= fitresult->LowerError(MEAN);
	spe_sigma_err_low= fitresult->LowerError(SIGMA);
	amp_E_err_low= fitresult->LowerError(AMP_E);
	p_E_err_low= fitresult->LowerError(P_E);
	shotnoise_err_low= fitresult->LowerError(SHOTNOISE);
	pedmean_err_low= fitresult->LowerError(PEDMEAN);
  
  	constant_err_high= fitresult->UpperError(CONSTANT);
	lambda_err_high= fitresult->UpperError(LAMBDA);
	spe_mean_err_high= fitresult->UpperError(MEAN);
	spe_sigma_err_high= fitresult->UpperError(SIGMA);
	amp_E_err_high= fitresult->UpperError(AMP_E);
	p_E_err_high= fitresult->UpperError(P_E);
	shotnoise_err_high= fitresult->UpperError(SHOTNOISE);
	pedmean_err_high= fitresult->UpperError(PEDMEAN);
  
	const double* params=fitresult->GetParams();
	pdfmean= FitOverROI::m_n(params);
	pdfmean_err=FitOverROI::pdfmean_error_corr(fitresult);
	pdfsigma=FitOverROI::sigma_n(params);
  }
  else{
    
    Message(ERROR)<<"Error: no fitresult found for channel "<<Channel
		  <<"."<<std::endl
		  <<"This bug should be fixed. If it's coming up, get in touch with Jason Brodsky, jaybrod@gmail.com"<<std::endl;
    throw std::runtime_error("no fitresultptr");
  }
}

 

void RunDB::runinfo::Init(bool reset)
{
  runid=-1;
  starttime=0;
  endtime=0;
  events=0;
  livetime=0;
  type="regular";
  comment="";
  drift_hv=0;
  extraction_hv=0;
  trigger_veto = -1;
  pre_trigger_time_us = -1;
  post_trigger_time_us = -1;
  channels.clear();
  if(!reset)
    InitializeParameterList();
}

void RunDB::runinfo::InitializeParameterList()
{
  RegisterParameter("runid", runid, "unique ID number for the run");
  RegisterParameter("starttime",starttime,"Timestamp at start of run");
  RegisterParameter("endtime",endtime, "Timestamp at end of run");
  RegisterParameter("events", events, "Number of events recorded during run");
  RegisterParameter("livetime", livetime, 
		    "Runtime scaled for number of accepted triggers");
  RegisterParameter("type", type, "Label giving purpose of run");
  RegisterParameter("comment",comment,"Generic information about run");
  RegisterParameter("drift_hv",drift_hv,"Setting of drift_hv field");
  RegisterParameter("extraction_hv",extraction_hv,"Extraction field setting");
  RegisterParameter("trigger_veto",trigger_veto,
		    "Length of per-trigger veto in ms; -1 to disable");
  RegisterParameter("pre_trigger_time_us",pre_trigger_time_us,
		    "Length of pre-trigger digitization window in us");
  RegisterParameter("post_trigger_time_us",post_trigger_time_us,
		    "Length of post-trigger digitization window in us");
  RegisterParameter("channels", channels, "Set of info for all channels");
  RegisterReadFunction("clear_channels" , ConfigFunctorDummyRead(), 
		       "obsolete reference for back-compatibility");
  
}

RunDB::runinfo::channelinfo::channelinfo(int ch) : 
  ParameterList("channelinfo","Information about the state of a daq channel for database insertion"), 
  channel(ch) , voltage(0), amplification(1), spe_mean(1), spe_sigma(0), calibration_run(-1),
  calibration_channel(-1), cal_filters(2), reload_cal(true), 
  reload_cal_run(true)
{
  InitializeParameterList();

}

void RunDB::runinfo::channelinfo::InitializeParameterList()
{
  RegisterParameter("channel", channel, "Unique ID number");
  RegisterParameter("voltage",voltage, "High Voltage applied to PMT");
  RegisterParameter("amplification", amplification, 
		    "Amplification applied to channel after PMT output");
  RegisterParameter("spe_mean",spe_mean,
		    "Single photoelectron mean from calibration database");
  RegisterParameter("spe_sigma",spe_sigma,
                    "Single photoelectron sigma from calibration database");
  RegisterParameter("calibration_run", calibration_run,
		    "Run from which calibration data was loaded");
  RegisterParameter("calibration_channel", calibration_channel,
		    "Channel which should be used for calibration data");
  RegisterParameter("cal_filters", cal_filters,
		    "Only consider runs with x filters for calibration");
  RegisterParameter("reload_cal", reload_cal,
		    "Search for calibration info in DB rather than saved vals");
  RegisterParameter("reload_cal_run", reload_cal_run,
		    "Search for new calibration run rather than previous one");
}


bool RunDB::runinfo::InsertIntoDatabase()
{
  bool success = false;
  return success;
}

std::istream & operator>> (std::istream & in, RunDB::runinfo::runtype & type)
{
  std::string temp = "";
  in >> temp;
  type.assign (temp);
  return in;
}

const std::string& runinfokey="runinfo";
const std::string& runinfohelp="Information about daq runs for database";

RunDB::runinfo::runinfo() : ParameterList(runinfokey, runinfohelp)
{
  Init();
}

  
bool RunDB::runinfo::SyncWithDatabase()
{

  Message(ERROR)<<"Attempt to sync with database, but mysql access disabled.\n";
  return false;
}




int RunDB::InsertLaserInfo(const std::vector<RunDB::laserinfo>& info)
{
  int lines=0;
  return lines;
}

int RunDB::InsertLaserInfo(const RunDB::laserinfo& info)
{
  std::vector<RunDB::laserinfo> vec;
  vec.push_back(info);
  return RunDB::InsertLaserInfo(vec);
}

int RunDB::InsertSpeInfo(const std::vector<RunDB::speinfo> &info){
  int lines=0;
  return lines;
}

double GetSPEScaleFactor(int run, int channel, int filters, bool isMC)
{
  const double failure = 0;
  return failure;
}

TGraph* PlotDatabaseVars(const char* varx, const char* vary, const char* table,
			 const char* cut)
{

  Message(ERROR)<<"Database access disabled!\n";
  return 0;
}

/// Read time_t from a string
std::istream& operator>>(std::istream& in, RunDB::runinfo::time_param& t)
{
  std::string dummy;
  in>>dummy;
  if(dummy[4] == '-'){
    //this is a string
    int year, month, day;
    sscanf(dummy.c_str(),"%4d-%2d-%2d",&year, &month, &day);
    in>>dummy;
    int hours, minutes, seconds;
    sscanf(dummy.c_str(),"%2d:%2d:%2d", &hours, &minutes, &seconds);
    struct tm timeinfo;
    timeinfo.tm_sec = seconds;
    timeinfo.tm_min = minutes;
    timeinfo.tm_hour = hours;
    timeinfo.tm_mday = day;
    timeinfo.tm_mon = month-1;
    timeinfo.tm_year = year-1900;
    timeinfo.tm_isdst=-1;
        
    t.t = timegm(&timeinfo);
    return in;
  }
  //if we get here, it should be an integer
  t.t = atoi(dummy.c_str());
  return in;
}
/// Make time_t print as a string
std::ostream& operator<<(std::ostream& out,const RunDB::runinfo::time_param& t)
{
  char str[20];
  std::strftime(str,20,"%Y-%m-%d %H:%M:%S",std::gmtime(&t.t));
  out<<str;
  return out;
}


/*
Command used to create laser table:

CREATE TABLE `testlaser` (
  `runid` int(11) NOT NULL,
  `channel` int(11) NOT NULL,
  `runtime` datetime NOT NULL,
  `filters` int(11) DEFAULT NULL,
  `roi_start` double DEFAULT NULL,
  `roi_end` double DEFAULT NULL,
  `nbins` int(11) DEFAULT NULL,
  `xmin` double DEFAULT NULL,
  `xmax` double DEFAULT NULL,
  `fitmin` double DEFAULT NULL,
  `fitmax` double DEFAULT NULL,
  `fitsettings` text DEFAULT NULL,
  `entries` int(11) DEFAULT NULL,
  `constant` double DEFAULT NULL,
  `lambda` double DEFAULT NULL,
  `spe_mean` double DEFAULT NULL,
  `spe_sigma` double DEFAULT NULL,
  `amp_E` double DEFAULT NULL,
  `p_E` double DEFAULT NULL,
  `shotnoise` double DEFAULT NULL,
  `pedmean` double DEFAULT NULL,
  `constant_err_low` double DEFAULT NULL,
  `lambda_err_low` double DEFAULT NULL,
  `spe_mean_err_low` double DEFAULT NULL,
  `spe_sigma_err_low` double DEFAULT NULL,
  `amp_E_err_low` double DEFAULT NULL,
  `p_E_err_low` double DEFAULT NULL,
  `shotnoise_err_low` double DEFAULT NULL,
  `pedmean_err_low` double DEFAULT NULL,
  `constant_err_high` double DEFAULT NULL,
  `lambda_err_high` double DEFAULT NULL,
  `spe_mean_err_high` double DEFAULT NULL,
  `spe_sigma_err_high` double DEFAULT NULL,
  `amp_E_err_high` double DEFAULT NULL,
  `p_E_err_high` double DEFAULT NULL,
  `shotnoise_err_high` double DEFAULT NULL,
  `pedmean_err_high` double DEFAULT NULL,
    `pdfmean` double DEFAULT NULL,
	`pdfmean_err` double DEFAULT NULL,
	`pdfsigma` double DEFAULT NULL,
  `chi2` double DEFAULT NULL,
  `ndf` int(11) DEFAULT NULL,
  `inserttime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
*/


bool RunDB::runinfo::channelinfo::LoadCalibrationInfo(time_param runtime,
						      bool isMC)
{
  //if we get here, there is no run with matching voltage
  Message(DEBUG)<<"Unable to load calibration info for channel "<<channel
		  <<"\n";
  return false;
}

int RunDB::runinfo::LoadCalibrationInfo()
{
  
  if(channels.size() == 0){
    Message(WARNING)<<"Attempted to load calibration without any channels!\n";
    return false;
  }

  Message(ERROR)<<"Attemptted to load calibration without database support!\n";
  return 0;
}

const RunDB::runinfo::channelinfo* RunDB::runinfo::GetChannelInfo(int channel)
{
  std::set<channelinfo>::iterator ch = channels.find(channel);
  if(ch != channels.end())
    return &(*ch);
  return 0;
}

void RunDB::runinfo::ResetRunStats()
{
  starttime = 0;
  endtime = 0;
  events = 0;
  livetime = 0;
}


int RunDB::GetRunNearDate(const std::string& datestr,  bool isMC)
{

  return -1;
}
 
RunDB::campaigninfo::campaigninfo()
{
  Init();
}

int RunDB::campaigninfo::LoadCampaignInfo(time_t run_start_time) 
{
    Init();
    
    Message(ERROR)<<"RunDB::campaigninfo::LoadCampaignInfo"<<std::endl
		  <<"Unable to load campaign info"<<std::endl;
    return 0;
    
}

bool RunDB::campaigninfo::pmtinfo::LoadCampaignInfo(long campaign_id, long version_id, int channel)
{
    return false;
}

RunDB::campaigninfo::pmtinfo* RunDB::campaigninfo::GetPMTInfo(int channel)
{
    std::map<int, RunDB::campaigninfo::pmtinfo>::iterator p = pmts.find(channel);
    if(p != pmts.end())
	return &(p->second);
    else
	return 0;
}

void RunDB::campaigninfo::Init()
{
    campaign_id = 0;
    version_id = 0;
    pmts.clear();
}

RunDB::campaigninfo::pmtinfo::pmtinfo()
{
    serial_id = "";
    photocathode_x = 0;
    photocathode_y = 0;
    photocathode_z = 0;
    photocathode_r = 0;
    photocathode_theta = 0;
    photocathode_area = 1;
    photocathode_exp_area = 1;
    qe = 1;
}
