/** @file genroot.cc
    @brief Main file for genroot, creates ROOT tree from raw data file
    @author bloer
*/

#include "Reader.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "EventHandler.hh"
#include "AverageWaveforms.hh"
#include <time.h>
#include "V172X_Params.hh"
#include "SumChannels.hh"
#include "BaselineFinder.hh"
#include "Differentiator.hh"
#include "FParameter.hh"
#include "Fitter.hh"
#include "SpeFinder.hh"
#include "PulseFinder.hh"
#include "RootWriter.hh"
#include "ConvertData.hh"
#include <cstdlib>



/// Determine the filename of the output root file
void SetOutputFile(RootWriter* writer, const char* inputfile){
 if( writer->GetFilename() == writer->GetDefaultFilename() ){
    //set the filename to be the input filename + .root
    std::string fname(inputfile);
    //remove any trailing slashes if this is a directory
    while(!fname.empty() && *(fname.rbegin()) == '/')
      fname.resize(fname.size()-1);
    //remove leading directories
    if(fname.rfind('/') != std::string::npos){
      fname = fname.substr(fname.rfind('/')+1);
    }
    //remove filename suffix
    fname = fname.substr(0, fname.find('.'));
    //append the root filename
    fname.append(".root");
    writer->SetFilename(fname);
  }
}

/// Fully process a single raw data file
int ProcessOneFile(const char* filename, std::string event_file, int max_event=-1, int min_event=0)
{
  Message(INFO)<<"\n***************************************\n"
	       <<"  Processing File "<<filename
	       <<"\n***************************************\n";
  EventHandler* modules = EventHandler::GetInstance();
  Reader reader(filename);
  if(modules->Initialize()){
    Message(ERROR)<<"Unable to initialize all modules.\n";
    return 1;
  }
  
  //read through the file and process all events
  time_t start_time = time(0);
  RawEventPtr raw;
  
  //if the first event is not 0, read it to get start of run info
  if(min_event != 0){
    ConvertData* converter = modules->GetModule<ConvertData>();
    if(converter){ 
      raw = reader.GetEventWithID(0);
      if(!raw){
	Message(ERROR)<<"Problem encountered reading event "<<min_event
		      <<std::endl;
	return 1;
      }
      EventPtr evt(new Event(raw));
      converter->Process(evt);
      //if the first event is not 1, read the event immediately before to get dt
      if(min_event > 1){
	raw = reader.GetEventWithID(min_event-1);
	if(!raw){
	  Message(ERROR)<<"Problem encountered reading event "<<min_event
			<<std::endl;
	  return 1;
	}
	evt.reset(new Event(raw));
	converter->Process(evt);
      }
    }
  }
  
  //Get the first event by ID
  raw = reader.GetEventWithID(min_event);


  //if using event list, open the event list file
  std::ifstream eventlist;
  bool use_elist = false;
  if (event_file!="") {
    eventlist.open(event_file.c_str());
    if (!eventlist.is_open()){
      Message(ERROR)<<"Unable to open event-list file "<<event_file<<endl;
      return 1;
    }
    use_elist = true;
  }
  if (use_elist) {
    //find first event in list greater than min_event
    int tmp_event;
    eventlist>>tmp_event;
    while (min_event > tmp_event && !eventlist.eof())
      eventlist>>tmp_event;
    if (eventlist.eof()) {
      Message(INFO)<<"No events to process!"<<std::endl;
      return 1;
    }
    min_event = tmp_event;
    raw = reader.GetEventWithID(min_event);
  }

  if(!raw){
    Message(ERROR)<<"Problem encountered reading event "<<min_event<<std::endl;
    return 1;
  }
  int evtnum = 0;
  while(raw){
    if(max_event > 0 && raw->GetID() >= (uint32_t)max_event) 
      break;
    //Message(DEBUG)<<"*************Event "<<evtnum<<"**************\n";
    if(evtnum%5000 == 0 && !use_elist)
	Message(INFO)<<"Processing event "<<raw->GetID()<<std::endl;
    else if (use_elist)
        Message(INFO)<<"Processing event "<<raw->GetID()<<std::endl;
    ++evtnum;
    if(modules->Process(raw)){
      if(raw)
	Message(ERROR)<<"Error processing event "<<raw->GetID()<<"\n";
      break;
    }

    //if using event list, read next event number to process
    if (use_elist) {
      int next_event;
      if (eventlist>>next_event) {
        raw = reader.GetEventWithID(next_event);
      }
      else {
        Message(INFO)<<"Reached end event-list file.\n";
        break;
      }
    }
    else {
      raw = reader.GetNextEvent();
    }
  }
  eventlist.close();

  //finish up
  modules->Finalize();
  Message(INFO)<<"Processed "<<evtnum<<" events in "
	       <<time(0) - start_time<<" seconds. \n";
  return 0;
}

int main(int argc, char** argv)
{
  int max_event=-1, min_event = 0;
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->SetProgramUsageString("genroot [options] <file1> [<file2>, ... ]");
  config->AddCommandSwitch(' ',"max","last event to process",
			   CommandSwitch::DefaultRead<int>(max_event),
			   "event");
  config->AddCommandSwitch(' ',"min","first event to process",
			   CommandSwitch::DefaultRead<int>(min_event),
			   "event");
  std::string event_file = "";
  config->AddCommandSwitch(' ',"event-list","read events to process from <file>",
                           CommandSwitch::DefaultRead<std::string>(event_file),
                           "file");
  
  EventHandler* modules = EventHandler::GetInstance();
  modules->AddCommonModules();
  modules->AddModule<AverageWaveforms>();
  //modules->AddModule<GenericAnalysis>();
  RootWriter* writer = modules->AddModule<RootWriter>();
  
  config->SetDefaultCfgFile("genroot.cfg");
  if(config->ProcessCommandLine(argc,argv))
    return -1;

  if(argc < 2){
    Message(ERROR)<<"Incorrect number of arguments: "<<argc<<std::endl;
    config->PrintSwitches(true);
  }
  
  for(int i = 1; i<argc; i++){
    if(i > 1)
      writer->SetFilename(writer->GetDefaultFilename());
    SetOutputFile(writer, argv[i] );
    if(ProcessOneFile(argv[i], event_file, max_event, min_event)){
      Message(ERROR)<<"Error processing file "<<argv[i]<<"; aborting.\n";
      return 1;
    }
  }
  return 0;
}
