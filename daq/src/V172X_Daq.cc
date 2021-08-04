/* --------------------------------------------------------
CAEN_V172XDAQ.cc
This is the implementation for the CAEN_V172XDAQ class, 
which inherits from the WARP_VetoDAQ class.
----------------------------------------------------------*/

#include "V172X_Daq.hh"
#include "V172X_Event.hh"
//#include "CAEN_V172XEvent.hh"
#include "CAENVMElib.h"
#include "CAENDigitizer.h"
#include "RawEvent.hh"
#include "Message.hh"
#include "ConfigHandler.hh"
#include "EventHandler.hh"
#include "TGraph.h"
#include <string>
#include <time.h>
#include <bitset>
#include <algorithm>
#include "boost/ref.hpp"
#include "boost/timer.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include <sstream>
#include <numeric>

//declare some useful constants
const int event_size_padding = 8;
typedef CAEN_DGTZ_ErrorCode ErrC;
enum VME_REGISTERS{
  VME_ChZSThresh =         0x1024,
  VME_ChZSNsamples =       0x1028,
  VME_ChTrigThresh =       0x1080,
  VME_ChTrigSamples =      0x1084,
  VME_ChStatus =           0x1088,
  VME_ChBuffersFull =      0x1094,
  VME_ChDAC =              0x1098,
  VME_ChannelsConfig =     0x8000,
  VME_BufferCode =         0x800C,
  VME_BufferFree =         0x8010,
  VME_CustomSize =         0x8020,
  VME_AcquisitionControl = 0x8100,
  VME_AcquisitionStatus =  0x8104,
  VME_SWTrigger =          0x8108,
  VME_TrigSourceMask =     0x810C,
  VME_TrigOutMask =        0x8110,
  VME_PostTriggerSetting = 0x8114,
  VME_FrontPanelIO =       0x811C,
  VME_ChannelMask =        0x8120,
  VME_DownsampleFactor =   0x8128,
  VME_EventsStored =       0x812C,
  VME_BoardInfo =          0x8140,
  VME_EventSize =          0x814C,
  VME_AlmostFull =         0x816C,
  VME_VMEControl =         0xEF00,
  VME_VMEStatus =          0xEF04,
  VME_BoardID =            0xEF08,
  VME_RelocationAddress =  0xEF10,
  VME_InterruptID =        0xEF14,
  VME_InterruptOnEvent =   0xEF18,
  VME_BLTEvents =          0xEF1C,
  VME_SWReset =            0xEF24,
  VME_SWClear =            0xEF28,
};


void PrintAllRegisters(int handle)
{
  static const uint32_t registers[] = { 
    0x8000, 0x800C, 0x8020, 0x8100, 0x8104, 0x810C, 0x8110, 0x8114, 0x811C, 
    0x8120, 0x8124, 0x812C, 0x8140, 0x814C, 0xEF00, 0xEF04, 0xEF14, 0xEF18, 
    0xEF1C, 0xEF20, 0xEF2C, 0xEF30,
    0xF000, 0xF004, 0xF008, 0xF00C, 0xF010, 0xF014, 0xF018, 0xF01C, 0xF020,
    0xF024, 0xF028, 0xF02C, 0xF030, 0xF034, 0xF038, 0xF03C, 0xF040, 0xF044,
    0xF048, 0xF04C, 0xF080, 0xF084, 0xF088 };
  const int nregisters = sizeof(registers) / sizeof(uint32_t);
  uint32_t data = 0;
  printf("Register map:\n");
  int i=0;
  for( ; i<nregisters; ++i){
    CAENVME_ReadCycle(handle, registers[i], &data, cvA32_U_DATA, cvD32);
    printf("0x%04X:   %08X\n",registers[i], data);
  }
}



V172X_Daq::V172X_Daq() : BaseDaq(), _initialized(false), 
			 _params(), _triggers(0), _vme_mutex()
{
  ConfigHandler::GetInstance()->RegisterParameter(_params.GetDefaultKey(),
						  _params);
  _handle_vme_bridge = 0;
  std::fill (_handle_board, _handle_board + _params.nboards, 0);
}

V172X_Daq::~V172X_Daq()
{
  if (_params.vme_bridge_link >= 0) CAENVME_End(_handle_vme_bridge);

  for(int i=0; i < _params.nboards; i++)
    if(_handle_board[i])
      CAEN_DGTZ_CloseDigitizer(_handle_board[i]);
}

