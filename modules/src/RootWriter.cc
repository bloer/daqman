#include "Message.hh"
#include "RootWriter.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "TClass.h"
#include "TDataMember.h"
#include "TROOT.h"
#include "TMacro.h"

#include "EventHandler.hh"

#include "TFile.h"
#include "TTree.h"
#include <string>
#include <sstream>


RootWriter::RootWriter() : 
  BaseModule("RootWriter","Save processed data into a ROOT tree"), 
  _filename(), _mode(), _outfile(0), _tree(0), 
  enabler(this), disabler(this)
{
  //default initialize filename
  /*
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo;
  timeinfo = localtime(&rawtime);
  char fbuf[40];
  strftime(fbuf,40,"processed_%y%m%d_%H%M.root",timeinfo);
  _filename = fbuf;
  */
  _filename = GetDefaultFilename();
  RegisterParameter("filename",_filename, 
		    "Name of the output root filename");
  RegisterParameter("directory",_directory=".",
		    "Directory in which to place the output root file");
  RegisterParameter("mode",_mode="RECREATE",
		    "Mode in which to create the rootfile (see TFile)");
  RegisterParameter("default_saveall", default_saveall = false);
  RegisterParameter("enable_branch" , enabler,
		    "Allows the user to enable writing a certain branch");
  RegisterParameter("disable_branch", disabler,
		    "Allows the user to disable writing a certain branch");
  ConfigHandler* config = ConfigHandler::GetInstance();
  config->
    AddCommandSwitch(' ',"rootfile","Set output ROOT filename to <file>",
		     CommandSwitch::DefaultRead<std::string>(_filename),
		     "file");
  config->AddCommandSwitch(' ',"rootdir","Set output ROOT directory to <dir>",
			   CommandSwitch::DefaultRead<std::string>(_directory),
			   "dir");
  
}

RootWriter::~RootWriter()
{
  Finalize();
  if(_outfile){
    _outfile->Close();
    delete _outfile;
    _outfile = 0;
  }
}

int RootWriter::Initialize()
{
  //append the .root suffix if necessary
  if( _filename.find(".root") == std::string::npos)
    _filename.append(".root");
  //add the directory prefix if not specified
  if( _filename.find("/") == std::string::npos)
    _filename.insert(0, _directory + "/");
  
  Message(INFO)<<"Saving output to file "<<_filename<<std::endl;
  _outfile = new TFile(_filename.c_str(), _mode.c_str());
  if(!_outfile || !_outfile->IsOpen() || _outfile->IsZombie()){
    Message(ERROR)<<"Unable to open ROOT file for writing.\n";
    enabled = false;
    return 1;
  }
  _tree = new TTree("Events","Processed data for each event");
  EventData* ptr = new EventData;
  _tree->Branch(EventData::GetBranchName(),&ptr);
  delete ptr;
  runinfo* rinfo = EventHandler::GetInstance()->GetRunInfo();
  if(rinfo){
    _tree->GetUserInfo()->AddFirst(rinfo);
  }
  SaveConfig();
  return 0;
}

void RootWriter::SaveConfig()
{
  ConfigHandler* cfghandler = ConfigHandler::GetInstance();
  //save the configuration to the tree as a TMacro
  TMacro cfg;
  std::stringstream cfgstream;
  cfgstream << *cfghandler; 
  cfgstream.seekg(std::ios::beg);
  std::string line;
  while( std::getline(cfgstream, line) ) 
    cfg.AddLine(line.c_str());
  cfg.Write("Configuration");
  //also save the daq configuration
  if(cfghandler->GetSavedCfgFile() != ""){
    TMacro daqcfg(cfghandler->GetSavedCfgFile().c_str());
    if(daqcfg.GetListOfLines()->GetEntries())
      daqcfg.Write("DAQConfiguration");
  }
    
}

int RootWriter::Process(EventPtr event)
{
  EventDataPtr data = event->GetEventData();
  EventData* ptr = data.get();
  _tree->SetBranchAddress(EventData::GetBranchName(), &ptr );
  _tree->Fill();
  //_tree->Write();
  return 0;
}

TTree* RootWriter::BuildMetadataTree(runinfo* info)
{
  TTree* tree = new TTree("metadata","Metadata associated with this run");
  int run_id = info->runid;
  tree->Branch("run_id",&run_id);
  tree->Fill();
  
  //make two branches for each metadata: 1 string and 1 double
  typedef runinfo::stringmap smap;
  smap& meta = info->metadata;
  for(smap::iterator it = meta.begin(); it != meta.end(); ++it){
    TBranch* b = tree->Branch(it->first.c_str(), &(it->second));
    b->Fill();
    double val = std::atof(it->second.c_str());
    b = tree->Branch((it->first + "_d").c_str(), &val);
    b->Fill();
  }
  return tree;
}

int RootWriter::Finalize()
{
  if(_tree){
    _tree->SetEntries();
    if(_tree->GetEntries()>0 && _outfile && _outfile->IsOpen()){
      Message(INFO)<<"Constructing tree index...\n";
      _tree->BuildIndex("run_id","event_id");
      runinfo* info = (runinfo*)_tree->GetUserInfo()->At(0);
      if(info){
	TTree* friendtree = BuildMetadataTree(info);
	_tree->AddFriend(friendtree);
	friendtree->Write();
	delete friendtree;
      }
      _tree->Write();
    }
    _tree->GetUserInfo()->Clear();
    delete _tree;
    _tree = 0;
  }
  if(_outfile){
    //save config again to get changes
    SaveConfig();
    _outfile->Close();
    delete _outfile;
    _outfile = 0;
  }
  return 0;
}
    
void RootWriter::EnableBranch(const char* classname, const char* branchname,
			      bool enable)
{
  TClass* cl = gROOT->GetClass(classname);
  if(!cl){
    Message(ERROR)<<"ROOT doesn't know about any class "<<classname<<"\n";
    return;
  }
  TDataMember* mem = cl->GetDataMember(branchname);
  if(!mem){
    Message(ERROR)<<"Class "<<classname<<" has no member named "
		  <<branchname<<"\n";
    return;
  }
  mem->SetBit(TObject::kWriteDelete, enable);
  
}

void RootWriter::DisableBranch(const char* classname, const char* branchname) 
{
  EnableBranch(classname, branchname, false);
}
