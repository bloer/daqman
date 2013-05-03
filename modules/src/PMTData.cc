#include "PMTData.hh"
#include "RunDB.hh"

void PMTData::Load(const RunDB::campaigninfo::pmtinfo& pmt_info)
{
    Clear();
    serial_id = pmt_info.serial_id;
    photocathode_x = pmt_info.photocathode_x;
    photocathode_y = pmt_info.photocathode_y;
    photocathode_z = pmt_info.photocathode_z;
    photocathode_r = pmt_info.photocathode_r;
    photocathode_theta = pmt_info.photocathode_theta;
    photocathode_area = pmt_info.photocathode_area;
    photocathode_exp_area = pmt_info.photocathode_exp_area;
    qe = pmt_info.qe;
}