int init_bridge (int link, int node, bool usb, int32_t *handle) {
  CVErrorCodes err = CAENVME_Init(usb ? cvV1718 : cvV2718, link, node, handle);
  if(err != cvSuccess){
    Message m(ERROR);
    m<<"Unable to initialize CAEN VME bridge for link " << link;
    if(usb) m<<" on USB ";
    m<< ": "<<std::endl;
    m<<"\t"<<CAENVME_DecodeError(err)<<std::endl;
    return -1;
  }
  char message[100];
  Message(DEBUG)<<"CAEN VME bridge successfully initialized for link " 
		<< link << "!"<<std::endl;
  CAENVME_BoardFWRelease(*handle,message);
  Message(DEBUG)<<"\tFirmware Release: "<<message<<std::endl;
  CAENVME_DriverRelease(*handle,message);
  Message(DEBUG)<<"\tDriver Release: "<<message<<std::endl;
  Message(INFO)<< "Link " << link << " initialized on handle " 
	       << *handle <<std::endl;
  return 0;
}


int V172X_Daq::Initialize()
{
  if(_initialized){
    Message(WARNING)<<"Reinitializing V172X_Daq..."<<std::endl;
    if (_params.vme_bridge_link >= 0) CAENVME_End(_handle_vme_bridge);

    for(int i=0; i < _params.nboards; i++){
      if(_handle_board[i])
	CAEN_DGTZ_CloseDigitizer(_handle_board[i]);
      _handle_board[i] = 0;
    }
  }
      
  if (_params.vme_bridge_link >= 0) {
    if (init_bridge (_params.vme_bridge_link, 0, false, &_handle_vme_bridge) ){
      _status=INIT_FAILURE;
      return -1;
    }
    //reset everything
    //CAENVME_SystemReset(_handle_vme_bridge);
    //CAENVME_DeviceReset(_handle_vme_bridge);
    if(_params.send_start_pulse) {
      CAENVME_ClearOutputRegister(_handle_vme_bridge, 0xFFFF);
      for(int line = 0; line<5; line++){
	CVErrorCodes err = CAENVME_SetOutputConf(_handle_vme_bridge, 
						 (CVOutputSelect)line, 
						 cvDirect, 
						 cvActiveHigh, cvManualSW);
	if(err != cvSuccess){
	  Message(ERROR)<<"Unable to configure the V2718 output registers.\n";
	  return -2;
	}
      }
    }
  } 
  else if (_params.send_start_pulse) {
    Message(WARNING)<<"send_start_pulse enabled but no bridge link present, "
		    <<"disabling pulses" << std::endl;
    _params.send_start_pulse = 0;
  }

  for(int i=0; i < _params.nboards; i++) {
    if(_params.board[i].enabled){
      CAEN_DGTZ_ConnectionType linkType = _params.board[i].usb ? 
	CAEN_DGTZ_USB : CAEN_DGTZ_OpticalLink;
      if(_params.board[i].usb || _params.board[i].link!=_params.vme_bridge_link)
	_params.board[i].address = 0;
      ErrC err = CAEN_DGTZ_OpenDigitizer(linkType,_params.board[i].link,
					 _params.board[i].chainindex,
					 _params.board[i].address,
					 _handle_board+i);
      if(err != CAEN_DGTZ_Success){
	Message(ERROR)<<"Error "<<err<<" generated while trying to open "
		      <<"board "<<i<<"\n";
	_status = INIT_FAILURE;
	return -3;
      }
      try{
	if(InitializeBoard(i)){
	  _status = INIT_FAILURE;
	  return -5;
	}
      }
      catch(std::exception& error){
	Message(EXCEPTION)<<error.what()<<std::endl;
	return -6;
      }
    }
  } 
  Message(INFO)<<"CAEN DAQ initialized\n";
  _initialized = true;

  return Update();
}

int V172X_Daq::InitializeBoard(int boardnum)
{
  V172X_BoardParams& board = _params.board[boardnum];
  int32_t handle = _handle_board[boardnum];
  
  //WriteDigitizerRegister(VME_SWReset, 1, handle);
  CAEN_DGTZ_Reset(handle);
  uint32_t data = ReadDigitizerRegister(VME_BoardInfo, handle);
  board.board_type = (BOARD_TYPE)(data&0xFF);
  board.nchans = (data>>16)&0xFF;
  board.mem_size = (data>>8)&0xFF;
  if(board.UpdateBoardSpecificVariables()){ //returns -1 on error
    Message(CRITICAL)<<"Board "<<boardnum<<" with address "
		     <<std::hex<<std::showbase
		     <<board.address<<std::dec<<std::noshowbase
		     <<" is not a V172X digitizer!"<<std::endl;
    _status = INIT_FAILURE;
    return -2;
  }
  else{
    Message(DEBUG)<<"Successfully initialized board "<<boardnum
		  <<" with address "<<std::hex<<std::showbase<<board.address
		  <<std::dec<<std::noshowbase<<" with "
		  <<board.mem_size<<"MB/ch memory."<<std::endl;
  }
  WriteDigitizerRegister(VME_BoardID,
		   board.id, handle);
  WriteDigitizerRegister(VME_InterruptID,
		   board.id, handle);
  return 0;
}

