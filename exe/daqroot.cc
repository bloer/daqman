/** @defgroup daqroot daqroot - ROOT environment plus extra classes/functions
 *  @file daqroot.cc
 *  @brief main file for the daqroot executable
 *
 *  daqroot adds the libraries/dictionaries for the EventData, ChannelData, etc
 *  classes which are stored in the Events tree, to the ROOT interactive shell.
 *  daqroot also adds several utility functions and classes defined in the 
 *  analysis directory. See the notes in LinkDef.h for how to add classes.
 *
 *  @ingroup daqroot
 */

#include "TRint.h"
#include "TSystem.h"
#include "utilities.hh"
#include <string>
#include <iostream>

int main(int argc, char** argv)
{
  //add all the EventData headers to the include path so we can compile
#ifdef ROOTINCLUDEPATH
  if(gSystem)
    gSystem->AddIncludePath(ROOTINCLUDEPATH);
#endif
  //add the SaveHistoToFile function to the TH1 command menu
  CustomizeHistogramMenus();
  
  std::cout<<"=== This is daqroot: the ROOT interpreter + "
	   <<"daqman classes and functions. ==="<<std::endl;
  TRint rint("daqroot",&argc, argv,0,0,true);
  rint.SetPrompt("daqroot %d> ");
  rint.Run();

}
