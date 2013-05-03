//////////////////////////////////////////////////////////////////////
//  Raytracing and Optical simulation
//
//  Author: Alfredo G. Cocco
//
//  Date: 01/01/2010
//
//  Last revision : 06/06/2011
//////////////////////////////////////////////////////////////////////

#ifndef __MY_MCOPTICALEVENT__
#define __MY_MCOPTICALEVENT__

#include <vector>

#include "TObject.h"
#include "TClonesArray.h"
#include "TPolyLine3D.h"
#include "TVector3.h"

class PMHit : public TObject {
 public:
  PMHit();
  virtual ~PMHit();

  int detector_index;
  int interaction;
  double wavelength;
  double time;
  TVector3 pos;
  TVector3 dir;
  TVector3 pol;

  ClassDef(PMHit,1);
};

class Ph : public TObject {
 public:
  Ph();
  virtual ~Ph();

  void Clear(void);
  void addHit(TVector3);
  TVector3 getHit(int nh);
  int getHitSize(void);

  TVector3 pos_i, dir_i, pol_i;
  TVector3 pos_f, dir_f, pol_f;

  Int_t nhits;
  Double_t length;
  Double_t wl;

  std::vector<TVector3> hits; //||  do not split 

  ClassDef(Ph,1);

};

class MCOpticalEvent : public TObject {

 public:
  MCOpticalEvent();
  virtual ~MCOpticalEvent();

  void Clear(void);
  void addPh(int, Ph*);
  void addPMHit(int, PMHit*);
  void addPMHit(int, TVector3*, TVector3*, TVector3*, double, double, int, int);

  void Set_Run(Int_t t) {NRun = t;}
  void Set_Event(Int_t t) {NEvent = t;}
  void Set_Time(Int_t t) {Time = t;}

  void Set_PartID(Int_t id) {PartID.push_back(id);}
  void Set_PartTime(Int_t t) {pTime.push_back(t);}
  void Set_Energy(Double_t e) {Energy.push_back(e);}
  void Set_Pos(TVector3* p) {pos.push_back(*p);}

  Int_t Get_Run(void) {return NRun;}
  Int_t Get_Event(void) {return NEvent;}
  Int_t Get_Time(void) {return Time;}

  Int_t Get_PartID(Int_t n) {return PartID[n];}
  Int_t Get_PartTime(Int_t n) {return pTime[n];}
  Double_t Get_Energy(Int_t n) {return Energy[n];}
  TVector3 Get_Pos(Int_t n) {return pos[n];}

  void Set_N1(Int_t n) {N1=n;}
  void Set_N2(Int_t n) {N2=n;}
  Int_t Get_N1(void) {return N1;}
  Int_t Get_N2(void) {return N2;}
  Int_t Get_S1(void) {return S1;}
  Int_t Get_S2(void) {return S2;}
  Double_t Get_T1(void) {return T1;}
  Double_t Get_T2(void) {return T2;}

  TClonesArray *getPhPointer(int);
  TClonesArray *getPMHitsPointer(int);

 protected:

  Int_t NRun;
  Int_t NEvent;
  Int_t Time;
  Int_t N1;
  Int_t N2;
  Int_t S1;
  Int_t S2;
  Double_t T1;
  Double_t T2;

  std::vector<Int_t> PartID;
  std::vector<Int_t> pTime;
  std::vector<Double_t> Energy;
  std::vector<TVector3> pos;

  TClonesArray  *Photons1;     //-> array with all rays         (S1)
  TClonesArray  *PMHits1;      //-> array with all stored hits  (S1)
  TClonesArray  *Photons2;     //-> array with all rays         (S2)
  TClonesArray  *PMHits2;      //-> array with all stored hits  (S2)

  static TClonesArray *fgPhotons1;
  static TClonesArray *fgPMHits1;
  static TClonesArray *fgPhotons2;
  static TClonesArray *fgPMHits2;

  ClassDef(MCOpticalEvent,1);

};

#endif

