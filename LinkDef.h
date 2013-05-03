/**
   @file LinkDef.h
   @brief Commands for ROOT dictionary generation
   
   This file contains directives for all classes/functions that need to have
   a dictionary generated for ROOT. This is necessary either to store
   instances of classes in a tree, or to use them in the interactive ROOT.

   Note that any class which will be stored in an stl container needs to have
   that container specifically defined as well

*/

#ifdef __MAKECINT__
#pragma link C++ class TOF+;
#pragma link C++ class Spe+;
#pragma link C++ class std::vector<Spe>+;
#pragma link C++ class eTrain+;
#pragma link C++ class std::vector<eTrain>+;
#pragma link C++ class Unspikes+;
#pragma link C++ class std::vector<Unspikes>+;
#pragma link C++ class PulseFit+;
#pragma link C++ class Pulse+;
#pragma link C++ class std::vector<Pulse>+;
#pragma link C++ class Baseline+;
#pragma link C++ class ChannelData+;
#pragma link C++ class std::vector<ChannelData>+;
#pragma link C++ class SumOfIntegral+;
#pragma link C++ class std::vector<SumOfIntegral>+;
#pragma link C++ class EventData+;
#pragma link C++ class Roi+;

#pragma link C++ class std::vector<Roi>+;
#pragma link C++ class LightYieldGraph+;
#pragma link C++ namespace RunDB+;
#pragma link C++ class RunDB::runinfo::channelinfo+;
#pragma link C++ class std::vector<RunDB::runinfo::channelinfo>+;
#pragma link C++ class RunDB::runinfo::time_param+;
#pragma link C++ class RunDB::runinfo::runtype+;
#pragma link C++ class RunDB::runinfo+;
#pragma link C++ class Ph+;
#pragma link C++ class PMHit+;
#pragma link C++ class MCOpticalEvent+;
#pragma link C++ all function; ///< This should catch all global functions

#endif
