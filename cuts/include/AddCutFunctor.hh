/** @file AddCutFunctor.hh
    @brief Defines AddCutFunctor utility class
    @author bloer
    @ingroup cuts
*/

#ifndef ADDCUTFUNCTOR_h
#define ADDCUTFUNCTOR_h

#include <string>
#include <map>
#include <iostream>

class BaseModule;
class ProcessingCut;

//useful macro
///Macro definition to register a new cut
#define REGISTER_CUT(cut) AddCutFunctor::CutRegistrar<cut> ___registrar___cut;

/** @class AddCutFunctor
    @brief Utility class to dynamically add cuts to modules from a config file
    @ingroup cuts
*/
class AddCutFunctor{
  BaseModule* _parent;
public:
  AddCutFunctor(BaseModule* parent) : _parent(parent) {}
  ~AddCutFunctor(){}
  
  static std::string GetFunctionName(){ return "add_cut"; }
  
  ///overload istream operator for the cut
  std::istream& operator()(std::istream& in);
  /// overload ostream operator for the cut
  std::ostream& operator()(std::ostream& out);
  
  /** @class VCut
      @brief Abstract cut class to be stored in STL containers
  */
  class VCut{
  public:
    VCut(){}
    virtual ~VCut(){}
    virtual ProcessingCut* GetCut()=0;
  };
  
  /** @typedef cuts_list
      @brief map of VCut* with a string name identifier
  */
  typedef std::map<std::string,VCut*> cuts_list;
  
  /// Get all the cuts defined so far
  static cuts_list* GetListOfCuts(){
    static cuts_list _cuts;
    return &_cuts;
  }
  
  /// Template class that registers a specific cut of type T
  template<class T> class CutRegistrar : public VCut{
  public:
    CutRegistrar(){ 
      AddCutFunctor::GetListOfCuts()->
	insert(std::make_pair(T::GetCutName(), this));
    }
    ~CutRegistrar(){}
    ProcessingCut* GetCut(){ return new T;}
  };
  
  
};
#endif
