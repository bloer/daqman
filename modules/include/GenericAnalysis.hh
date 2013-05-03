/** @file GenericAnalysis.hh
    @brief Defines the GenericAnalysis module
    @author bloer
    @ingroup modules
*/
#ifndef GENERICANALYSIS_h
#define GENERICANALYSIS_h

#include "BaseModule.hh"
/** @class GenericAnalysis
    @brief calculate generic info - used for testing purposes
    @ingroup modules
*/
class GenericAnalysis : public BaseModule{
public:
  
  GenericAnalysis();
  ~GenericAnalysis();
  
  int Initialize();
  int Finalize();
  int Process(EventPtr event);
  
  static const std::string GetDefaultName(){ return "GenericAnalysis"; }

};

#endif