int V172X_Daq::waitforstable(int32_t handle, int channel)
{
  int tries = 0;
  uint32_t status = ReadDigitizerRegister(VME_ChStatus +channel*0x100, handle);
  while((status&0x4) || !(status&0x2)){
    if(++tries > 200){
      Message(ERROR)<<"Channel "<<channel<<" will not stabilize offset!\n";
      return tries;
    }
    Message(DEBUG2)<<"Waiting for channel "<<channel<<" to stabilize. "
		   <<"Status is "<<std::hex<<status<<"\n";
    boost::this_thread::sleep(boost::posix_time::millisec(50));
    ReadDigitizerRegister(VME_ChStatus +channel*0x100, handle);
  }
  return 0;
}

int V172X_Daq::CalibrateBaselines(int boardnum)
{
  V172X_BoardParams& board = _params.board[boardnum];
  uint32_t calibmask = 0;
  std::map<int,TGraph*> interp;
  for(int ch=0; ch<board.nchans; ++ch){
    if(board.channel[ch].calibrate_baseline){
      calibmask |= (1<<ch);
      interp[ch] = new TGraph(_params.basecalib_max_tries);
      board.channel[ch].dc_offset = 0xffff*(1.-board.channel[ch].target_baseline/
					    pow(2,board.sample_bits));
    }
  }
  if(calibmask == 0) //nothing to do
    return 0;
  
  Message(INFO)<<"Calibrating baselines for board "<<boardnum<<", please wait\n";
  
  //set the nsamps, trigger, and enable masks
  int handle = _handle_board[boardnum];
  CAEN_DGTZ_ClearData(handle);
  WriteDigitizerRegister(VME_TrigSourceMask,(1<<31),handle); //SW trigger only
  WriteDigitizerRegister(VME_TrigOutMask,0,handle); //no trigger outs
  WriteDigitizerRegister(VME_ChannelMask,calibmask,handle);
  CAEN_DGTZ_SetRecordLength(handle,_params.basecalib_samples);
  CAEN_DGTZ_GetRecordLength(handle,&(_params.basecalib_samples));
  CAEN_DGTZ_SetAcquisitionMode(handle,CAEN_DGTZ_SW_CONTROLLED);
  CAEN_DGTZ_SetMaxNumEventsBLT(handle,1);
  int tries = 0;
  char* buffer = 0;
  uint32_t bufsize = 0;
  CAEN_DGTZ_MallocReadoutBuffer(handle,&buffer, &bufsize);

  while(tries < _params.basecalib_max_tries && calibmask){
    Message(DEBUG)<<"Attempt "<<tries<<" to calibrate board "<<boardnum<<" baselines\n";
    for(int ch=0; ch<board.nchans; ++ch){
      if(calibmask & (1<<ch)){
	CAEN_DGTZ_SetChannelDCOffset(handle,ch,board.channel[ch].dc_offset);
	waitforstable(handle,ch);
	interp[ch]->SetPoint(tries,0,board.channel[ch].dc_offset);
      }
    }
    //wait until the board is ready to take data
    boost::this_thread::sleep(boost::posix_time::millisec(1500));
    uint32_t status = 0;
    int count = 0;
    while( !((status&0x100) && (status&0xc0)) ){
      status = ReadDigitizerRegister(VME_AcquisitionStatus, handle);
      if(count++ > 500){
	Message(ERROR)<<"Unable to initialize board "<<boardnum<<" at address "
		      <<std::hex<<board.address<<std::dec<<"\n";
	return 1;
      }
    }
      
    CAEN_DGTZ_SWStartAcquisition(handle);
    for(uint32_t trig=0; trig < _params.basecalib_triggers; ++trig){
      CAEN_DGTZ_SendSWtrigger(handle);
      while(CAEN_DGTZ_ReadData(handle,
			       CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
			       buffer,&bufsize) != 0){}
      V172X_BoardData data((const unsigned char*)buffer);
      for(int ch=0; ch<board.nchans; ++ch){
	if(calibmask & (1<<ch)){
	  double sum = 0;
	  if(board.bytes_per_sample == 1)
	    sum = std::accumulate((uint8_t*)data.channel_start[ch],
				  (uint8_t*)data.channel_end[ch],0);
	  else if(board.bytes_per_sample == 2)
	    sum = std::accumulate((uint16_t*)data.channel_start[ch],
				  (uint16_t*)data.channel_end[ch],0);
	  else
	    sum = std::accumulate((uint32_t*)data.channel_start[ch],
				  (uint32_t*)data.channel_end[ch],0);
	  interp[ch]->GetX()[tries] += sum/_params.basecalib_samples/
	    _params.basecalib_triggers;
	}
      }
    }
    CAEN_DGTZ_SWStopAcquisition(handle);
    //check the new baselines, see which channels need adjusting still
    for(int ch=0; ch<board.nchans; ++ch){
      if(calibmask & (1<<ch)){
	V172X_ChannelParams& cpar = board.channel[ch];
	cpar.final_baseline = interp[ch]->GetX()[tries];
	Message(DEBUG2)<<ch<<" "<<cpar.dc_offset<<" "
		       <<std::dec<<cpar.final_baseline<<"\n";
	if(std::abs(cpar.final_baseline - cpar.target_baseline) < 
	   cpar.allowed_baseline_offset){
	  //we're done with this channel!
	  calibmask &= ~(1<<ch);
	}
	else{
	  //prevent being stuck at edge
	  if(cpar.final_baseline <1)
	    cpar.dc_offset -= 0x1000;
	  else if(cpar.final_baseline > pow(2,board.sample_bits)-1)
	    cpar.dc_offset += 0x1000;
	  if(tries >1)
	    cpar.dc_offset = interp[ch]->Eval(cpar.target_baseline);
	  else{
	    //need to make some guesses here
	    double ratio = pow(2,16.-board.sample_bits);
	    double newval = 1.*cpar.dc_offset - 
	      ratio*(cpar.target_baseline - cpar.final_baseline);
	    if(newval > 0xffff) newval = 0xffff;
	    if(newval < 0) newval = 0;
	    cpar.dc_offset = newval;
	  }
	}
      }
    }
    ++tries;
  }
  Message m(INFO);
  m<<"Final baselines for board "<<boardnum<<":\nCh\tDCval\tBaseline\n";
  for(int ch=0; ch < board.nchans; ++ch){
    if(!board.channel[ch].calibrate_baseline)
      continue;
    m<<ch<<"\t"<<board.channel[ch].dc_offset<<"\t"<<board.channel[ch].final_baseline
     <<"\n";
    delete interp[ch];
  }
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_ClearData(handle);
  return calibmask;
}


