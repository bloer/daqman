/** @file genroot.cc
    @brief Main file for genroot, creates ROOT tree from raw data file
    @author bloer
*/

#include "Reader.hh"
#include "RawWriter.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "EventHandler.hh"
#include <time.h>
#include <cstdlib>


/// Determine the filename of the output file
void SetOutputFile(RawWriter* writer, const char* inputfile){
 if( writer->GetFilename() == writer->GetDefaultFilename() ){
    //set the filename to be the input filename + _v1
    std::string fname(inputfile);
    //remove leading directories
    while(fname.find('/') != std::string::npos){
      fname = fname.substr(fname.find('/')+1);
    }
    //remove filename suffix
    fname = fname.substr(0, fname.find('.'));
    //append the new filename
    fname.append(".out");
    writer->SetFilename(fname);
  }
}

/// Fully process a single raw data file
int ProcessOneFile(const char* filename, int max_event=-1, int min_event=0)
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
  int evtnum = min_event;
  while(reader.IsOk() && !reader.eof()){
    if(max_event > 0 && evtnum >= max_event) 
      break;
    //Message(DEBUG)<<"*************Event "<<evtnum<<"**************\n";
    if(evtnum%1000 == 0)
      Message(INFO)<<"Processing event "<<evtnum<<std::endl;
    
    RawEventPtr raw = reader.GetEventWithIndex(evtnum++);
    if(!raw){
      Message(ERROR)<<"Problem encountered reading event "<<evtnum<<std::endl;
    }
    modules->Process(raw);
    
  }
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
  config->SetProgramUsageString("updatefile [options] <file1> [<file2>, ... ]");
  config->AddCommandSwitch(' ',"max","last event to process",
			   CommandSwitch::DefaultRead<int>(max_event),
			   "event");
  config->AddCommandSwitch(' ',"min","first event to process",
			   CommandSwitch::DefaultRead<int>(min_event),
			   "event");
  
  EventHandler* modules = EventHandler::GetInstance();
  RawWriter* writer = modules->AddModule<RawWriter>();
  //disable saving the config file by default
  //writer->SetSaveConfig(false);
  config->ProcessCommandLine(argc,argv);
  
  if(argc < 2){
    Message(ERROR)<<"Incorrect number of arguments: "<<argc<<std::endl;
    config->PrintSwitches(true);
  }
  
  for(int i = 1; i<argc; i++){
    if(i > 1)
      writer->SetFilename(writer->GetDefaultFilename());
    SetOutputFile(writer, argv[i] );
    if(ProcessOneFile(argv[i], max_event, min_event)){
      Message(ERROR)<<"Error processing file "<<argv[i]<<"; aborting.\n";
      return 1;
    }
  }
  return 0;
}
