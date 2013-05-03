/** @file Roi.hh
    @brief Defines the Roi storage class
    @author bloer
    @ingroup modules
    @ingroup daqroot
*/

#ifndef ROI_h
#define ROI_h

#include "TObject.h"
#include <iostream>
/** @class Roi
    @brief Stores info about a region of interest defined by start,end time
    @ingroup modules
    @ingroup daqroot
*/
class Roi{
public:
  /// Default constructor sets all parameters to default values
  Roi() { Init(); }
  /// Construct a region with the given start,end times and extrema
  Roi(int start, int end, double amax, double amin, double anint, double anpe)
  { Init(start, end, amax, amin, anint, anpe); }
  /// Destructor
  virtual ~Roi() {}
  /// Set the start, end index values and extrema
  void Init(int start=0, int end=0, double amax=0, double amin=0, 
	    double anint=0, double anpe=0)
  {start_index = start; end_index = end; max = amax; min = amin; 
    integral = anint, anpe=0;
  }
  double start_time;  ///< Time defining start of region
  double end_time;    ///< Time defining end of region
  int start_index;    ///< Index definig start of region
  int end_index;      ///< Index defining end of region
  double max;         ///< Maximum value obtained over the region
  double min;         ///< Minimum value obtained over the region
  double integral;    ///< Integral of the signal over the region
  double npe;         ///< Integral divided by single p.e. peak
  int min_index;      ///< Index at which minimum is reaced
  ClassDef(Roi,3);
};
/// Overload ostream to print region info
inline std::ostream& operator<<(std::ostream& out, const Roi& roi){
  return out<<"\n\tstart_time "<<roi.start_time
	    <<"\n\tend_time "<<roi.end_time
	    <<"\n\tstart_index "<<roi.start_index
	    <<"\n\tend_index "<<roi.end_index
	    <<"\n\tmin "<<roi.min
	    <<"\n\tmax "<<roi.max
	    <<"\n\tintegral "<<roi.integral
	    <<"\n\tnpe "<<roi.npe;
}
     
#endif
