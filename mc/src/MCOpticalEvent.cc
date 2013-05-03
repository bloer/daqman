//////////////////////////////////////////////////////////////////////
//  Raytracing and Optical simulation
//
//  Author: Alfredo G. Cocco
//
//  Date: 01/01/2010
//
//  Last revision : 06/06/2011
//////////////////////////////////////////////////////////////////////

#include "MCOpticalEvent.hh"

ClassImp(PMHit)
ClassImp(Ph)
ClassImp(MCOpticalEvent)

TClonesArray *MCOpticalEvent::fgPhotons1 = 0;
TClonesArray *MCOpticalEvent::fgPMHits1 = 0;
TClonesArray *MCOpticalEvent::fgPhotons2 = 0;
TClonesArray *MCOpticalEvent::fgPMHits2 = 0;

static bool first_t1;
static bool first_t2;

PMHit::PMHit() {

}

PMHit::~PMHit() {

}

Ph::Ph() {
  nhits = 0;
  length = 0.0;
}

Ph::~Ph() {
  Clear();
}

void Ph::Clear(void) {
  hits.clear();
  nhits = 0;
  length = 0.0;
}

void Ph::addHit(TVector3 hit) {
  hits.push_back(hit);
  nhits++;
}

int Ph::getHitSize(void) {return hits.size();}

TVector3 Ph::getHit(int nh) {return hits[nh];}

MCOpticalEvent::MCOpticalEvent() {

  NRun = 0;
  NEvent = 0;
  Time = 0;
  N1=0;
  N2=0;
  S1=0;
  S2=0;
  T1 = 0.0;
  T2 = 0.0;

  PartID.clear();
  Energy.clear();
  pos.clear();
  pTime.clear();

  first_t1 = false;
  first_t2 = false;

  if (fgPhotons1 == 0) fgPhotons1 = new TClonesArray("Ph", 7000);
  else fgPhotons1->Clear();
  Photons1 = fgPhotons1;

  if (fgPMHits1 == 0) fgPMHits1 = new TClonesArray("PMHit", 7000);
  else fgPMHits1->Clear();
  PMHits1 = fgPMHits1;

  if (fgPhotons2 == 0) fgPhotons2 = new TClonesArray("Ph", 7000);
  else fgPhotons2->Clear();
  Photons2 = fgPhotons2;

  if (fgPMHits2 == 0) fgPMHits2 = new TClonesArray("PMHit", 7000);
  else fgPMHits2->Clear();
  PMHits2 = fgPMHits2;

}

MCOpticalEvent::~MCOpticalEvent() {
   Clear();
}

void MCOpticalEvent::Clear(void) {

  Ph *ph1;
  Ph *ph2;

  NRun = 0;
  NEvent = 0;
  Time = 0;
  N1=0;
  N2=0;
  S1=0;
  S2=0;
  T1 = 0.0;
  T2 = 0.0;

  PartID.clear();
  Energy.clear();
  pos.clear();
  pTime.clear();

  first_t1 = false;
  first_t2 = false;

  if(fgPhotons1 != 0) {
    for(int i=0; i<fgPhotons1->GetEntries(); i++) {
      ph1 = (Ph*) fgPhotons1->At(i);
      ph1->Clear();
    }
    fgPhotons1->Clear();
  }

  if(fgPhotons2 != 0) {
    for(int i=0; i<fgPhotons2->GetEntries(); i++) {
      ph2 = (Ph*) fgPhotons2->At(i);
      ph2->Clear();
    }
    fgPhotons2->Clear();
  }

  if(fgPMHits1 != 0) fgPMHits1->Clear();

  if(fgPMHits2 != 0) fgPMHits2->Clear();

}

void MCOpticalEvent::addPh(int ii, Ph *ray) {

  int i;

  if(ii==1) {
    i = fgPhotons1->GetLast() + 1;
    new((*fgPhotons1)[i]) Ph(*ray);
    N1++;
  } else {
    i = fgPhotons2->GetLast() + 1;
    new((*fgPhotons2)[i]) Ph(*ray);
    N2++;
  }

}

void MCOpticalEvent::addPMHit(int ii, PMHit *shit) {

  int i;

  if(ii==1) {
    i = fgPMHits1->GetLast() + 1;
    new((*fgPMHits1)[i]) PMHit(*shit);
    S1++;
    if(!first_t1) {
      T1 = shit->time; 
      first_t1 = true;
    } else {
      if((shit->time < T1) && (shit->time > 0.0)) T1=shit->time;
    }
  } else {
    i = fgPMHits2->GetLast() + 1;
    new((*fgPMHits2)[i]) PMHit(*shit);
    S2++;
    if(!first_t2) {
      T2 = shit->time; 
      first_t2 = true;
    } else {
      if((shit->time < T2) && (shit->time > 0.0)) T2=shit->time;
    }
  }

}

void MCOpticalEvent::addPMHit(int ii, TVector3 *p, TVector3 *d, TVector3 *r, double w, double t, int index, int itx) {

  int i;
  PMHit stored_hit;

  stored_hit.pos = *p;
  stored_hit.dir = *d;
  stored_hit.pol = *r;
  stored_hit.wavelength = w;
  stored_hit.detector_index = index;
  stored_hit.interaction = itx;
  stored_hit.time = t;

  if(ii==1) {
    i = fgPMHits1->GetLast() + 1;
    new((*fgPMHits1)[i]) PMHit(stored_hit);
    S1++;
    if(!first_t1) {
      T1 = stored_hit.time; 
      first_t1 = true;
    } else {
      if((stored_hit.time < T1) && (stored_hit.time > 0.0)) T1=stored_hit.time;
    }
  } else {
    i = fgPMHits2->GetLast() + 1;
    new((*fgPMHits2)[i]) PMHit(stored_hit);
    S2++;
    if(!first_t2) {
      T2 = stored_hit.time; 
      first_t2 = true;
    } else {
      if((stored_hit.time < T2) && (stored_hit.time > 0.0)) T2=stored_hit.time;
    }
  }

}

TClonesArray *MCOpticalEvent::getPhPointer(int i) {
  return (i==1) ? fgPhotons1 : fgPhotons2;
}

TClonesArray *MCOpticalEvent::getPMHitsPointer(int i) {
  return (i==1) ? fgPMHits1 : fgPMHits2;
}

