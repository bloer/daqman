/** @file SumOfIntegralEval.hh
    @brief defines the SumOfIntegralEval module
    @author rsaldanha
    @ingroup modules
*/

#ifndef SUMOFINTEGRALEVAL_h
#define SUMOFINTEGRALEVAL_h

#include "BaseModule.hh"
#include <map>
/** @class SumOfIntegralEval
    @brief SumOfIntegralEval processes ...
    @ingroup modules
*/
class SumOfIntegralEval : public BaseModule
{
public:
  SumOfIntegralEval();
  ~SumOfIntegralEval();
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  static const std::string GetDefaultName(){ return "SumOfIntegralEval"; }

private:
};

#endif