int V172X_Daq::Update()
{
  if(!_initialized){
    Message(ERROR)<<"Attempt to update parameters before initializations!"
		  <<std::endl;
    return -1;
  }
  if(_is_running){
    Message(ERROR)<<"Attempt to update parameters while in run."
		  <<std::endl;
    return -2;
  }
  try{
    for(int iboard=0; iboard < _params.nboards; iboard++){
      V172X_BoardParams& board = _params.board[iboard];
      if(!board.enabled) 
	continue;
      if(board.downsample_factor < 1)
	board.downsample_factor=1; 
      if(board.zs_type != NONE){
	Message(INFO)<<"Software downsampling not enabled for "
		     <<"zero suppressed data\n";
	board.downsample_factor = 1;
      }
      
      //calibrate dc offsets first!
      int ret = CalibrateBaselines(iboard);
      if(ret) return ret;
      
      int handle = _handle_board[iboard];
      //determine the trigger acquisition window for the database
      //WARNING: Assumes it is the same for all boards!!!
      runinfo* info = EventHandler::GetInstance()->GetRunInfo();
      if(info){
	std::stringstream ss;
	ss<<"board"<<iboard<<"__pre_trigger_time_us";
	info->SetMetadata(ss.str(), board.pre_trigger_time_us);
	ss.str("");
	ss<<"board"<<iboard<<"__post_trigger_time_us";
	info->SetMetadata(ss.str(), board.post_trigger_time_us);
	
      }
       
      uint32_t channel_mask = 0;
      uint32_t trigger_mask = 0;
      uint32_t trigger_out_mask = 0;
      //need to know total_nsamps to estimate event size
      //do the per-channel stuff
      for(int i=0; i<board.nchans; i++){
	V172X_ChannelParams& channel = board.channel[i];
	channel_mask += (1<<i) * channel.enabled;
	uint32_t trigmaskbit = (1<<i);
	if( (board.board_type == V1730) || (board.board_type == V1725) )
	  trigmaskbit = (1<<(i/2));
	trigger_mask |= (trigmaskbit * channel.enable_trigger_source);
	trigger_out_mask |= (trigmaskbit * channel.enable_trigger_out);
	//write the per-channel stuff
	//Zero suppression threshold
	uint32_t zs_thresh = (1<<31) * channel.zs_polarity +
	  channel.zs_threshold;
	WriteDigitizerRegister(VME_ChZSThresh+i*0x100,zs_thresh, handle);
	//zero suppression time over threshold
	uint32_t nsamp = channel.zs_thresh_time_us * board.GetSampleRate();
	if(nsamp >= (1<<20)) nsamp = (1<<20) -1;
	if(board.zs_type == ZLE){
	  //nsamp contains the pre and post samples
	  if(channel.zs_pre_samps>=(1<<16)) channel.zs_pre_samps = (1<<16)-1;
	  if(channel.zs_post_samps>=(1<<16)) channel.zs_post_samps = (1<<16)-1;
	  uint32_t npre = 
	    std::ceil(channel.zs_pre_samps/board.stupid_size_factor);
	  uint32_t npost = 
	    std::ceil(channel.zs_pre_samps/board.stupid_size_factor);
	  nsamp = (npre<<16) + npost;
	}
	WriteDigitizerRegister(VME_ChZSNsamples+i*0x100, nsamp, handle);
	//trigger threshold
	WriteDigitizerRegister(VME_ChTrigThresh+i*0x100, 
			 channel.threshold, handle);
	
	//time over trigger threhsold
	nsamp = std::ceil(channel.thresh_time_us * board.GetSampleRate()) 
	  / board.stupid_size_factor;
	if(nsamp >= (1<<12)) nsamp = (1<<12) - 1;
	WriteDigitizerRegister(VME_ChTrigSamples+i*0x100, nsamp, handle);
	//dc offset
	waitforstable(handle,i);
	CAEN_DGTZ_SetChannelDCOffset(handle,i, channel.dc_offset);
	waitforstable(handle,i);
	//WriteDigitizerRegister(VME_ChDAC+i*0x100, channel.dc_offset, handle);

	//this register is now ‘Self-Trigger logic’, set all to OR
	if (board.board_type == V1730 || board.board_type == V1725) {    
    		for(int i=0; i<board.nchans; ++i) {
        		if(i%2 == 0) WriteDigitizerRegister(VME_ChTrigSamples + i*0x100, 3, handle);
    		}
	}
 

      } // end of channel loop

      //finish up with the board parameters
      uint32_t channel_config = (1<<16) * board.zs_type + 
	(1<<6) * board.trigger_polarity + 
	(1<<4) + //Memory Sequential access
	(1<<3) * board.enable_test_pattern + 
	(1<<1) * board.enable_trigger_overlap;
      WriteDigitizerRegister(VME_ChannelsConfig, channel_config, handle);
      //Buffer code (determines total trigger time
      WriteDigitizerRegister(VME_BufferCode, board.GetBufferCode(), handle);
      //Custom size of register
      
      WriteDigitizerRegister(VME_CustomSize, 
		       board.GetCustomSizeSetting(), handle);
      
      //almost full register (affects busy relative to full signal)
      //not sure if it's subtractive or absolute, so try 1 now
      int nbuffers = board.GetTotalNBuffers();
      int reserve = board.almostfull_reserve;
      int almostfull = nbuffers - reserve;
      
      if(reserve == 0)
	almostfull = 0;
      else if(almostfull <= 0){
	if(nbuffers == 1){
	  Message(WARNING)<<"Requested almostfull_reserve "<<reserve
			  <<"but only 1 buffer available! Disabling.\n";
	  almostfull = 0;
	}
	else{
	  Message(WARNING)<<"Requested almostfull_reserve "<<reserve
			  <<" but only "<<nbuffers<<" total buffers!\n\t"
			  <<"AlmostFull level will be set to 1.\n";
	  almostfull = 1;
	}
      }
      if(almostfull > 0){
	Message(DEBUG)<<"BUSY will be asserted when "<<almostfull
		      <<" buffers are filled.\n";
      }
      WriteDigitizerRegister(VME_AlmostFull,
		       almostfull, handle);
      //Acquisition Control
      uint32_t acq_control =  
	(1<<3) * board.count_all_triggers +
	_params.send_start_pulse;
//      acq_control = (1<<3) * board.count_all_triggers + 4;
      WriteDigitizerRegister(VME_AcquisitionControl,acq_control, handle);
      board.acq_control_val = acq_control;
      //trigger mask
      if(board.local_trigger_coincidence >7) 
	board.local_trigger_coincidence = 7;
      if(board.coincidence_window_ticks > 0xF)
	board.coincidence_window_ticks = 0xF;
      trigger_mask += (1<<31) * board.enable_software_trigger 
	+ (1<<30) * board.enable_external_trigger
	+ (1<<24) * board.local_trigger_coincidence
	+ (1<<20) * board.coincidence_window_ticks;
      WriteDigitizerRegister(VME_TrigSourceMask, trigger_mask, handle);
      //trigger out mask
      trigger_out_mask += (1<<31) * board.enable_software_trigger_out +
	(1<<30) * board.enable_external_trigger_out;
      if(board.trigout_coincidence > 0){
	if(board.trigout_coincidence > 7)
	  board.trigout_coincidence = 7;
	trigger_out_mask += (2<<8) + (board.trigout_coincidence << 10);
      }
      WriteDigitizerRegister(VME_TrigOutMask, trigger_out_mask, handle);
      //post trigger setting
      
      WriteDigitizerRegister(VME_PostTriggerSetting, 
		       board.GetPostTriggerSetting(), handle);
      //signal logic and front panel programming
      uint32_t trgoutmask = 0;
      if(board.trgout_mode == BUSY)
	trgoutmask = 0xD; //1101
      uint32_t fpio = board.signal_logic //NIM or TTL
	/*| (1<<6) //programmed IO*/
	| (trgoutmask<<16); //trgoutsetting (bits 16-19)

      WriteDigitizerRegister(VME_FrontPanelIO,
		       fpio, handle);
      //channel mask
      WriteDigitizerRegister(VME_ChannelMask, channel_mask, 
		       handle);
      //VME control
      uint32_t vme_control = (1<<5) * _params.align64 + 
	(1<<4) + //enable bus error
	(1<<3) + //enable optical link error
	( board.usb ? 0 : 1 ); //interrupt level
      WriteDigitizerRegister(VME_VMEControl, vme_control, handle);
      //Interrupt num, BLT event num
      WriteDigitizerRegister(VME_InterruptOnEvent, 0, handle);
      WriteDigitizerRegister(VME_BLTEvents, 1, handle);
      //wait until the board is ready to take data
      uint32_t status = 0;
      int count = 0;
      while( !((status&0x100) && (status&0xc0)) ){
	status = ReadDigitizerRegister(VME_AcquisitionStatus, handle);
	Message(DEBUG2)<<"Board "<<board.id<<" reporting status "
		       <<std::hex<<status<<"\n";
	if(count++ > 500){
	  Message(ERROR)<<"Unable to initialize board "<<iboard<<" at address "
			<<std::hex<<board.address<<std::dec<<"\n";
	  return 1;
	}
      }
      //this ensures the digitizer re-calculates event size in case
      // a buffer was previously allocated
      char* dummy = 0;
      uint32_t dummysize = 0;
      CAEN_DGTZ_MallocReadoutBuffer(handle, &dummy, &dummysize);
      CAEN_DGTZ_FreeReadoutBuffer(&dummy);
      
    }
    /*Find the max expected event size in bytes
    The header size is 16 bytes per board
    The data size is 2 bytes per sample per channel
    */
    Message(DEBUG)<<"The expected event size is "<<_params.GetEventSize(false)
		  <<" bytes."<<std::endl;
    runinfo* info = EventHandler::GetInstance()->GetRunInfo();
    if(info){
      info->SetMetadata("nchans",_params.GetEnabledChannels());
      info->SetMetadata("event_size",_params.event_size_bytes);
    }

    //wait 2 seconds for DC offset levels to adjust
    boost::this_thread::sleep(boost::posix_time::millisec(1500));
  }
  catch(...){ 
    return -3;
  }
  return 0;
}

