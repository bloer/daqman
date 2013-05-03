#include "FitOverROI.hh"
#include "FitTH1F.hh"
#include "TNamed.h"
#include "TString.h"
#include "TCanvas.h"
#include "EventData.hh"
#include "TRint.h"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "utilities.hh"
#include "RunDB.hh"
#include "RootGraphix.hh"
#include "ChanFitSettings.hh"
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "ParameterList.hh"
#include "TTree.h"
#include "TFile.h"
#include "TF1.h"

#define mNCHANS 14

using namespace std;
using namespace FitOverROI;


class tab{
public:
  int n;
  char f;
  tab(int nspaces, char fill=' ') : n(nspaces),f(fill) {}
};

ostream& operator<<(ostream& out, tab t)
{ return out<<left<<setfill(t.f)<<setw(t.n); }

void PrintResults(std::map<int, FitTH1F*>* spectra)
{
	
  cout<<"\n"<<tab(55,'*')<<""<<endl;
  cout<<"Current results are: "<<endl;
  cout<<tab(10)<<"Channel"<<tab(11)<<"spe_mean"<<tab(12)<<"spe_sigma";
  cout<<tab(11)<<"lambda"<<tab(11)<<"amp_E"<<tab(12)<<"p_E"<<tab(12)<<"Chi2/NDF";
  cout<<tab(6)<<"|||"<<tab(10)<<"pdfmean"<<tab(11)<<"pdfsigma"<<"pdfmean_err";
  cout<<endl;
  cout<<tab(104,'-')<<""<<endl;
  map<int, FitTH1F*>::iterator it = spectra->begin();
  for( ; it != spectra->end(); it++){
    cout<<tab(10)<<it->first;
    TF1* fit = (it->second)->GetFunction("spefunc");
    if(!fit){
      cout<<endl;
      continue;
    }
	
	
	double* params=fit->GetParameters();
	//double sigma_1=sqrt(params[FitOverROI::SHOTNOISE]*params[SHOTNOISE] + params[SIGMA]*params[SIGMA]);
	//double pdfmean=(1-params[FitOverROI::P_E])*(params[MEAN]+ sigma_1/(sqrt(2 * TMath::Pi()) * 0.5 *(1- TMath::Erf(-params[MEAN]/(sigma_1*sqrt(2))))) *TMath::Power(TMath::E(),-0.5 *params[MEAN]*params[MEAN]/(sigma_1 * sigma_1))) + params[P_E] * params[AMP_E];
    cout<<tab(11)<<params[MEAN]<<tab(12)<<params[SIGMA];
    cout<<tab(11)<<params[LAMBDA];
	cout<<tab(11)<<params[AMP_E];
	cout<<tab(12)<<params[P_E];
    stringstream s;
    s<<fit->GetChisquare()<<"/"<<fit->GetNDF();
    cout<<tab(12)<<s.str();
    cout<<tab(6)<<"|||";
	cout<<tab(10)<<m_n(params);
    //afan------------
    //double pdfmean_approx = params[P_E]*params[AMP_E]+(1-params[P_E])*params[MEAN];
    cout<<tab(11)<<sigma_n(params);
    double pdfmean_error = FitOverROI::pdfmean_error_corr((it->second)->fitResult);
    cout <<tab(10)<<pdfmean_error;
    
    //----------------
	
	
	//cout<<tab(10);
    if(fit->GetProb() < 0.01)
      cout<<"!";
    if(fit->GetProb() < 0.001)
      cout<<"!";
    cout<<endl;
  }
  cout<<tab(55,'*')<<""<<endl;
}

void DrawSpectra(map<int, FitTH1F*>* spectra, TCanvas* c)
{
  c->Clear();
  DividePad(c,spectra->size());
  int padn=1;
  for( std::map<int,FitTH1F*>::iterator it = spectra->begin();
       it != spectra->end(); it++){
    //RootGraphix::Lock lock = root.AcquireLock();
    c->cd(padn++);
    gPad->SetLogy();
    TH1* hist = (it->second);
    hist->Draw();
  }
  c->cd(0);    
  c->Draw();
  c->Update();
}

