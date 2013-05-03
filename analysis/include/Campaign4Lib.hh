#ifndef RUN4LIB_H
#define RUN4LIB_H

#include <iostream>
#include <fstream>
#include <vector>
#include "EventData.hh"

//CUTS 

bool nodesync (EventData* event)
{
    return (event->status == 0);
}    
bool nosat (EventData* event)
{
    return (event->saturated == 0);
}   
bool baseline (EventData* event)
{
    return (bool)(event->GetChannelByID(-2)->baseline.found_baseline);
}    
bool start_time (EventData* event)
{
    return (bool)(event->GetChannelByID(-2)->pulses[0].start_time <= 0. && event->GetChannelByID(-2)->pulses[0].start_time >= -0.13);
}  

bool late_peak (EventData* event, int n_pulse)
{
    return (bool)(event->GetChannelByID(-2)->pulses[n_pulse].peak_time - event->GetChannelByID(-2)->pulses[n_pulse].start_time > 0.2);
}

bool more_one_pulses (EventData* event)
{
    return (bool)(event->GetChannelByID(-2)->npulses > 1);
}

bool s1s2_Identifier (EventData* event, Int_t& s1_pulse_number, std::vector<Int_t>& s2_pulse_number)
{
    Int_t npulses = event->GetChannelByID(-2)->npulses;
    s1_pulse_number = -1;
    s2_pulse_number.clear();
   
    Double_t max_t_drift = 105.;

    switch (npulses) 
    {
	//ASSUMING FOR ALL CASES THAT THE FIRST PULSE IS S1 
    case 0:
	s1_pulse_number = -1;
	break;
            
    case 1:
	s1_pulse_number = 0;
	break;
            
    case 2://ADD CASE WHEN SECOND PULSE IS SHIT
	if((event->GetChannelByID(-2)->pulses[1].end_time - event->GetChannelByID(-2)->pulses[1].start_time) < 6.)
	    //too short BUT FIX IT!!!!!!!
	{
	    s1_pulse_number = 0;
	}
	else
	{
	    s1_pulse_number = 0;
	    s2_pulse_number.push_back(1);
	}
	break;
            
    case 3:
	s1_pulse_number = 0;
            
	if( (event->sum_of_int[1].npe/event->sum_of_int[2].npe) < 0.001 
	    && ((event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) )
	{
	    //Event with a second pulse not real and very small between s1 and s2
                
	    s2_pulse_number.push_back(2);
	}
	else if( (event->sum_of_int[1].npe/event->sum_of_int[2].npe) > 0.001 
		 && ((event->GetChannelByID(-2)->pulses[1].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) 
		 && ((event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) > max_t_drift))
	{
	    //Event with second pulse that is s2 and a third pulse that can be a shit or s3 but not another s2
	    s2_pulse_number.push_back(1);
	}
	else if( (event->GetChannelByID(-2)->pulses[1].start_time - event->GetChannelByID(-2)->pulses[0].start_time) > max_t_drift )
	{
	    //Pile-up of two events or unphysical
	}
	else if( (event->sum_of_int[1].npe/event->sum_of_int[2].npe) > 0.001 
		 && (event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift )
	{
	    //Third pulse can be another s2 or tail of first s2 split into a new pulse (it cannot be s3 that is 100us from s2)
	    s2_pulse_number.push_back(1);
	    s2_pulse_number.push_back(2);
	}
            
	break;
            
    case 4:
	s1_pulse_number = 0;

	//Second pulse is a shit
	if( (event->sum_of_int[1].npe/event->sum_of_int[2].npe) < 0.001)
	{
		
	    if( ((event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) 
		&& ((event->GetChannelByID(-2)->pulses[3].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) )
	    {
		//Second pulse is a shit and the 3rd and 4th are two s2 (or the 4th is the tail of the 3rd inside max_t_drift)
		s2_pulse_number.push_back(2);
		s2_pulse_number.push_back(3);
	    }
	    else if( ((event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) 
		     && ((event->GetChannelByID(-2)->pulses[3].start_time - event->GetChannelByID(-2)->pulses[0].start_time) > max_t_drift) )
	    {
		//Second pulse is a shit and the 3rd is s2. 4th is tail of 3d outisde the max_t_drift, or s3 or shit
		s2_pulse_number.push_back(2);
	    }
	}
	//2nd pulse is a shit and 3rd is a shit and 4th is regular
	else if( ((event->sum_of_int[1].npe/event->sum_of_int[3].npe) < 0.001) 
		 && ((event->sum_of_int[2].npe/event->sum_of_int[3].npe) < 0.001) )
	{
	    //3rd pulse is a shit 
	    s2_pulse_number.push_back(3);
	}
	//2nd pulse is regular and 3rd is a shit and 4th is shit
	else if( ((event->sum_of_int[1].npe/event->sum_of_int[2].npe) > 0.001) 
		 && ((event->sum_of_int[2].npe/event->sum_of_int[3].npe)> 0.001 
		     && event->sum_of_int[3].npe>10.))//CHECK THIS NUMBER USED TO SAY LAST PULSE IS SMALL
	{
	    s2_pulse_number.push_back(1);
	}
	//2nd pulse is regular and 3rd is regular and 4th is shit
	else if( ((event->sum_of_int[1].npe/event->sum_of_int[2].npe) > 0.001) 
		 && ((event->sum_of_int[3].npe/event->sum_of_int[2].npe) < 0.001) 
		 && ((event->GetChannelByID(-2)->pulses[2].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift))
	{
	    s2_pulse_number.push_back(1);
	    s2_pulse_number.push_back(2);
	}
	//2nd pulse is regular and 3rd is regular and 4th is regular
	else if( ((event->sum_of_int[1].npe/event->sum_of_int[2].npe) > 0.001) 
		 && ((event->sum_of_int[2].npe/event->sum_of_int[3].npe) > 0.001) 
		 && ((event->GetChannelByID(-2)->pulses[3].start_time - event->GetChannelByID(-2)->pulses[0].start_time) < max_t_drift) 
		 && event->sum_of_int[3].npe>10.)//CHECK THIS NUMBER USED TO SAY LAST PULSE IS SMALL)
	{
	    s2_pulse_number.push_back(1);
	    s2_pulse_number.push_back(2);
	    s2_pulse_number.push_back(3);
	}
	break;
    }

    return true;
}

bool load_position_recon_file (std::string filename, std::vector<std::vector <double> >& calib_points)
{
    std::ifstream fin(filename.c_str());
    if(!fin.is_open())
    {
	std::cout<<"Unable to open position calibration file "
		 <<filename<<std::endl;;
	return false;
    }
    
    calib_points.clear();
    std::string line;
    while(std::getline(fin,line))
    {
	std::stringstream s(line);
	double val;
	std::vector<double> vals;
	while(s>>val)
	    vals.push_back(val);
	//make sure we're actually within the fiducial volume...
	if( sqrt(vals[0]*vals[0]+vals[1]*vals[1]) <= 8.25*2.54/2. )
	    calib_points.push_back(vals);
	
    }
  
    std::cout<<"Loaded "<<calib_points.size()<<" calibration points for position reconstruction.\n";
	
    if(calib_points.size() == 0)
	return false;

    return true; 
}

bool position_eval (EventData* event, int n_pulse, double& x, double& y, std::vector<std::vector<double> > calib_points)
{
    if ( n_pulse < 0 
	 || (n_pulse + 1 > event->GetChannelByID(-2)->npulses))
    {
	cout << "No such pulse" << endl;
	return false;
    }

    x = -10; y = -10;

    //CALCULATE FRACTIONAL CHARGE ON EACH PHOTOTUBE
    double total_charge = event->sum_of_int[n_pulse].fixed_npe2;
    std::vector<double> frac_charge (event->channels.size(), 0);
    std::vector<double> err_frac_charge (event->channels.size(), 0);
    
    for (int j = 0; j < (int)event->channels.size() - 1; j++)
    {//Loop over physical channels
	
	frac_charge[j] = (-(event->GetChannelByID(j)->pulses[n_pulse].fixed_int2)/(event->GetChannelByID(j)->spe_mean))/total_charge;
	err_frac_charge[j] = sqrt(frac_charge[j])/total_charge;
    }	

    //Compare with data file and find lowest chi2
    double minchi2=1E12;
    for(size_t pt=0; pt < calib_points.size(); ++pt)
    {
	double chi2=0;
	for (int j = 0; j < (int)event->channels.size() - 1; j++)
	{//Loop over physical channels
	    
	    double val = calib_points[pt][2+2*j]; //this event's signal in each channel
	    double eval = calib_points[pt][3+2*j]; //each channel's signal uncertainty
	    chi2 += (frac_charge[j]-val)*(frac_charge[j]-val)
		/(err_frac_charge[j]*err_frac_charge[j] + eval*eval);
	}

	if(chi2<minchi2)
	{
	    minchi2 = chi2;
	    x = calib_points[pt][0];
	    y = calib_points[pt][1];
	}

    }
    
    return true;
}

double drift_time_eval  (EventData* event, int n_pulse_1, int n_pulse_2)
{
    if ( n_pulse_1 < 0
	 || n_pulse_2 < 0
	 || (n_pulse_1 + 1 > event->GetChannelByID(-2)->npulses)
	 || (n_pulse_2 + 1 > event->GetChannelByID(-2)->npulses))
    {
	cout << "No such pulse" << endl;
	return -1;
    }

    return (event->GetChannelByID(-2)->pulses[n_pulse_2].start_time 
	    - event->GetChannelByID(-2)->pulses[n_pulse_1].start_time);    
}

#endif