//predicate for find_if function used to test if any board has data
bool DataAvailable(uint32_t status)
{
  return (status & 0x8);
}

class Average{
 private: 
  unsigned factor;
  unsigned bytes;
  char* start;
  uint32_t mask;
 public:
  Average(char* _start, unsigned _factor, unsigned _bytes) : 
    factor(_factor), bytes(_bytes), start(_start) { mask = (1<<(8*bytes))-1; }
  uint32_t operator()(){
    uint32_t out = 0;
    for(size_t i=0; i<factor; ++i){
      out += (*(uint32_t*)(start)) & mask;
      start += bytes;
    }
    return out / factor;
  }
};

int DownsampleEvent(unsigned char* buf, uint32_t& factor, long& eventsize, 
		    int bytes_per_sample)
{
  V172X_BoardData board(buf);
  if(board.zle_enabled){
    factor = 1;
    return 1;
  }
  int nsamps = 0;
  for(int i=0; i<board.nchans; ++i){
    if(board.channel_mask & (1<<i)){
      nsamps = (board.channel_end[i] - board.channel_start[i])/bytes_per_sample;
      break;
    }
  }
  if(nsamps <=0)
    return 2;
  //should divide evenly!
  if(nsamps % factor){
    Message(WARNING)<<"Cannot downsample "<<nsamps<<" samples evenly by "
		    <<factor<<"; reducing automatically.\n";
    while(--factor > 1 && (nsamps%factor)) {}
    if(factor <= 1){
      factor = 1;
      return 3;
    }
    Message(INFO)<<"New downsample factor is "<<factor<<"\n";
  }
  //now actually do the division
  eventsize = 16;
  for(int i=0; i<board.nchans; ++i){
    if(board.channel_start[i]){
      Average avg(board.channel_start[i], factor, bytes_per_sample);
      if(bytes_per_sample == 1)
	std::generate((uint8_t*)(buf+eventsize),
		      (uint8_t*)(buf+eventsize+nsamps/factor*bytes_per_sample),
		      avg);
      else if(bytes_per_sample < 3)
	std::generate((uint16_t*)(buf+eventsize),
		      (uint16_t*)(buf+eventsize+nsamps/factor*bytes_per_sample),
		      avg);
      else
	std::generate((uint32_t*)(buf+eventsize),
		      (uint32_t*)(buf+eventsize+nsamps/factor*bytes_per_sample),
		      avg);
      eventsize += nsamps / factor * bytes_per_sample;

    }
  }
  //update the event size in the raw event buffer
  ((uint32_t*)(buf))[0] = 0xA0000000 + (eventsize / sizeof(uint32_t));
  return eventsize;
}


