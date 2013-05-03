#include "PadZoomer.hh"
#include "TCanvas.h"
#include "TH1.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TF1.h"
#include "TExec.h"
#include "TList.h"

#include <string>

void PadZoomer::Attach(TVirtualPad* pad) 
{
  if(!pad) return;
  owner = pad;
  TList* execs = pad->GetListOfExecs();
  if(!execs){
    //this seems to be the only way to create the list...
    pad->AddExec("dummy","");
    execs = pad->GetListOfExecs();
    execs->SetOwner(true);
    execs->Clear();
  }
  TObject* oldcopy = execs->FindObject(GetName());
  if(oldcopy){
    execs->Remove(oldcopy);
    delete oldcopy;
  }
  SetBit(kCanDelete,true);
  execs->Add(this);
  //also add capability to sub-pads
  TList* owned = pad->GetListOfPrimitives();
  if(owned){
    for(int i=0; i<owned->GetSize(); ++i){
      if(owned->At(i)->InheritsFrom(TVirtualPad::Class()))
	new PadZoomer((TVirtualPad*)(owned->At(i)));
    }
  }
}

void PadZoomer::Exec(const char*)
{
  if(!owner || owner->GetEvent() != 61)
    return;
  
  const TObject* selected = owner->GetSelected();
  if(!selected) 
    return;
  std::string drawopt = selected->GetDrawOption();
  if(selected->InheritsFrom(TH1::Class()) || 
     selected->InheritsFrom(TGraph::Class()) ||
     selected->InheritsFrom(TF1::Class()) ){
    if(selected->InheritsFrom(TGraph::Class())) {
      //TGraphs may belong to a TMultiGraph, so look to it for draw option
      TList* primitives = owner->GetListOfPrimitives();
      for(int i=0; i < primitives->GetSize(); ++i){
	TObject* prim = primitives->At(i);
	if(prim->InheritsFrom(TMultiGraph::Class()) &&
	   ((TMultiGraph*)prim)->GetListOfGraphs()->FindObject(selected)){
	  //prim owns selected
	  drawopt = ((TMultiGraph*)prim)->
	    GetGraphDrawOption((const TGraph*)selected);
	  if(drawopt=="")
	    drawopt = prim->GetDrawOption();
	}
      }
      //graph might still need to have the 'a' for 'axes' appended
      if(drawopt.find('a')==std::string::npos)
	drawopt.append("a");
    }
    size_t pos = drawopt.find("same");
    if(pos != std::string::npos){
      drawopt.erase(pos,4);
    }
  }
  else{
    //if we get here, we want to copy the whole sub-pad
    //but don't copy full-size pads!
    if(owner->InheritsFrom(TCanvas::Class()) || 
       owner->GetCanvas()->GetListOfPrimitives()->GetSize()==1)
      return;
    selected = owner;
  }
  TCanvas* c = new TCanvas;
  c->cd(0);
  c->Update(); //<why do I have to do this?
  TObject* newobj = selected->DrawClone(drawopt.c_str());
  newobj->SetBit(kCanDelete,true);
  if(selected == owner){
    TVirtualPad* newpad = (TVirtualPad*)newobj;
    newpad->SetPad(0,0,1,1);
    new PadZoomer(newpad);
  }
    
  return;
}
