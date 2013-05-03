#include "EventHandler.hh"
#include "ConvertData.hh"
#include "Message.hh"

#include "MCLaserData.hh"
#include "averageSPE.hh"
#include <iomanip>

static double hb[3276800];
static int hp[3276800];

// new fitted values 
const double pdf_mean = 60.8;
const double pdf_rms = 23.89;
const double spe_mean  = 69;
const double spe_sigma = 19.914;
const double exp_tau = 26.847;
const double P_E = 0.1091;

MCLaserData::MCLaserData() : BaseModule(GetDefaultName(), "To be inserted... ")
{
  AddDependency<ConvertData>();

  RegisterParameter(the_runinfo.GetDefaultKey(), the_runinfo);
  RegisterParameter(the_params.GetDefaultKey(), the_params);

  mcwriter = new MCRawWriter();
}

MCLaserData::~MCLaserData()
{

}

int MCLaserData::Start_Run()
{ 
  Message(INFO) << "MCLaserData Start \n";

  histPMT = &hb[0];
  barePMT = &hp[0];

  mc_written = 0;

  std::stringstream filename;
  filename<<"Run"<<std::setw(6)<<std::setfill('0')<<the_params.mch_run_number;

  mcwriter->SetFilename(filename.str().c_str());
  mcwriter->Initialize();
  mcwriter->SetRunID(the_params.mch_run_number);

  //  RunDB::runinfo *rinfo = mcwriter->GetRunInfo();
  the_runinfo.starttime = time(NULL);
  the_runinfo.comment = "MC Laser Data";

  // fser = TFile::Open("mc/R11065_ser.root","READ");
  // if(!fser->IsOpen()) {
  //   Message(ERROR) << " file mc/R11065.root is missing !!!\n";
  //   return -1;
  // }

  // hser = (TH1F*) fser->Get("R11065_ser");
    
  //xxxt, pmt_ser, xtime_start, xtime_end and xtime_ser are global constants defined in mc/include/averageSPE.hh
  averageSPE(xxxt, pmt_ser);
    
  ser_integral = 0.0;
  for(int i=xtime_start; i<xtime_end; i++) 
  {
      //TEMP
      //Normalize such that an spe_mean of 69(= ser integral) 
      //gives integer values to all samples
      pmt_ser[i] = (int)(pmt_ser[i]/4 + 0.5); 
      ser_integral += pmt_ser[i];
  }
  std::cout << " ser integral = " << ser_integral << " counts*bin" << std::endl;

  return 0;
}

int MCLaserData::Initialize()
{ 
  //  Message(INFO) << "MCLaserData Init \n";

  return 0;
}