void V172X_Daq::DataAcquisitionLoop()
{
  if(!_initialized) Initialize();
  //prepare some variables
  _triggers = 0;
  

  int32_t irq_handle = _handle_vme_bridge;
  if (_params.vme_bridge_link) 
    for(int i=0; i < _params.nboards; i++) { 
      irq_handle = _handle_board[i]; 
      if (_params.board[i].enabled && _params.board[i].link >= 0) break;
    }
  
  //figure out whether we can use interrupts
  //usb interface can't do interrupts, so use polling only
  
  bool use_interrupt = true;
  for(int i=0; i<_params.nboards; i++){
    if(_params.board[i].enabled && _params.board[i].usb) 
      use_interrupt = false;
  }

  //Arm the boards
  try{
    std::vector<uint32_t> acq_write;
    for(int i=0; i<_params.nboards; i++){
      if(_params.board[i].enabled){
        WriteDigitizerRegister(VME_SWClear,1,_handle_board[i]);
        WriteDigitizerRegister(VME_AcquisitionControl,
                               _params.board[i].acq_control_val+0x4,
                               _handle_board[i]);
      }
    }
    if(_params.send_start_pulse)
      CAENVME_SetOutputRegister(_handle_vme_bridge, 
                                cvOut0Bit | cvOut1Bit | cvOut2Bit | 
                                cvOut3Bit | cvOut4Bit );
  }
  catch(std::exception& e){
    Message(ERROR)<<"Unable to arm the board for run!\n";
    _initialized=false;
    _status=INIT_FAILURE;
    return;
  }
  
  //and we're running!
  while(_is_running ){

    CVErrorCodes err = cvTimeoutError;
    
    if(use_interrupt){
      //Enable IRQ lines and wait for an interrupt
      CAENVME_IRQEnable(irq_handle,0xFF);
      err = CAENVME_IRQWait(irq_handle,0xFF,
                            _params.trigger_timeout_ms);
      CAENVME_IRQDisable(irq_handle,0xFF);
    }
    else{
      for(int i=0; i<_params.nboards; i++){
        if(!_params.board[i].enabled) continue;
        //see if there is an event ready on the board
        if(DataAvailable(ReadDigitizerRegister(VME_AcquisitionStatus, 
                                               _handle_board[i])) ){
          err = cvSuccess;
          break;
        }
      }
    }

    //Check to see if there was an interrupt or just the timeout
    switch(err){
    case cvSuccess:
      break;
    case cvGenericError:
      Message(DEBUG)<<"Generic error occurred. Probably just a timeout...\n";
      //notice: no break here!
    case cvTimeoutError:
      if(!use_interrupt){
        //sleep for the timeout interval
        boost::this_thread::sleep(
           boost::posix_time::millisec(_params.trigger_timeout_ms));
      }
      if(_params.auto_trigger){
        Message(DEBUG)<<"Triggering...\n";
        for(int i=0; i<_params.nboards; i++){
          if(!_params.board[i].enabled) continue;
          WriteDigitizerRegister(VME_SWTrigger,1,_handle_board[i]);
        }
      }
      else
        Message(DEBUG)<<"Waiting for trigger..."<<std::endl;
      continue;
      break;
    default:
      Message(ERROR)<<"Unknown error waiting for trigger interrupt\n";
      break;
    }
    
    //if we get here, there is an event ready for download
    //get a new event ready 
    RawEventPtr next_event(new RawEvent);
    size_t blocknum = 
      next_event->AddDataBlock(RawEvent::CAEN_V172X,
                               _params.event_size_bytes+event_size_padding);
    unsigned char* buffer = next_event->GetRawDataBlock(blocknum);
    const uint32_t UNSET_EVENT_COUNTER = 0xFFFFFFFF;
    uint32_t event_counter = UNSET_EVENT_COUNTER;
    //get the data
    int data_transferred = 0;
    for(int i=0; i<_params.nboards; i++){
      if(!_params.board[i].enabled) continue;
      //wait until the event is ready on the board
      
      int tries = 0;
      const int maxtries = 50;
      while(!DataAvailable(ReadDigitizerRegister(VME_AcquisitionStatus, 
                                                 _handle_board[i])) &&
            tries++ < maxtries) {}
      if(tries >= maxtries){
        Message(DEBUG)<<"No trigger received on board "<<i<<"\n";
        continue;
      }
      
      uint32_t this_dl_size = 0;
      tries=0;
      ErrC err = CAEN_DGTZ_Success;
      while(this_dl_size == 0 && ++tries<maxtries ){
        err = CAEN_DGTZ_ReadData(_handle_board[i], 
                                 CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                                 (char*)(buffer+data_transferred), 
                                 &this_dl_size);
        if(err != CAEN_DGTZ_Success){
          Message(ERROR)<<"Error generated while downloading event from board"
                        <<i<<": "<<err<<"\n";
          continue;
        }
      }
      
      if(this_dl_size==0){
        Message(ERROR)<<"0 bytes downloaded for board "<<i<<std::endl;
        Message(DEBUG)<<"Events stored on this board: "
                      <<ReadDigitizerRegister(VME_EventsStored, 
                                              _handle_board[i])
                      <<"\n";
        uint32_t out;
        out = ReadDigitizerRegister(VME_VMEStatus, _handle_board[i]);
        Message(DEBUG)<<"VME status:"
                      <<"\n\tBERR flag: "<< (out&4)
                      <<"\n\tOutput buffer full: "<< (out&2)
                      <<"\n\tData ready: "<< (out&1)<<"\n";
        out = ReadDigitizerRegister(VME_AcquisitionStatus, 
                                    _handle_board[i]);
        Message(DEBUG)<<"Acquisition status:"
                      <<"\n\tReady for acquisition: "<< (out&256)
                      <<"\n\tPLL Status: "<< (out&128)
                      <<"\n\tPLL Bypass: "<< (out&64)
                      <<"\n\tClock source: "<< (out&32)
                      <<"\n\tEvents full: "<< (out&16)
                      <<"\n\tEvent ready: "<< (out&8)
                      <<"\n\t Run on: "<< (out&4) <<"\n";
        out = ReadDigitizerRegister(VME_EventSize, _handle_board[i]);
        Message(DEBUG)<<"Next event size: "<<out<<"\n";
        Message(DEBUG)<<"Expected event size "
                      <<_params.board[i].event_size_bytes/4<<"\n";
        
        
        //free the buffer so we don't re-trigger spuriously
        WriteDigitizerRegister(VME_BufferFree,1, 
                               _handle_board[i]);
        Message(DEBUG)<<"Events stored on this board: "
                      <<ReadDigitizerRegister(VME_EventsStored,_handle_board[i])
                      <<"\n";
        WriteDigitizerRegister(VME_BufferFree,2, 
                               _handle_board[i]);
        Message(DEBUG)<<"Events stored on this board: "
                      <<ReadDigitizerRegister(VME_EventsStored,_handle_board[i])
                      <<"\n";
        Message(ERROR)<<"Boards don't usually recover from this error! "
                      <<"Aborting!\n";
        _status = COMM_ERROR;
        _is_running = false;
        break;
      }
      else{
        
        //this could ignore potential align64 extra bits:
        //data_transferred += this_dl_size;
        //instead, we check the actual event
        long ev_size = (*((uint32_t*)(buffer+data_transferred)) & 
                        0x0FFFFFFF) * sizeof(uint32_t);
        if(std::abs(ev_size - (long)this_dl_size) > 5){
          Message(WARNING)<<"Event size does not match download count!\n\t"
                          <<"Event size: "<<ev_size<<"; download size: "
                          <<this_dl_size<<"; requested download "
                          <<_params.board[i].event_size_bytes<<std::endl;
          Message(ERROR)<<"Boards don't usually recover from this error! "
                        <<"Aborting!\n";
          _status = COMM_ERROR;
          _is_running = false;
          break;
        }
        //check for event ID misalignment
        uint32_t evct = (((uint32_t*)(buffer+data_transferred))[2])&0xFFFFFF;
        if(event_counter == UNSET_EVENT_COUNTER)
          event_counter = evct;
        else if(evct != event_counter){
          Message(CRITICAL)<<"Mismatched event ID on board "<<i
                           <<"; received "<<evct<<", expected "<<event_counter
                           <<"; Aboring run\n";
          _status = GENERIC_ERROR;
          _is_running=false;
          break;
        }
        
        if(_params.board[i].downsample_factor > 1){
          DownsampleEvent(buffer+data_transferred, 
                          _params.board[i].downsample_factor, ev_size, 
                          _params.board[i].bytes_per_sample);
          
        }
        
        data_transferred += ev_size;
        
      }
    }
    if(GetStatus() != NORMAL){
      _is_running = false;
      break;
    }
    if(data_transferred <= 0){
      Message(ERROR)<<"No boards transferred usable data this event!\n";
      continue;
    }
    else{
      _triggers++;
      next_event->SetDataBlockSize(blocknum, data_transferred);
      PostEvent(next_event);
    }    
  }//end while(_is_running)
   //We only reach here once the event has stopped, so clean up any mess
  //First, end the run, and clear the buffers
  if(_params.send_start_pulse)
    CAENVME_ClearOutputRegister(_handle_vme_bridge, 0xFFFF);
  for(int i=0; i<_params.nboards; i++){
    if(_params.board[i].enabled){
      WriteDigitizerRegister( VME_AcquisitionControl,
		       _params.board[i].acq_control_val, _handle_board[i]);
      WriteDigitizerRegister(VME_SWClear,0x1, _handle_board[i]);
      WriteDigitizerRegister(VME_SWReset,0x1, _handle_board[i]);
    }
  }
  Message(DEBUG)<<_triggers<<" total triggers downloaded."<<std::endl;
}
