/** @file mcoptical.cc
    @brief MC full events from optical simulation
    @author agcocco
*/

#include "Reader.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "EventHandler.hh"
#include "MCOptical.hh"
#include <time.h>
#include <cstdlib>

// #include "TRandom.h"

// static TRandom myran;
static int nevents;
static int read_event;

MCOptical* mcoptical;
MC_Params *mcpar;

/// Fully process a single raw data file
int ProcessOneFile(const char* filename)
{
  Message(INFO)<<"\n***************************************\n"
	       <<"  Processing File "<<filename
	       <<"\n***************************************\n";
  EventHandler* modules = EventHandler::GetInstance();
  modules->AllowDatabaseAccess(false);
  Reader reader(filename);
  if(!reader.IsOk())
    return 1;
  if(modules->Initialize()){
    Message(ERROR)<<"Unable to initialize all modules.\n";
    return 1;
  }
  
  //read through the file and process all events
  time_t start_time = time(0);
  int evtnum;

  //  nev = reader.GetNumEvents();
  //  Message(INFO)<<"Events in file "<< nev << std::endl;

  evtnum = 0;
  while(reader.IsOk() && !reader.eof()){
    if(read_event == nevents) 
      break;
    if(read_event%500 == 0)
      Message(INFO)<<"Processed "<<read_event<< " events " << std::endl;

    //    evtnum = myran.Uniform(1,1000);
    RawEventPtr raw = reader.GetEventWithIndex(evtnum);

    //    RawEventPtr raw = reader.GetNextEvent();
    if(!raw){
      Message(ERROR)<<"Problem encountered reading event "<<evtnum<<std::endl;
      continue;
    }
    Message(DEBUG)<<"Processing event number " << evtnum << " ... ";
    modules->Process(raw);
    Message(DEBUG)<<"done\n";

    evtnum++;
    read_event++;
  }
  //finish up
  modules->Finalize();
  Message(INFO)<<"Processed "<<read_event<<" events in "
	       <<time(0) - start_time<<" seconds. \n";
  return 0;
}

int main(int argc, char** argv)
{

  ConfigHandler* config = ConfigHandler::GetInstance();

  int nrun, nPMT, nsamples, sample, offset, baseline, single_rate, rand_seed;

  nrun = 90000;
  nPMT = 14;
  nsamples = 2048;
  sample = 4;
  offset = 200;
  baseline = 0;
  single_rate = 0;
  rand_seed = 0;

  nevents = 10;

  config->SetProgramUsageString("mcoptical [options] <MCoptical.root> [<baseline_file1>, ... ]");

  config->AddCommandSwitch('n',"nevents","number of events to write in output",
  			   CommandSwitch::DefaultRead<int>(nevents),
  			   "N");
  config->AddCommandSwitch('r',"run","run number",
  			   CommandSwitch::DefaultRead<int>(nrun),
  			   "9xxxx");
  config->AddCommandSwitch('p',"nPMT","number of PMTs",
                           CommandSwitch::DefaultRead<int>(nPMT),
                           "N");
  config->AddCommandSwitch('s',"nsamples","number of samples",
  			   CommandSwitch::DefaultRead<int>(nsamples),
  			   "N");
  config->AddCommandSwitch('z',"sampling","sampling time",
  			   CommandSwitch::DefaultRead<int>(sample),
  			   "ns");
  config->AddCommandSwitch('t',"trigger","trigger delay",
  			   CommandSwitch::DefaultRead<int>(offset),
  			   "samples"); 
  config->AddCommandSwitch('b',"baseline","PMT baseline common offset",
  			   CommandSwitch::DefaultRead<int>(baseline),
  			   "counts");
  config->AddCommandSwitch('x',"ser-rate","PMT SER rate",
  			   CommandSwitch::DefaultRead<int>(single_rate),
  			   "Hz");
  config->AddCommandSwitch('g',"ran-seed","TRandom seed",
  			   CommandSwitch::DefaultRead<int>(rand_seed),
  			   "n");
 
  EventHandler* modules = EventHandler::GetInstance();
  mcoptical = modules->AddModule<MCOptical>();

  config->ProcessCommandLine(argc,argv);
  
  if(argc < 3){
    Message(ERROR)<<"Incorrect number of arguments: "<<argc<<std::endl;
    config->PrintSwitches(true);
    return 1;
  }

  if(nsamples%2) {
    nsamples++;
    Message(WARNING) << " nsamples should be even !! Corrected to " << nsamples << "\n";
  }

  if(nPMT*nsamples > 32768000) {
    Message(ERROR) << " number_of_PMTs x number_of_samples exceded limit (100x32768) !!\n ";
    return -1;
  }

  mcpar = mcoptical->GetParams();

  mcpar->mch_run_number = nrun;
  mcpar->mch_number_of_events = nevents;
  mcpar->mch_number_of_channels = nPMT;
  mcpar->mch_number_of_samples = nsamples;
  mcpar->mch_trigger_delay = offset;
  mcpar->mch_baseline = baseline;
  mcpar->mch_sampling_time = sample;
  mcpar->mch_single_rate = single_rate;

  mcoptical->Set_Filename(argv[1]);

  mcoptical->Start_Run();
  if(rand_seed > 0) mcoptical->Set_Random_Seed(rand_seed);

  read_event = 0;

 again_from_start:
  
  for(int i = 2; i<argc; i++){
    if(ProcessOneFile(argv[i])){
      Message(ERROR)<<"Error processing file "<<argv[i]<<"; quitting\n";
      break;
    }
    if(read_event == nevents) break;
  }

  if(read_event < nevents) goto again_from_start;

  mcoptical->End_Run();

  return 0;
}
