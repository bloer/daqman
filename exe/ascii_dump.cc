/** @file ascii_dump.cc
    @brief main file for ascii_dump, which writes pulses to text files
    @author bloer
*/

#include "Reader.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "EventHandler.hh"
#include "AsciiWriter.hh"
#include "ConvertData.hh"

#include <cstdlib>
#include <string>


void SetOutputFile(AsciiWriter* writer, const char* inputfile){
  if( writer->GetFilename() == writer->GetDefaultFilename() ){
     //set the filename to be the input filename + .txt
    std::string fname(inputfile);
    //remove a possible trailing slash in case of a directory argument
    while(fname.length()>0 && fname[fname.length()-1] =='/' ){
      fname.resize(fname.length()-1);
    }
    //remove leading directories
    while(fname.find('/') != std::string::npos){
      fname = fname.substr(fname.find('/')+1);
    }
    //remove filename suffix
    fname = fname.substr(0, fname.find('.'));
    //append the root filename
    fname.append(".txt");
    //make sure we haven't made an empty string here!
    if(fname!=".txt")
      writer->SetFilename(fname);
  }
}

int ProcessOneFile(const char* filename, int max_event=-1, int min_event=0)
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
  int evtnum = min_event;
  while(reader.IsOk() && !reader.eof()){
    if(max_event > 0 && evtnum >= max_event) 
      break;
    Message(DEBUG)<<"*************Event "<<evtnum<<"**************\n";
    if(evtnum%5000 == 0)
      Message(INFO)<<"Processing event "<<evtnum<<std::endl;
    
    RawEventPtr raw = reader.GetEventWithIndex(evtnum++);
    if(!raw){
      Message(ERROR)<<"Problem encountered reading event "<<evtnum<<std::endl;
    }
    modules->Process(raw);
    
  }
  //finish up
  modules->Finalize();
  Message(INFO)<<"Processed "<<evtnum - min_event<<" events in "
	       <<time(0) - start_time<<" seconds. \n";
  return 0;
}

int main(int argc, char** argv)
{
  int max_event=-1, min_event = 0;
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->SetProgramUsageString("ascii_dum [<options>] <rawdata> [<rawdata2>...]");
  config->AddCommandSwitch(' ',"max","last event to process",
			   CommandSwitch::DefaultRead<int>(max_event),
			   "event");
  config->AddCommandSwitch(' ',"min","first event to process",
			   CommandSwitch::DefaultRead<int>(min_event),
			   "event");
  
  EventHandler* modules = EventHandler::GetInstance();
  //modules->AddCommonModules();
  modules->AddModule<ConvertData>();
  AsciiWriter* writer = modules->AddModule<AsciiWriter>();

  try{
    config->ProcessCommandLine(argc,argv);
  }
  catch(std::exception& e){
    Message(EXCEPTION)<<"While processing command line: "<<e.what()<<std::endl;
    config->PrintSwitches(true);
  }

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
