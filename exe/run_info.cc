/** @file run_info.cc
    @brief Main file for run_info executable; give some basic stats about a run
    @author bloer
*/

#include "Reader.hh"
#include "EventHandler.hh"
#include "ConvertData.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "RunDB.hh"
#include "phrase.hh"
#include <iostream>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include "Message.hh"

using namespace std;

/// Print info about a single file
void PrintFileInfo(const char* fname, EventHandler* modules, 
		   const string& runinfo_file="", const string& comment="",
		   char answer=0,
		   bool force_regen = false,
		   bool db_only = false)
{
  cout<<"Processing runinfo for file "<<fname<<"..."<<endl;
  Reader reader(fname);
  if(!reader.IsOk())
    return;
  
 
  RunDB::runinfo* info = modules->GetRunInfo();
  if( ConfigHandler::GetInstance()->LoadParameterList(info) ){
    //couldn't find it in saved config, so see if there's a default file
    if(runinfo_file != ""){
      cout<<"Loading runinfo from file "<<runinfo_file<<endl;
      info->ReadFromFile(runinfo_file.c_str());
    }
  }
    
  if(comment != "") info->comment = comment;  
  if(force_regen || info->runid <=0 || 
     info->starttime == 0 || info->endtime == 0 ||
     info->events == 0 || info->livetime == 0){
    //process the first event
    //don't read from the database
    modules->AllowDatabaseAccess(false);
    modules->Initialize();
    RawEventPtr event = reader.GetEventWithIndex(0);
    if(modules->Process(event)){
      Message(ERROR)<<"Problem processing event from file "<<fname<<endl;
      return;
    }
    //process the last event
    event = reader.GetLastEvent();
    if(modules->Process(event)){
      Message(ERROR)<<"Problem processing event from file "<<fname<<endl;
      return;
    }
    
    //Finalize erases the info, so copy it
    info = new RunDB::runinfo(*info);
    modules->Finalize();
  }
  //print the results
  if(!db_only){
    cout<<"Run information for file "<<fname<<":\n";
    cout<<*info<<endl;
  }
  //ask if we want to insert into the database
  while(answer != 'Y' && answer != 'y' && answer != 'N' && answer != 'n' &&
	answer != 'E' && answer != 'e'){
    if(answer)
      cout<<"'"<<answer<<"' is not a valid response!"<<endl;
    cout<<"Insert this run into the database? ([Y]es/[N]o/[E]dit)"<<endl;
    cin>>answer;
  }
  switch(answer){
  case 'N':
  case 'n':
    break;
  case 'E':
  case 'e':
    {
      char tempname[] = "/tmp/runinfoXXXXXX";
      int error = mkstemp(tempname);
      if(error > 0){
	info->SaveToFile(tempname);
	stringstream cmd;
	cmd<<"emacs "<<tempname;
	error = system(cmd.str().c_str());
	info->ReadFromFile(tempname);
	remove(tempname);
      }
    }
  case 'Y':
  case 'y':
    if(info->comment == ""){
      cout<<"Please enter a comment for this run:"<<endl;
      cin.ignore(100,'\n');
      getline(cin, info->comment);
    }
    info->InsertIntoDatabase();
    break;
  default:
    cerr<<"Something weird happened..."<<endl;
  }
  
    
}
  
int main(int argc, char** argv)
{
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->SetProgramUsageString("run_info [options] <file1> [<file2> ...]");
  string runinfo_file="";
  std::string comment="";
  char query_answer=0;
  bool force_regen = false;
  bool db_only = false;
  config->AddCommandSwitch('i',"info-file", "Read the default run info from <file> if not found in the run cfg file",
			   CommandSwitch::DefaultRead<string>(runinfo_file),
			   "file");
  config->AddCommandSwitch('m',"message",
			   "Set the database comment on this run to <comment>",
			   CommandSwitch::DefaultRead<string>(comment),
			   "comment");
  config->AddCommandSwitch('a',"answer","Skip interactive query for database insertion; value can be 'y', 'n', or 'e'(edit)",
			   CommandSwitch::DefaultRead<char>(query_answer),
			   "answer");
  config->AddCommandSwitch('f',"force","Force regenerate info from raw data",
			   CommandSwitch::SetValue<bool>(force_regen,true));
  config->AddCommandSwitch(' ',"db-only",
			   "Don't print run info, just insert into database",
			   CommandSwitch::SetValue<bool>(db_only,true));
			   
			   
  config->ProcessCommandLine(argc, argv);
  if(config->GetNCommandArgs()==0)
    config->PrintSwitches(true);
  EventHandler* modules = EventHandler::GetInstance();
  modules->AddModule<ConvertData>();
  //cout<<"runid\tstarttime\tendtime\tevents\tlivetime\n";
  for(int i=1; i < argc; i++){
    PrintFileInfo(argv[i], modules, runinfo_file,comment,query_answer,
		  force_regen, db_only);
  }
  return 0;
}
