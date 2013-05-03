/** @file PulseShapeEval.hh
    @brief defines the PulseShapeEval module
    @author rsaldanha
    @ingroup modules
*/

#ifndef PULSESHAPEEVAL_h
#define PULSESHAPEEVAL_h

#include "BaseModule.hh"
#include <map>
/** @class PulseShapeEval
    @brief PulseShapeEval processes ...
    @ingroup modules
*/
class PulseShapeEval : public BaseModule
{
public:
  PulseShapeEval();
  ~PulseShapeEval();
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  static const std::string GetDefaultName(){ return "PulseShapeEval"; }

private:
    TH1F* event_shape;

    std::map<int, TH1F*> gatti_weights;
    std::map<int, TH1F*> ll_ele_weights;
    std::map<int, TH1F*> ll_nuc_weights;
    std::map<int, TH1F*> ll_r_weights;

    std::string pulse_shape_file;
    std::string gatti_weights_hist;
    std::string ll_ele_weights_hist;
    std::string ll_nuc_weights_hist;
    std::string ll_r_weights_hist;
    
    int LoadWeights();
};

#endif
