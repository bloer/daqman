/** @file PMTData.hh
    @brief Defines the PMT data class
    @author rsaldanha
    @ingroup daqroot
    @ingroup modules
*/

#ifndef PMTDATA_h
#define PMTDATA_h
#include "phrase.hh"
#include "RunDB.hh"

/** @class PMTData
    @brief Stores information relevant to a single photomultiplier tube
    @ingroup modules
    @ingroup daqroot
*/
class PMTData
{
public:
    PMTData() { Clear(); }
    void Clear();
    //Load information from database object
    void Load (const RunDB::campaigninfo::pmtinfo& pmt_info);
public:
    phrase serial_id;             ///< serial number
    double photocathode_x;        ///< x position of the center of the photocathode
    double photocathode_y;        ///< y position of the center of the photocathode
    double photocathode_z;        ///< z position of the center of the photocathode
    double photocathode_r;        ///< r (cylindrical coord) position of the center of the photocathode
    double photocathode_theta;    ///< theta (cylindrical coord) position of the center of the photocathode
    double photocathode_area;     ///< area [cm2] of the photocathode
    double photocathode_exp_area; ///< exposed fraction of total photocathode area
    double qe;                    ///< quantum efficiency
	  
};

inline void PMTData::Clear()
{
    serial_id = "";
    photocathode_x = 0;
    photocathode_y = 0;
    photocathode_z = 0;
    photocathode_r = 0;
    photocathode_theta = 0;
    photocathode_area = 1;
    photocathode_exp_area = 1;
    qe = 1;
};

#endif
