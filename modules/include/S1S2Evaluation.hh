/** @file S1S2Evaluation.hh
    @brief Defines the S1S2Evaluation module
    @author bloer
    @ingroup modules
*/
#ifndef S1S2EVALUATION_h
#define S1S2EVALUATION_h

#include "BaseModule.hh"

class PulseFinder; 

/** @class S1S2Evaluation
    @brief Calculates S1 and S2
    @ingroup modules
*/
class S1S2Evaluation : public BaseModule{
public:
  ///@todo Document each member of the class
  S1S2Evaluation();
  virtual ~S1S2Evaluation() {}
  
  int Initialize();
  int Finalize();
  int Process(EventPtr evt);
  
  static std::string GetDefaultName(){ return "S1S2Evaluation"; }
private:
  PulseFinder* _pulse_finder;
};

#endif

