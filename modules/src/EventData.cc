#include "EventData.hh"
#include "Message.hh"

void EventData::Print(int verbosity)
{
    if (verbosity >= 1)
    {
	Message m(INFO);
	m<<std::endl;
	m<<"************************************************************************"<<std::endl;
	m<<"************************* EVENT INFORMATION ****************************"<<std::endl;
	m<<"************************************************************************"<<std::endl;
	m<<"Run ID: "<<run_id<<std::endl;
	m<<"Event ID: "<<event_id<<std::endl;
	m<<"Status: "<<status<<std::endl;
	m<<"Trigger Count: "<<trigger_count<<std::endl;
	m<<"Time Stamp: "<<timestamp<<std::endl;
	m<<"Dt: "<<dt<<std::endl;
	m<<"Event Time: "<<event_time<<std::endl;
	m<<"N. Chans: "<<nchans<<std::endl;
	m<<"Saturated: "<<saturated<<std::endl;
	m<<"S1 Valid: "<<s1_valid<<std::endl;
	m<<"S2 Valid: "<<s2_valid<<std::endl;
	m<<"S1S2 Valid: "<<s1s2_valid<<std::endl;
	m<<"S1 Start Time: "<<s1_start_time<<std::endl;
	m<<"S1 End Time: "<<s1_end_time<<std::endl;
	m<<"S2 Start Time: "<<s2_start_time<<std::endl;
	m<<"S2 End Time: "<<s2_end_time<<std::endl;
	m<<"S1 Full: "<<s1_full<<std::endl;
	m<<"S1 Fixed: "<<s1_fixed<<std::endl;
	m<<"S2 Full: "<<s2_full<<std::endl;
	m<<"S2 Fixed: "<<s2_fixed<<std::endl;
	m<<"Max S1: "<<max_s1<<std::endl;
	m<<"Max S2: "<<max_s2<<std::endl;
	m<<"Max S1 Channel: "<<max_s1_chan<<std::endl;
	m<<"Max S2 Channel: "<<max_s2_chan<<std::endl;
	m<<"F90 Full: "<<f90_full<<std::endl;
	m<<"F90 Fixed: "<<f90_fixed<<std::endl;
	m<<"Gatti: "<<gatti<<std::endl;
	m<<"LL_R: "<<ll_r<<std::endl;
	m<<"Drift Time: "<<drift_time<<std::endl;
	m<<"Position Valid: "<<position_valid<<std::endl;
	m<<"X Position: "<<x<<std::endl; 
	m<<"Y Position: "<<y<<std::endl;
	m<<"Z Position: "<<z<<std::endl;
	m<<"Barycentre Valid: "<<bary_valid<<std::endl;
	m<<"X Barycentre: "<<bary_x<<std::endl;
	m<<"Y Barycentre: "<<bary_y<<std::endl;
	m<<"************************************************************************"<<std::endl;
    }
    if (verbosity == 2)
    {
	{
	    Message m(INFO);
	    m<<std::endl;
	    m<<"************************************************************************"<<std::endl;
	    m<<"************************** CHANNEL SUMMARY *****************************"<<std::endl;
	    m<<"************************************************************************"<<std::endl;
	    m<<"\t\tCh. Baseline\t\t\t\t\t"<<std::endl;
	    m<<"Ch.\tPulses\tFound\tMean\tInt.Min\tS1-Full\tS1-Fix\tS2-Full\tS2-Fix"<<std::endl;
	    m<<"************************************************************************"<<std::endl;
	    for (size_t ch = 0; ch < channels.size(); ch++)
	    {
		m<<std::setw(2)<<ch<<"\t"<<std::setw(2)<<channels[ch].npulses<<"\t"
		 <<std::setw(2)<<channels[ch].baseline.found_baseline<<"\t"<<std::setw(6)<< std::setprecision(4)<<channels[ch].baseline.mean<<"\t"	
		 <<std::setw(7)<< std::setprecision(5) <<channels[ch].integral_min<<"\t"
		 <<std::setw(4)<< std::setprecision(4) <<channels[ch].s1_full<<"\t"<<std::setw(4)<< std::setprecision(4) <<channels[ch].s1_fixed<<"\t"
		 <<std::setw(4)<< std::setprecision(4) <<channels[ch].s2_full<<"\t"<<std::setw(4)<< std::setprecision(4) <<channels[ch].s2_fixed
		 <<std::endl;
	    }
	    m<<"***********************************************************************"<<std::endl;
	    //Print details for sum of integrals for each pulse
	    for (size_t p = 0; p < sum_of_int.size(); p++)
	    {
		sum_of_int.at(p).Print(p);
	    }
	    //Print details for sum channel
	    if (GetChannelByID(ChannelData::CH_SUM))
	    {
		m<<std::endl;
		m<<"************************************************************************"<<std::endl;
		m<<"*********************** SUM CHANNEL INFORMATION ************************"<<std::endl;
		m<<"************************************************************************"<<std::endl;
	    }
	}
	if (GetChannelByID(ChannelData::CH_SUM))
	    GetChannelByID(ChannelData::CH_SUM)->Print(verbosity);
    }
    if (verbosity == 3)
    {
	{
	    Message m(INFO);
	    m<<std::endl;
	    m<<"************************************************************************"<<std::endl;
	    m<<"************************* CHANNEL INFORMATION **************************"<<std::endl;
	    m<<"************************************************************************"<<std::endl;
	}
	for (size_t ch = 0; ch < channels.size(); ch++)
	{
	    channels[ch].Print(verbosity);
	}
    }
}
