/** @defgroup cuts cuts - Skip processing of some modules based on cuts
    @ingroup modules
*/

/** @file ProcessingCut.hh
    @brief Defines the ProcessingCut base class
    @author bloer
    @ingroup cuts
*/

#ifndef PROCESSING_CUT_h
#define PROCESSING_CUT_h

#include "Event.hh"
#include "ParameterList.hh"
class BaseModule;

/** @class ProcessingCut
    @brief Base class representing a cut to check whether a module should run
    @ingroup cuts
*/
class ProcessingCut : public ParameterList{
  std::string _cut_name; ///< The name of the cut for config files
public:
  /// Constructor takes name of the cut
  ProcessingCut(const std::string& cut_name="", const std::string helptext="");
  /// Destructor 
  virtual ~ProcessingCut(){}
  
  /// Evaluate the entire event to determine if it should pass
  virtual bool Process(EventDataPtr event);
  /// Evaluate a single channel to determine if it should pass
  virtual bool ProcessChannel(ChannelData* chdata){ return true; }
  /// Cuts should make sure that they include any modules they depend on
  virtual int AddDependenciesToModule(BaseModule*){ return 0; }
  /// Get the name of this cut
  std::string GetName(){ return _cut_name; }
protected:
  bool default_pass;    ///< require all channels to pass to be true?
};

/** @namespace ProcessingCuts
    @brief Segments cuts so they don't conflict with modules in empty namespace
    @ingroup cuts
*/
namespace ProcessingCuts{}

#endif