void UpdateDatabase(map<int,FitTH1F*>* spectra, TTree* Events, ChanFitSettings ChannelSettings[])
{
  const string table="laserruns";
  //find the runid
  EventData* ev = 0;
  Events->SetBranchAddress("event",&ev);
  Events->GetEntry(0);
  

  
  if(ev->run_id < 0){
    std::cerr<<"Unable to read runid from rootfile! Aborting."<<std::endl;
    return;
  }
  
  cout<<"How many filters were used in this run? "<<endl;
  int nfilters;
  cin>>nfilters;
  std::vector<RunDB::laserinfo> vec;
for( map<int,FitTH1F*>::iterator it = spectra->begin();
it != spectra->end(); it++){
	Events->GetEntry(0);
	for(int i =1;(ev->GetChannelByID(it->first)->regions.size() < 1) && (i<Events->GetEntries());i++)
	{  
		Events->GetEntry(i);
	}
	if(ev->GetChannelByID(it->first)->regions.size() < 1){
		std::cout<<"no regions found anywhere in tree!"<<endl;
		throw std::runtime_error("No regions found in tree");
	}
	
	TF1* spefunc = (it->second)->GetFunction("spefunc");
	if(!spefunc){
		cerr<<"Unable to get spefunc from channel "<<it->first<<endl;
		continue;
	}
	std::stringstream CFSss;
	ChannelSettings[it->first].WriteTo(CFSss);
	std::string CFSstring = CFSss.str();
	FitTH1F* hist = it->second;
	
	int Runid = ev->run_id;
	int Channel = it->first;
	time_t Runtime=ev->timestamp;
	
	double Roi_start = ev->GetChannelByID(it->first)->regions[0].start_time;
		//	    double Roi_start, double Roi_end, FitTH1F* h, std::string Fitsettings
	
    vec.push_back(RunDB::laserinfo(Runid, 
				   Channel,  
				   Runtime,
				   nfilters,
				   Roi_start,
				   ev->GetChannelByID(it->first)
				     ->regions[0].end_time,
				   hist, CFSstring) )
				   ; 
    
  }
  int rows = InsertLaserInfo(vec);
  std::cout<<rows<<" entries were successfully inserted into the database."
	   <<std::endl;
}

void RefitChannel(FitTH1F* h, int entries, TCanvas* c, ChanFitSettings& CFS)
{
  c->Clear();
  h->Draw();
  string response="";

/*   TNamed* options = (TNamed*)h->FindObject("options");
  if(!options){
    options = new TNamed("options","");
    options->SetBit(TObject::kCanDelete, true);
    h->GetListOfFunctions()->Add(options); 
  }*/
  while( response[0] != 'r' && response[0] != 'f'
	 && response[0] != 's' &&response[0] != 'd'){
    if(response != "")
      cerr<<response<<" is not a valid option!"<<endl;
    cout<<"Would you like to \n"
	<<" r) Rebin the histogram\n"
	<<" s) Suppress the background exponential in the fit\n"
	<<" f) fix the fit by altering the config\n"
	<<" d) refit with the default method\n"
	<<endl;
    cin >> response;
  }
/*   std::string optstring = options->GetTitle();
  optstring += response;
  options->SetTitle(optstring.c_str()); */
  

  bool allow_bg = true;
  bool force_old = false;
  if(response.find('d') != string::npos){}
  if(response.find('r') != string::npos){
    int lastbin = h->GetXaxis()->GetLast();
    h->Rebin();
    h->GetXaxis()->SetRange(0,lastbin/2);
  }
 
  if(response.find('s') != string::npos){
    allow_bg = false;
  }
  if(response.find('f') != string::npos){

	char temppath[]="/tmp/fileXXXXXX";
	int test = mkstemp(temppath);
	if(test == -1){
	  Message(ERROR)<<"Unable to open temp file!\n";
	  return;
	}
	fstream tempfile(temppath, fstream::in | fstream::out | fstream::app);
	CFS.WriteTo(tempfile);
	tempfile<<endl<<"# Parameter meanings:"<<endl;
	tempfile<<"# amp_E: Decay constant of SPE exponential"<<endl;
	tempfile<<"# constant: Normalization to number of events"<<endl;
	tempfile<<"# lambda: Occupancy"<<endl;
	tempfile<<"# mean: SPE gaussian center"<<endl;
	tempfile<<"# p_E: Proportion of total SPE distribution in the exponential"<<endl;
	tempfile<<"# pedmean: Pedestal gaussian center"<<endl;
	tempfile<<"# pedrange: Range looked at to find pedestal center."<<endl;
	tempfile<<"# range: Histogram x-axis range"<<endl;
	tempfile<<"# shotnoise: Pedestal width"<<endl;
	tempfile<<"# sigma: SPE gaussian width"<<endl;
	tempfile.clear();
	tempfile.close();
	std::stringstream command;
    command << "emacs "<<temppath;
    if( system(command.str().c_str()) ){
      std::cerr<<"Unable to open config for editing."<<std::endl;
    }
	else{
		
		tempfile.open(temppath, fstream::in);
		CFS.ReadFrom(tempfile);
		cout<<"Config File read back."<<endl;
		CFS.WriteTo(cout);
		cout<<endl;
	}
	tempfile.close();
	remove(temppath);
  }
  FitSPE(h, CFS,entries,allow_bg, force_old);
  //FitSPE(h,entries,min,max,allow_bg);
}    
  

