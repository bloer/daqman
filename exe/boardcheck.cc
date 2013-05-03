/** @file boardcheck.cc
    @brief main file for boardcheck executable, probes for CAEN V172X digitizers
    @author bloer
    @ingroup daqman
*/

#include "CAENVMElib.h"
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <sstream>

using namespace std;


int main(int argc, const char** argv)
{
  char message[100];
  CVErrorCodes err;
  //check the software release
  err = CAENVME_SWRelease(message);
  cout<<"CAEN software version: "<<message<<endl;
  
  //check for boards on the allowed address space
  
  int nboards=0;
  uint32_t min = 0;
  uint32_t max = 0xFFFF0000;
  if(argc >= 2){
    stringstream minstream(argv[1]);
    minstream>>hex>>min;
  }
  if(argc == 3){
    stringstream maxstream(argv[2]);
    maxstream>>hex>>max;
  }
  if(argc > 3)
    cerr<<"Usage: "<<argv[0]<<" [min_address] [max_address]\n";
  
  if(max > 0xFFFF0000) max = 0xFFFF0000;
    
  for (int link = 0; link < 10; link++) {
    cerr << "trying link " << link << endl << endl;
  int32_t handle;
  err = CAENVME_Init(cvV2718, link, 0, &handle);
  if(err != cvSuccess){
    cerr<<"Cannot connect to V2718 controller!\n";
    continue;
  }
  CAENVME_SystemReset(handle);
  int dummy=0;
  dummy=system("sleep 1");
  err = CAENVME_BoardFWRelease(handle, message);
  if(err != cvSuccess){
    //this is probably now a caen board...
    cout<<"Error when attempting to communicate with this board\n";
    return 1;
  }
  cout<<"\tFirmware Release: "<<message<<endl;
  err = CAENVME_DriverRelease(handle,message);
  cout<<"\tDriver Release: "<<message<<endl;
  
  for( uint32_t address = min; address <= max; address += 0x10000){
    //cout<<hex<<showbase<<"Checking address "<<address<<"...\n"
    //<<dec<<noshowbase;
    
    
    uint32_t data;
    err = CAENVME_ReadCycle(handle, address+0xF030, &data,cvA32_U_DATA,cvD32);
    if(err != cvSuccess){
      CAENVME_DeviceReset(0);
      continue;
    }
    //print out the board info
    nboards++;
    cout<<"-------------------------------------------\n";
    cout<<"Board found at address "<<hex<<showbase
	     <<address<<dec<<noshowbase<< " link " << link << endl;
    
    cout<<"\tBoard Type: ";
    if(data == 0x10)
      cout<<"V1724LC"<<endl;
    else if(data == 0x11)
      cout<<"V1724"<<endl;
    else if(data == 0x40)
      cout<<"V1724B"<<endl;
    else if(data == 0x12)
      cout<<"V1724C"<<endl;
    else if(data == 0x41)
      cout<<"V1724D"<<endl;
    else if(data == 0x42)
      cout<<"V1724E"<<endl;
    else if(data == 0x43)
      cout<<"V1724F"<<endl;
    else
      cout<<"Unknown board type"<<endl;
    
    
    uint32_t rev0, rev1, rev2, rev3;
    err = CAENVME_ReadCycle(handle, address+0xf04c, &rev0, cvA32_U_DATA,cvD32);
    err = CAENVME_ReadCycle(handle, address+0xf048, &rev1, cvA32_U_DATA,cvD32);
    err = CAENVME_ReadCycle(handle, address+0xf044, &rev2, cvA32_U_DATA,cvD32);
    err = CAENVME_ReadCycle(handle, address+0xf040, &rev3, cvA32_U_DATA,cvD32);
    
    cout<<"Hardware revision: "<<rev0<<" "<<rev1<<" "<<rev2<<" "<<rev3<<"\n";
    err = CAENVME_ReadCycle(handle, address+0x108C, &data, cvA32_U_DATA,cvD32);
    const char* months[13] = {"Jan", "Feb", "Mar", "Apr", "May", "June", 
			      "July", "Aug", "Sep", "Nov", "Dec", "MonthError"};
    uint32_t month = ((data>>24)&0xF);
    if(month > 12) month = 12;
    cout<<"\tChannel AMC Firmware: "<<((data>>8) & 0xFF)<<"."<<(data & 0xFF)
	<<" released "<<((data>>16) & 0xFF)<<" "
	<<months[month]
	<<" 200"<<((data>>28) & 0xF)<<endl;
    err = CAENVME_ReadCycle(handle, address+0x8124, &data, cvA32_U_DATA,cvD32);
    cout<<"\tROC Firmware: "<<((data>>8) & 0xFF)<<"."<<(data & 0xFF)
	<<" released "<<((data>>16) & 0xFF)<<" "
	<<months[((data>>24) & 0xF)]
	<<" 200"<<((data>>28) & 0xF)<<endl;
    err = CAENVME_ReadCycle(handle, address+0xEF04, &data, cvA32_U_DATA,cvD32);
    cout<<"\tVME status: "<<data<<"\n";
    err = CAENVME_ReadCycle(handle, address+0xF088, &data, cvA32_U_DATA,cvD32);
    cout<<"\tVCXO clock: "<<(data ? "1 GHz" : "500 MHz" )<<"\n";
    err = CAENVME_ReadCycle(handle, address+0x8104, &data, cvA32_U_DATA,cvD32);
    cout<<"\tBoard status: "<<(data&0x1FF)<<"\n\t"<<boolalpha
	<<"\tPLL lock: "<<((data>>7)&1)<<"\n\t"
	<<"\tPLL Bypass mode: "<<((data>>6)&1)<<"\n\t"
	<<"\tClock source: "<<(((data>>5)&1) ? "External" : "Internal")<<"\n\t"
	<<"\tReady for run: "<<((data>>8)&1)<<"\n"<<noboolalpha;
  }
  err = CAENVME_End(handle);
  cout<<"****************************************\n";
  cout<<nboards<<" total boards found.\n";
}
  return 0;
}