int MCLaserData::Process(EventPtr event)
{
    int idx;

  MCRawEventPtr raw(new MCRawEvent);
  size_t event_size_bytes = the_params.mch_number_of_channels*the_params.mch_number_of_samples*2;
  int blocknum = raw->AddDataBlock(RawEvent::MONTECARLO, event_size_bytes);
  buffer = raw->GetRawDataBlock(blocknum);

  EventDataPtr data = event->GetEventData();
  std::vector<ChannelData>::iterator chdata;

  int s_start, s_end, s_start_max;
  s_start = -1; s_end = -1; s_start_max = -1;

  // clean

  for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
  {
    idx = npmt*the_params.mch_number_of_samples;
    for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime++) 
    {
      histPMT[idx+itime]=0.0;
      barePMT[idx+itime]=0;
    }
  }

  if(the_params.mch_baseline != 0) 
  {//Fixed baseline + random hits + noise

    // baseline
    
    for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
    {
	idx = npmt*the_params.mch_number_of_samples;
	for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime++) 
	{
	    histPMT[idx+itime] += the_params.mch_baseline;
	}
    }
    
    // random hits

    if(the_params.mch_single_rate != 0) 
    {

      double prob = the_params.mch_single_rate * the_params.mch_sampling_time * 1.0e-9;
      Message(WARNING) << "single_rate <> 0 !!\n";
      for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
      {
	  idx = npmt*the_params.mch_number_of_samples;
	  for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime++) 
	  {
	      if(ran.Uniform() < prob) 
		  barePMT[idx+itime] += 1.0;
	  }
      }
      
    }
    
    // noise

    double low, up;
    for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
    {
	double noise_level = 0.0;
	idx = npmt*the_params.mch_number_of_samples;
	for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime++) 
	{
	    //Noise model from WARP
	    low = -2.0 - (noise_level-5.0)/5.0;
	    up = low+2.0;
	    noise_level += ran.Uniform(low, up);

	    //Gaussian noise model
	    //noise_level = ran.Gaus(0, 1.5);
	    
	    histPMT[idx+itime] += noise_level;
	}
    }
    
  } 

  else 
  {// Use baseline from random triggers in background run
      
      for(chdata = data->channels.begin() ; chdata != data->channels.end(); chdata++)
      {
	  int id = chdata->channel_id;
	  if(id < 0) continue;
	  if(id >= the_params.mch_number_of_channels) continue;
	  int nsamps = chdata->nsamps;
	  if(nsamps < the_params.mch_number_of_samples) 
	  {
	      Message(ERROR)<< "Not enough samples in input file !!!\n";
	      return -1;
	  }
	  idx = id*the_params.mch_number_of_samples;
	  if(s_start < 0) 
	  {
	      s_start_max = nsamps-the_params.mch_number_of_samples;
	      s_start = ran.Uniform(0, s_start_max);
	      s_end = s_start + the_params.mch_number_of_samples;
	  }
	  int ii = 0;
	  
	  for(int isamp=s_start; isamp<s_end; isamp++) 
	  {
	      histPMT[idx+ii] = 4095 - chdata->waveform[isamp];
	      ii++;
	  }
      }
      
  }
  
  // photoelectrons from laser
  for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
  {
      idx = npmt*the_params.mch_number_of_samples;
      barePMT[idx+the_params.mch_trigger_delay] = ran.Poisson(poisson_mean);
  }
  
  double ser_norm;
  

  // Convert photoelectrons (from laser and random hits) to signal
  for(Int_t npmt=0; npmt<the_params.mch_number_of_channels; npmt++) 
  {
      idx = npmt*the_params.mch_number_of_samples;
      for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime++) 
      {
	  if(barePMT[idx+itime] == 0) continue;
	  for(int iphe=0;iphe<barePMT[idx+itime]; iphe++) 
	  {//For each photoelectron:

	      //Determine amplitude of pulse
	      if(ran.Uniform() < P_E) 
	      {    // Exponential tail
		  ser_norm = - TMath::Log(ran.Uniform(0.0, 1.0))*exp_tau;
	      } 
	      else 
	      {   //Gaussian component
		  ser_norm = ran.Gaus(spe_mean, spe_sigma);
	      }
	      ser_norm /= ser_integral;
	     
	      //Get time profile from profile saved in mc/include/averageSPE.hh
	      int ioff,ptime;
	      ioff = xtime_ser - xtime_start;
	      for(int is=xtime_start; is<xtime_end; is++) 
	      {
		  ptime = itime - ioff + is - xtime_start;
		  if(ptime>=0 && ptime<the_params.mch_number_of_samples) 
		      histPMT[idx+ptime] += pmt_ser[is]*ser_norm;
	      }
	  }
      }
  }
  
  unsigned int *bufint = (unsigned int*) buffer;
  unsigned int x1,x2;
  int count;

  count = 0;

  for(Int_t ii=0; ii<the_params.mch_number_of_channels; ii++) 
  {
      idx = ii*the_params.mch_number_of_samples;
      for(Int_t itime = 0; itime<the_params.mch_number_of_samples; itime+=2) 
      {
	  x1 = (unsigned int) (histPMT[idx+itime  ] + 0.5);
	  x2 = (unsigned int) (histPMT[idx+itime+1] + 0.5);
	  if(x1 & 0xFFFFF000) x1 = 0xFFF;
	  if(x2 & 0xFFFFF000) x2 = 0xFFF;
	  
	  x1 = 4095 - x1;
	  x2 = 4095 - x2;
	  
	  bufint[count++] = x1 | (x2 << 16);
      }
  }
  
  mcwriter->Process(MCEventPtr(new MCEvent(raw))); //< writes event to disk
  mc_written++;
  
  return 0;
}

int MCLaserData::Finalize()
{
  //  Message(INFO) << "MCLaserData Finalize \n";

  return 0;
}

int MCLaserData::End_Run()
{
  Message(INFO) << "MCLaserData End \n";

  // RunDB::runinfo *rinfo = mcwriter->GetRunInfo();
  // rinfo->endtime = time(NULL);

  mcwriter->SetSaveConfig(true);

  ParameterList *rinfo = mcwriter->GetRunInfo();

  the_runinfo.runid = the_params.mch_run_number;
  the_runinfo.events = mc_written;

  the_runinfo.type = "laser";

  // RunDB::runinfo::channelinfo *cinfo = rinfo->GetChannelInfo(1);
  // if(cinfo != NULL) std::cout << "found" << std::endl;

  MC_RunInfo::channelinfo cinfo;
  for(int ii=0; ii<the_params.mch_number_of_channels; ii++) {
    cinfo.amplification = 10;
    cinfo.channel = ii;
    cinfo.spe_mean = spe_mean;
    cinfo.spe_sigma = spe_sigma;
    cinfo.reload_cal=false;
    cinfo.reload_cal_run=false;
    cinfo.calibration_channel=ii;
    cinfo.calibration_run=the_params.mch_run_number-1;
    cinfo.voltage=1323;
    the_runinfo.channels.push_back(cinfo);
  }

  the_runinfo.endtime = time(NULL);

  rinfo->RegisterParameter("runinfo", the_runinfo, "");
  rinfo->RegisterParameter("MC_Params", the_params, "");

  mcwriter->Finalize();
  return 0;
}