void QueryUser(map<int, FitTH1F*>* spectra, TTree* Events, RootGraphix* root, ChanFitSettings ChannelSettings[], 
	       TCanvas* c = 0)
{
  if(!c)
    c = new TCanvas;
  bool showresults=true;
  string response = "";
  while(response != "q"){
    if(showresults){
      RootGraphix::Lock lock = root->AcquireLock();
      DrawSpectra(spectra, c);
      PrintResults(spectra);
    }
    showresults=false;
    cout<<"Enter \n q) to quit, \n"
	<<" #) to retry fitting channel # \n"
	<<" r#) to remove channel # from the list\n"
	<<" w) to write the results to the database."<<endl;
    cin >> response;
    if(response == "q"){
      cout<<"Aborting without saving results."<<endl;
      break;
    }
    if(response == "w"){
      UpdateDatabase(spectra, Events, ChannelSettings);
      break;
    }
    bool remove = false;
    if( response[0] == 'r'){
      remove = true;
      response.erase(0,1);
    }
    int chan = atoi(response.c_str());
    if( chan == 0 && response != "0"){
      //user entered something weird
      cout<<"'"<<response<<"' is not a valid response!"<<endl;
      continue;
    }
    //ok, we should have a number in there
    //find the relevant channel
    map<int, FitTH1F*>::iterator it = spectra->find(chan);
    if( it == spectra->end()){
      cout<<chan<<" is not a valid channel!"<<endl;
      continue;
    }
    showresults = true;
    if(remove){
      (it->second)->Delete();
      spectra->erase(it);
      continue;
    }
    else{
      RootGraphix::Lock lock = root->AcquireLock();
      FitTH1F* h = (it->second);
      RefitChannel(h,Events->GetEntries(),c,ChannelSettings[it->first]);
      showresults=true;
      c->Update();
      continue;
    }
  } 
  if(c) delete c;
}

