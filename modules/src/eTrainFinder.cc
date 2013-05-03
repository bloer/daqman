#include "EventHandler.hh"
#include "eTrainFinder.hh"
#include "ConvertData.hh"
#include "RootWriter.hh"
#include "TFile.h"
#include "TTree.h"
#include "eTrain.hh"
#include <vector>
#include <numeric>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

using namespace std;

eTrainFinder::eTrainFinder():
  ChannelModule(GetDefaultName(), "Checks for pulses in the pre-s1 window")
{

  RegisterParameter("search_start_time", search_start_time = -20.,
                    "Time from start of pulse to begin search [us]");
  RegisterParameter("search_stop_time", search_stop_time = -0.5,
                    "Time from start of pulse to begin search [us]");
  RegisterParameter("rough_threshold", rough_threshold = 25,
                    "Value by which waveform must drop over 1 sample");
  RegisterParameter("distance", distance = 100,
		    "Minumum number of samples between spikes");
  RegisterParameter("coinc_dist", coinc_dist = 3,
                    "Maximum number of events (between bad events) for ++coincidence");
  RegisterParameter("eMin", eMin = 1,
                    "Minimum number of spikes for a bad event");
  RegisterParameter("eMax", eMax = 100,
                    "Maximum number of spikes for a bad event");
  AddDependency("BaselineFinder");
}

eTrainFinder::~eTrainFinder()
{
}


int eTrainFinder::Initialize()
{
  const int nchans=14;
  for(int i=0; i<nchans; i++)
  {
    _nbad.push_back(0);
    _coinc_track.push_back(0);
    _coinc.push_back(0);
    _first.push_back(0);
    _last.push_back(0);
  }

  _n=0;

  return 0;
}


int eTrainFinder::Finalize()
{
  if(gFile && gFile->IsOpen())
  {
    unsigned int nchans=14;
    int maxbad=0;
    int bchan=0;
//double check that all vectors have size = nchans
    if(   _coinc.size()!=nchans
       || _first.size()!=nchans
       || _last.size()!=nchans
       || _coinc_track.size()!=nchans
       || _nbad.size()!=nchans)
      Message(ERROR)<<"a vector is not sized aright"<<endl;

    for(unsigned int i=0; i<_coinc.size(); i++)
    {
      if(_coinc.at(i)>maxbad)
      {
	maxbad=_coinc.at(i);
	bchan=i;
      }
    }

    int RunID = EventHandler::GetInstance()->GetRunID();
    string id;
    stringstream convert;
    convert << RunID;
    id=convert.str();
    string treename0, filename0, filename1;
    filename0 = "Run00";
    filename1 = ".root";
    treename0 = "Found_eTrain_Run_";
    string treename, filename;
    filename = filename0 + id + filename1;
    treename = treename0 + id;

    TTree *eTrains = new TTree("eTrains",treename.c_str());
    eTrain* caught= new eTrain();
    eTrains->Branch("caught.",&caught);

    caught->bright_channel = bchan;
    caught->first_coinc = _first.at(bchan);
    caught->last_coinc = _last.at(bchan);
    for(unsigned int i=0; i<_coinc.size(); i++)
      caught->coincidences.push_back(_coinc.at(i));

    eTrains->Fill();
    gFile->Write();
    delete caught;
    delete eTrains;

    _nbad.clear();
    _coinc_track.clear();
    _coinc.clear();
    _first.clear();
    _last.clear();

  }
  return 0;
}


int eTrainFinder::Process(ChannelData* chdata)
{
  _n++;
  EventDataPtr current_event_data = _current_event->GetEventData();
  const int startscan = chdata->TimeToSample(search_start_time);
  const int stopscan = chdata->TimeToSample(search_stop_time);
  const int nsamps = 1 + stopscan - startscan;
  const int thresh = rough_threshold;  //difference in baseline in one sample for a hit
  const int min_sep = distance;  //separation in samples for a new hit
  const int eventID = current_event_data->event_id;
  const int channelID = chdata->channel_id;
  double* wave = chdata->GetBaselineSubtractedWaveform();
  int lastbad = startscan;

  if(channelID<0)
    return 0;

  const int old_nbad = _nbad.at(channelID);

  for(int test_sample = startscan+1; test_sample < nsamps; test_sample++) //look for the rising edge of a pulse
  {
    if( ( (wave[test_sample]-wave[test_sample-1]) > thresh )
        &&
        ( test_sample - lastbad >= min_sep ) )
    {
      _nbad.at(channelID)++;
      lastbad = test_sample;
    }
  }
  if(     (_nbad.at(channelID)-old_nbad >= eMin)
       && (_nbad.at(channelID)-old_nbad <= eMax) ) //then we caught a train
  {
    if(     (_coinc_track.at(channelID)>0)
         && (_coinc_track.at(channelID) <= coinc_dist) ) //then there is an event-coincident eTrain
    {
        _coinc.at(channelID)++;
        _last.at(channelID)=eventID;
    if(_first.at(channelID)==0)
      _first.at(channelID)=eventID;
    }
    _coinc_track.at(channelID)=0;
  }
  else
    _coinc_track.at(channelID)++; //otherwise, augment distance between eTrain-having events

  Unspikes found_unspikes;
  found_unspikes.nbad = _nbad.at(channelID) - old_nbad;
  chdata->unspikes.push_back(found_unspikes);

  return 0;
}
