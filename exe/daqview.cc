/** @file daqview.cc
    @brief Main file for daqview, displays a single event from raw data
    @author bloer
*/

#include "Reader.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "V172X_Params.hh"
#include "ProcessedPlotter.hh"
#include "SumChannels.hh"
#include "V172X_Event.hh"
#include "EventHandler.hh"
#include "Message.hh"
#include "RootGraphix.hh"
#include "TFile.h"
#include "TCanvas.h"
#include <fstream>
#include <string>
#include <exception>
#include <stdlib.h>
 
using namespace std;
/// Allows user to save a displayed canvas to a ROOT file
void SaveCanvas(const TCanvas* c, const char* name)
{
  TFile fout("daqview.root","UPDATE");
  if(!fout.IsOpen() || fout.IsZombie()){
    Message(ERROR)<<"Unable to open file daqview.root to save canvas!\n";
  }
  if(c)
    c->Write(name);
}

int main(int argc, char** argv)
{
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->SetProgramUsageString("daqview [options] <filename> [<event>]");
  EventHandler* modules = EventHandler::GetInstance();
  modules->AddCommonModules();
  //add rootgraphix first to pass dependencies, but process afterward
  RootGraphix* rootgraphix = new RootGraphix;
  modules->AddModule(rootgraphix, /*process*/ false, /*register*/ true);
  ProcessedPlotter* plotter = new ProcessedPlotter;
  modules->AddModule(plotter);
  modules->AddModule(rootgraphix, /*process*/ true, /*register*/ false);
  
  
  string list_filename="";
  config->AddCommandSwitch('l',"event-list",
			   "Read the events to display from <file>",
			   CommandSwitch::DefaultRead<string>(list_filename),
			   "file");
  int printlevel;
  config->RegisterParameter("printlevel",printlevel = 1,
			    "Event print verbosity (0=NONE to 3=All info)");
  config->AddCommandSwitch(' ',"printlevel",
			   "Event print verbosity (0=NONE to 3=All info)",
			   CommandSwitch::DefaultRead<int>(printlevel),
			   "level");
  
  
  config->SetDefaultCfgFile("cfg/daqview.cfg");
  if(config->ProcessCommandLine(argc,argv))
    return -1;
  if(argc != 3 && argc != 2){
    Message(ERROR)<<"Incorrect number of arguments: "<<argc<<std::endl;
    config->PrintSwitches(true);
  }
  ifstream eventlist;
  bool use_elist = false;
  if(list_filename != ""){
    eventlist.open(list_filename.c_str());
    if(!eventlist.is_open()){
      Message(ERROR)<<"Unable to open event-list file "<<list_filename<<endl;
      return -1;
    }
    use_elist = true;
  }
  
    
  std::string fname(argv[1]);
  Reader reader(fname.c_str());
  if(!reader.IsOk())
    return 1;
  int next_evt = 0;
  if (argc == 3 ) next_evt = atoi(argv[2]);
  else if(use_elist) eventlist>>next_evt;
  if(modules->Initialize()){
    Message(ERROR)<<"Unable to initialize all modules.\n";
    return 1;
  }
  while(reader.IsOk() && !reader.eof()){
    RawEventPtr raw = reader.GetEventWithID(next_evt);
    if(!raw){
      if(!reader.eof())
	Message(ERROR)<<"Problem encountered reading event "<<next_evt<<"\n";
      else
	Message(INFO)<<"Reached end of file series.\n";
      break;
    }
    else{
      Message(DEBUG)<<"Processing event...\n";
      modules->Process(raw);
      if( plotter->GetLastProcessReturn()){
	//the plotter failed some cut; try the next one
	next_evt++;
	continue;
      }
      Message(DEBUG)<<"Done!\n";
      Message(DEBUG)<<"Current event has file index "<<reader.GetCurrentIndex()
		   <<" and ID "<<raw->GetID()<<"\n";
      modules->GetCurrentEvent()->GetEventData()->Print(printlevel);
      
    }
    int this_evt = next_evt;
    if(use_elist){
      if( !(eventlist>>next_evt)){
	Message(INFO)<<"Reached end event-list file.\n";
	next_evt = this_evt+1;
      }
    }
    else
      next_evt = this_evt+1;
    ///@todo If you close a window, there is a segmentation violation when you attempt to quit
    Message(INFO)<<"Enter the next event to view; <enter> for "<<next_evt
		 <<", s to save this canvas, q or -1 to quit\n";
    std::string line;
    getline(std::cin, line);
    if( line == "" )
      {}
    else if(line == "q" || line == "Q" || line == "-1"){
      Message(DEBUG)<<"Quitting\n";
      break;
    }
    else if(line == "s" || line == "S"){
      Message(INFO)<<"Saving canvas to file daqview.root.\n";
      char name[30];
      sprintf(name, "Event%d_1",next_evt);
      SaveCanvas(plotter->GetCanvas(0),name);
      sprintf(name, "Event%d_2",next_evt);
      SaveCanvas(plotter->GetCanvas(1),name);
    } 
    else{
      next_evt = atoi(line.c_str());
    }
  }
  modules->Finalize();
  return 0;
}