int ProcessLaserRun(const char* fname, unsigned region=0,bool rehist=false,bool min=false)
{
	ParameterList* ChanSettingsHandler = new ParameterList("ChannelsSettings","Stores 8 channels of fit settings");
	ChanFitSettings ChannelsSettings[ mNCHANS ];
	
	
	for(int j=0; j< mNCHANS; j++){
		stringstream name;
		stringstream help;
		name<<"chan"<<j;
		help<<"Fit settings for channel "<<j;
		ChanSettingsHandler->RegisterParameter(name.str(),ChannelsSettings[j], help.str());
		}
	ifstream CFSConfig("cfg/LaserCFSConfig.cfg");
	ChanSettingsHandler->ReadFrom(CFSConfig);
	CFSConfig.close();
	
  const int nbins=315;
  const double start=-30, end=600; 
  std::map<int,FitTH1F*> spectra;
  std::map<int,FitTH1F*> spectra2; //historgram of amplitude (min)
  RootGraphix root;
  root.Initialize();
  TFile *f= OpenFile(fname,"UPDATE");
  if(!f){cout<<"File not opened!"<<endl;}
  TTree* Events = GetEventsTree(fname);
  if(!Events) {cout<< "!Events"<<endl; return -1;}

	
  EventData* event=0;
  Events->SetBranchAddress("event",&event);
  Events->GetEntry(0);
  cout<<"There are "<<Events->GetEntries()<<" entries in this tree."<<endl;

   //check for cached histograms
	bool found_all=true;

	//if(event->channels.size() > 10){cout<< "channels.size() >10"<<endl;    return -2;}
	if(!rehist){
		for(size_t j=0; j < event->channels.size(); j++){
			TString name = "channel";
			ChannelData* channel = &(event->channels.at(j));
			name += channel->channel_id;
			FitTH1F *histo= new FitTH1F();
			int read= histo->Read(name);
			cout<<"readvar" << read<<endl;
			if (read==0) { found_all = false; delete histo; break; }

			if(found_all){
				cout<<"Loaded cached histogram "<<name<<endl;
				std::map<int,FitTH1F*>::iterator mapit = spectra.find(channel->channel_id);

				if(mapit == spectra.end()){
					spectra.insert( std::make_pair(channel->channel_id, histo) );	  }
				  else{
					histo = (mapit->second);
				  }
				}
		}
	}
  //Create histograms of amplitude (min) (by Xiaoyang)
	if(min){
     for(int i=0; i < Events->GetEntries(); i++){
		if(i%5000 == 0) cout<<"Processing Entry "<<i<<endl;
		
			Events->GetEntry(i);
			//iterate over all channels
			
			
			for(size_t j=0; j < event->channels.size(); j++){
			  ChannelData* channel = &(event->channels.at(j));
			  if(channel->channel_id < 0){
				cout<<"channel->channel_id < 0"<<endl;
				continue;
				}
			  std::map<int,FitTH1F*>::iterator mapit = spectra2.find(channel->channel_id);
			  FitTH1F* histo=0;
			  if(mapit == spectra2.end()){
				TString name = "channel";
				name += channel->channel_id;
				cout<<"Creating new histogram with name "<<name<<endl;
				histo = new FitTH1F(name,name,nbins,start,end);
				spectra2.insert( std::make_pair(channel->channel_id, histo) );
			  }
			  else{
				histo = (mapit->second);
			  }
		
			  //fill the histogram
			  if(!histo){
			cerr<<"Null pointer passed!\n";
			return -2;
			  }

			if( channel->regions.size() > region){	
				histo->Fill( -channel->regions.at(region).min);
			}
		}
	}

	
  //Draw all the histograms
  TCanvas* cmin = new TCanvas("cmin",fname);
  cmin->SetLogy();
  for( std::map<int,FitTH1F*>::iterator it = spectra2.begin();
       it != spectra2.end(); it++){
    RootGraphix::Lock lock = root.AcquireLock();
    FitTH1F* hist = (it->second);
    hist->Draw();
	if(rehist||!found_all){	hist->Write(hist->GetName(),TObject::kOverwrite);}
	cout<<"about to fit"<<endl;
	//FitSPE(hist, ChannelsSettings[it->first], Events->GetEntries());
	cout<<"done fitting"<<endl;
	 cmin->Update();
  }
 DrawSpectra(&spectra2,cmin);
	}

	//create historgams of integrals
  if(!found_all||rehist){
  	  //process the tree line by line, and create histograms for each channel
	  for(int i=0; i < Events->GetEntries(); i++){
		if(i%5000 == 0) cout<<"Processing Entry "<<i<<endl;
		
			Events->GetEntry(i);
			//iterate over all channels
			/*
			  if(event->channels.size() > 10){    
				cout<<"channels.size()>10"<<endl;
				return -2;}
			*/
			for(size_t j=0; j < event->channels.size(); j++){
			  ChannelData* channel = &(event->channels.at(j));
			  if(channel->channel_id < 0){
				cout<<"channel->channel_id < 0"<<endl;
				continue;
				}
        if(!channel->baseline.found_baseline){
				if (i==0){cout<<"channel "<<channel->channel_id<<": no baseline found, skipping"<<endl;}
				continue;
				}
			  std::map<int,FitTH1F*>::iterator mapit = spectra.find(channel->channel_id);
			  FitTH1F* histo=0;
			  if(mapit == spectra.end()){
				TString name = "channel";
				name += channel->channel_id;
				cout<<"Creating new histogram with name "<<name<<endl;
				histo = new FitTH1F(name,name,nbins,start,end);
				spectra.insert( std::make_pair(channel->channel_id, histo) );
			  }
			  else{
				histo = (mapit->second);
			  }
		
			  //fill the histogram
			  if(!histo){
			cerr<<"Null pointer passed!\n";
			return -2;
			  }
			//if(channel->pulses.size() > region)	histo->Fill( -channel->pulses.at(region).integral);
			if( channel->regions.size() > region){	
			    histo->Fill( -channel->regions.at(region).integral);
			}
		}
	}
  }
	
  //Draw all the histograms
  TCanvas* c = new TCanvas("c",fname);
  c->SetLogy();
  for( std::map<int,FitTH1F*>::iterator it = spectra.begin();
       it != spectra.end(); it++){
    RootGraphix::Lock lock = root.AcquireLock();
    FitTH1F* hist = (it->second);
    hist->Draw();
	if(rehist||!found_all){	hist->Write(hist->GetName(),TObject::kOverwrite);}
	cout<<endl<<"About to fit channel: "<<it->first<<endl<<endl;
	FitSPE(hist, ChannelsSettings[it->first], Events->GetEntries());
	cout<<endl<<"Done fitting channel: "<<it->first<<endl<<endl;
    c->Update();
  } 
  
  
  QueryUser(&spectra, Events, &root, ChannelsSettings, c);
  f->Close();
  return spectra.size();
  
}

