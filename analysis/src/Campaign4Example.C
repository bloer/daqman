#include "TFile.h"
#include "TChain.h"
#include "TH1F.h"
#include "TH2F.h"
#include <iostream>
#include <fstream>
#include "math.h"
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "../include/Campaign4Lib.hh"
#include "../include/DBLib.hh"
#include "EventData.hh"

Bool_t USE_DB = kFALSE;

void Campaign4Example()
{
    std::ostringstream os;
    std::string path = "../../data/ds10/data/test_processing/Run";
    std::vector<int> run_id_list;   
    run_id_list.push_back(3007);
    run_id_list.push_back(3008);
    	
    TChain *chain = new TChain("Events");

    if (! USE_DB)
    {
	std::cout<<"WARNING: Database access disabled ! Make sure to check run list manually"<<std::endl;
    }
    //Check runs for validity and add them to the TChain
    for(vector<int>::const_iterator it = run_id_list.begin(); it != run_id_list.end(); it++)
    {
	if (USE_DB)
	{
	    if ( ! DB_does_run_exist("rundb", "daqruns", *it))
	    {
		std::cout<<"Run "<<(*it)<<" does not exist in the database"<<std::endl;
		continue;
	    }
	    
	    if(DB_get_string("rundb", "daqruns", *it, "comment").find("HARDWARE_ISSUE") != string::npos) 
	    {
		std::cout << "Run" << *it << " has HARDWARE ISSUES. Skipping ..." <<std::endl;
		continue;
	    }       
	    
	    if(DB_get_string("rundb", "daqruns", *it, "type").find("laser") == string::npos 
	       && DB_get_string("rundb", "daqruns", *it, "type").find("*junk") == string::npos 
	       && DB_get_string("rundb", "daqruns", *it, "type").find("darkbox") == string::npos 
	       && DB_get_string("rundb", "daqruns", *it, "type").find("other") == string::npos)
		
	    {
		os.str("");
		os << path << setw(6) << setfill('0') << *it << ".root";
		chain->Add(os.str().c_str());
	    }
	}
	else
	{
	    os.str("");
	    os << path << setw(6) << setfill('0') << *it << ".root";
	    chain->Add(os.str().c_str());
	}
    }

    //Create EventData object to store information from ROOT TTree
    EventData* event = NULL;
    chain->SetBranchAddress("event", &event);
    int n_events = chain->GetEntries();

    //Create output histograms
    TFile* f = new TFile ("Run4example.root", "RECREATE");
    TH1F* s1_hist = new TH1F("s1_hist", "S1 Spectrum", 100, 0, 2000);
    TH2F* s2s1_f90_hist = new TH2F("s2s1_f90_hist", "Log(S2/S1) vs F90", 100, 0, 1, 100, -3, 3.2);

    //Loop over all events in TTree
    for(int n = 0; n < n_events; n = n + 1)
    {
		
        if ( n % 10000 == 0)
            std::cout<<"Processing Event: "<<n<<"/"<<n_events<<std::endl;
        
        chain->GetEntry(n);
        
	//Identify S1 and S2 pulses
	Int_t s1_pulse_number;
	std::vector<Int_t> s2_pulse_number;
	s1s2_Identifier(event, s1_pulse_number, s2_pulse_number);
	

	if (s1_pulse_number == -1)
	    continue;
	
	Int_t s2_multiplicity = s2_pulse_number.size();
	if (s2_multiplicity != 1)
	    continue;

	//Calculate energies
	Double_t s1_fixed = -1;
	if (event->sum_of_int[s1_pulse_number].fixed_npe1_valid)
	    s1_fixed = event->sum_of_int[s1_pulse_number].fixed_npe1;

	
	Double_t s2_fixed = -1;
	if (event->sum_of_int[s2_pulse_number[0]].fixed_npe2_valid)
	    s2_fixed = event->sum_of_int[s2_pulse_number[0]].fixed_npe2;
       

	//Calculate drift time
        Double_t t_drift = -1;
	if (s2_multiplicity > 0)
	    t_drift = drift_time_eval (event, s1_pulse_number, s2_pulse_number[0]);
	
	//CUTS
	if ( start_time(event) 
	     && more_one_pulses(event) 
	     && nosat(event) 
	     && nodesync(event) 
	     && s1_fixed > 4. 
	     && s1_fixed < 600. 
	     && s2_multiplicity == 1 
	     && s2_fixed > 10. 
	     && t_drift > 10. 
	     && t_drift < 85.
	     && ! late_peak (event, s1_pulse_number))
	{
	    s1_hist->Fill(s1_fixed);
	    s2s1_f90_hist->Fill(event->sum_of_int[s1_pulse_number].f90, TMath::Log10(s2_fixed/s1_fixed));
	}
    }//Finish loop over events

    s1_hist->Write();
    s2s1_f90_hist->Write();
    f->Close();
}
