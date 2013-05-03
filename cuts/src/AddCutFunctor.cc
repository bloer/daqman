#include "AddCutFunctor.hh"
#include "BaseModule.hh"
#include "CheckRegion.hh"
#include "Message.hh"
#include <stdexcept>

using namespace ProcessingCuts;
std::istream& AddCutFunctor::operator()(std::istream& in)
{
  cuts_list* all_cuts = GetListOfCuts();
  ProcessingCut* cut = 0;
  std::string key;
  in>>key;
  cuts_list::iterator cutit= all_cuts->find(key);
  if(cutit != all_cuts->end()){
    cut = (cutit->second)->GetCut();  
    in>>*cut;
    if(!in){
      Message(ERROR)<<"Error reading cut parameters for "<<key<<std::endl;
    }
    else{
      Message(DEBUG)<<"Adding cut "<<cut->GetName()<<" to module "
		    <<_parent->GetName()<<std::endl;
      _parent->AddProcessingCut(cut);
      cut->AddDependenciesToModule(_parent);
    }
  }
  else{
    Message(ERROR)<<"Unkown cut :\""<<key<<"\"\n";
    throw std::invalid_argument(key);
  }

  return in;
}

std::ostream& AddCutFunctor::operator()(std::ostream& out)
{
  const std::vector<ProcessingCut*>* cuts = _parent->GetCuts();
  std::vector<ProcessingCut*>::const_iterator cutit = cuts->begin();
  for( ; cutit != cuts->end(); cutit++){
    out<<GetFunctionName()<<" "<<(*cutit)->GetDefaultKey()<<" "<<*(*cutit)<<" ";
  }
  return out;
}