int main(int argc, char** argv)
{
  bool rehist=false; 
  bool reprocess=false;
  bool min = false; //amplitude switch
  bool mc=false;

  ConfigHandler* config = ConfigHandler::GetInstance();
  config->AddCommandSwitch('m',"amplitude","generate histogram of amplitude",
                           CommandSwitch::Toggle(min));
  config->AddCommandSwitch('r',"rehist","re-histogram data",
			   CommandSwitch::Toggle(rehist));
  config->AddCommandSwitch('p',"reprocess",
			   "Force (re)processing of genroot output",
			   CommandSwitch::SetValue<bool>(reprocess,true)); 
  config->AddCommandSwitch('s',"mc",
			   "Data is MC simulation",
			   CommandSwitch::SetValue<bool>(mc,true));

  config->SetProgramUsageString("laserrun [options] <run number>");
  config->ProcessCommandLine(argc,argv);
  
  if(argc != 2){
    config->PrintSwitches(true);
  }
  //generate the run filename
  int run = atoi(argv[1]);
  std::stringstream rootfile, rawfile;
  if(!mc){
    rootfile << "/data/singlepe/Run";
    rawfile  << "/data/rawdata/Run";
    rootfile<<std::setw(6)<<std::setfill('0')<<run<<".root";
    rawfile<<std::setw(6)<<std::setfill('0')<<run<<"/Run"<<std::setw(6)<<std::setfill('0')<<run<<".out";
  } else {
    rootfile << "/data/test_processing/mc_singlepe/Run";
    rawfile  << "/data/test_processing/mc_singlepe_raw/Run";
    rootfile<<std::setw(6)<<std::setfill('0')<<run<<".root";
    rawfile<<std::setw(6)<<std::setfill('0')<<run<<"/Run"<<std::setw(6)<<std::setfill('0')<<run<<".out";
  }

  std::ifstream fin(rootfile.str().c_str());
  if(reprocess || !fin.is_open()){
    if(!reprocess){
      std::cout<<"The processed rootifle "<<rootfile.str()<<" is not present."
	       <<"\nWould you like to create it now? (y/n)"<<std::endl;
      char answer;
      std::cin>>answer;
      if(answer != 'y' && answer != 'Y'){
	std::cout<<"OK, aborting"<<endl;
	return 1;
      }
    }
    std::stringstream command;
    if(!mc){
      command << "./genroot --cfg cfg/laserrun_analysis.cfg "<<rawfile.str();
    } else {
      command << "./genroot --cfg cfg/laserrun_mc_analysis.cfg "<<rawfile.str();
    }

    if( system(command.str().c_str()) ){
      std::cerr<<"Unable to generate rootfile; aborting."<<std::endl;
      return 1;
    }
  }
  fin.close();
  return ProcessLaserRun(rootfile.str().c_str(),0,rehist,min);
}
