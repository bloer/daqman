/** @file SumOfIntegral.hh
    @brief Defines the SumOfIntegral data class
    @author rsaldanha
    @ingroup daqroot
    @ingroup modules
*/

#ifndef SUMOFINTEGRAL_h
#define SUMOFINTEGRAL_h

#include "Rtypes.h" //has the classdef macro
#include "Message.hh"
#include <vector>
#include <iostream>

/** @class SumOfIntegral
    @brief Stores information relevant to the sum of single scintillation event across all channels
    @ingroup modules
    @ingroup daqroot
*/
class SumOfIntegral{
public:
    SumOfIntegral() { Clear(); }
    virtual ~SumOfIntegral() {  }
    void Clear();
    void Print(int pulse);
public:
    Int_t start_index;      ///< index marking the start of the pulse
    double start_time;    ///< time since trigger at start of pulse
    Int_t end_index;        ///< index for the end of the pulse
    double end_time;      ///< time since trigger at end of pulse
    bool start_clean;     ///< start of pulse does not overlap with previous 
    bool end_clean;       ///< end of pulse does not overlap with next or window
    double dt;            ///< Time between the start of this pulse and the next one (or end of trigger)

    bool saturated;       ///< was the pulse saturated on any channel?
    double npe;                   ///< integral scaled for single pe amplitude
    std::vector <double> f_param; ///< f-parameters for different time values
    double f90;                   ///< f-parameter for 90 ns
    double f90_fixed;             ///< f-parameter for 90 ns over fixed_time1 
    double fixed_npe1;            ///< integral evaluated at fixed_time1
    double fixed_npe2;            ///< integral evaluated at fixed_time2
    bool fixed_npe1_valid;        ///< did the event extend at least past fixed_time1?
    bool fixed_npe2_valid;        ///< did the event extend at least past fixed_time2?
    
    Int_t max_chan;                 ///< channel which had the highest number of p.e.
    double max_chan_npe;             ///< no of p.e. on channel which had the highest number of p.e.

    bool is_s1;          ///< true = S1 pulse
    bool is_s2;          ///< true = S2 pulse
    
    double gatti;        ///< Gatti parameter for electron and nuclear recoil templates
    double ll_ele;       ///< Log-likelihood for electron recoil template
    double ll_nuc;       ///< Log-likelihood for nuclear recoil template
    double ll_r;         ///< Log-likelihood ratio of nuclear to electron recoil template
    
  
    ClassDef(SumOfIntegral,2);
};

inline void SumOfIntegral::Clear()
{
    saturated = false;
    start_index = -1;
    start_time = 0;
    end_index = -1;
    end_time = 0;
    npe = 0;
    f_param.clear();
    f90 = 0;
    f90_fixed = 0;
    gatti = 0;
    ll_ele = 0;
    ll_nuc = 0;
    ll_r = 0;
    fixed_npe1 = 0;
    fixed_npe2 = 0;
    fixed_npe1_valid = false;
    fixed_npe2_valid = false;
    dt = -1;
    is_s1 = false;
    is_s2 = false;
    start_clean = false;
    end_clean = false;
    max_chan = -1;
    max_chan_npe = 0;
};

inline void SumOfIntegral::Print(int pulse)
{
    Message m(INFO);
    m<<std::endl;
    m<<"************************************************************************"<<std::endl;
    m<<"******************* SOI PULSE "<<pulse<<"  INFORMATION ***************************"<<std::endl;
    m<<"************************************************************************"<<std::endl;
    m<<"Start Index: "<<start_index<<std::endl;
    m<<"Start Time: "<<start_time<<std::endl;
    m<<"End Index: "<<end_index<<std::endl;
    m<<"End Time: "<<end_time<<std::endl;
    m<<"Start Clean: "<<start_clean<<std::endl;
    m<<"End Clean: "<<end_clean<<std::endl;
    m<<"DT: "<<dt<<std::endl;
    m<<"Saturated: "<<saturated<<std::endl;
    m<<"Npe: "<<npe<<std::endl;
    m<<"Fixed Npe 1 Valid: "<<fixed_npe1_valid<<std::endl;
    m<<"Fixed Npe 1 : "<<fixed_npe1<<std::endl;
    m<<"Fixed Npe 2 Valid: "<<fixed_npe2_valid<<std::endl;
    m<<"Fixed Npe 2 : "<<fixed_npe2<<std::endl;
    m<<"Max Channel: "<<max_chan<<std::endl;
    m<<"Max Channel Npe: "<<max_chan_npe<<std::endl;
    m<<"F90: "<<f90<<std::endl;
    m<<"F90 Fixed: "<<f90_fixed<<std::endl;
    m<<"Gatti: "<<gatti<<std::endl;
    m<<"LL Electron: "<<ll_ele<<std::endl;
    m<<"LL Nuclear: "<<ll_nuc<<std::endl;
    m<<"LL Ratio: "<<ll_r<<std::endl;
    m<<"Is S1: "<<is_s1<<std::endl;
    m<<"Is S2: "<<is_s2<<std::endl;
    m<<"************************************************************************"<<std::endl;
}
#endif
