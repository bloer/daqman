#include "V172X_Daq.hh"
#include "CAENVMElib.h"
#include "CAENDigitizer.h"
#include "Message.hh"
//#include "exstream.hh"
#include <exception>
#include <stdexcept>

//change this if CAENVME_MultiWrite/Read starts working...
//DO NOT CHANGE since now several handles are used one for each board
const bool stupid = true;
typedef CAEN_DGTZ_ErrorCode ErrC;
CVAddressModifier add_mod[20] = { cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA,cvA32_U_DATA,cvA32_U_DATA,
				  cvA32_U_DATA, cvA32_U_DATA };
CVDataWidth data_width[20] = { cvD32,cvD32,cvD32,cvD32,cvD32,cvD32,
			       cvD32,cvD32,cvD32,cvD32,cvD32,cvD32,
			       cvD32,cvD32,cvD32,cvD32,cvD32,cvD32,
			       cvD32,cvD32, };

//using the new CAENDigitizer libraray
void V172X_Daq::WriteDigitizerRegister(uint32_t address, uint32_t write_me,
				       int32_t handle) throw(std::runtime_error)
{
  ErrC err = CAEN_DGTZ_WriteRegister(handle, address, write_me);
  if(err != CAEN_DGTZ_Success){
    _status = COMM_ERROR;
    Message e(EXCEPTION);
    e <<" Error "<<err<<" writing to digitizer register "
      <<std::hex<<std::showbase<<address<<"\n";
    throw std::runtime_error(e.str());
  }
}

uint32_t V172X_Daq::ReadDigitizerRegister(uint32_t address, int32_t handle) throw(std::runtime_error)
{
  uint32_t data = 0;
  ErrC err = CAEN_DGTZ_ReadRegister(handle, address, &data);
  if(err != CAEN_DGTZ_Success){
    _status = COMM_ERROR;
    Message e(EXCEPTION);
    e <<" Error "<<err<<" reading from digitizer register "
      <<std::hex<<std::showbase<<address<<"\n";
    throw std::runtime_error(e.str());
  }
  return data;
}

void V172X_Daq::WriteVMERegister(uint32_t address, 
				 uint32_t write_me, int32_t handle) throw(std::runtime_error)
{
  CVErrorCodes err = CAENVME_WriteCycle(handle,
					address,
					&write_me, add_mod[0], data_width[0]);
//  Message(INFO) << "writing on handle " <<  handle << " addreess " << std::hex << address << std::endl;
  if(err != cvSuccess){
    switch(err){
    case cvBusError:
      _status = BUS_ERROR;
      break;
    case cvCommError:
      _status = COMM_ERROR;
      break;
    default:
      _status = GENERIC_ERROR;
    }
    Message e(EXCEPTION);
    e <<" Error writing to VME Register "
      <<std::hex<<std::showbase<<address
      <<std::dec<<std::noshowbase<<": "
      <<CAENVME_DecodeError(err)<<std::endl;
    throw std::runtime_error(e.str());
  }
  
}
  
uint32_t V172X_Daq::ReadVMERegister(uint32_t address, int32_t handle) throw(std::runtime_error)
{
  uint32_t data=0;
  CVErrorCodes err = CAENVME_ReadCycle(handle,
				       address,
				       &data, add_mod[0], data_width[0]);
  
//  Message(INFO) << "reading on handle " <<  handle << " addreess " << std::hex << address << std::endl;
  if(err != cvSuccess){
    switch(err){
    case cvBusError:
      _status = BUS_ERROR;
      break;
    case cvCommError:
      _status = COMM_ERROR;
      break;
    default:
      _status = GENERIC_ERROR;
    }
    Message e(EXCEPTION);
    e <<" Error reading from VME Register "
      <<std::hex<<std::showbase<<address<<": "
      <<std::dec<<std::noshowbase<<CAENVME_DecodeError(err)
      <<std::endl;
    throw std::runtime_error(e.str());
  }
  return data;
}

void V172X_Daq::WriteVMERegisters(uint32_t address, uint32_t write_me) throw(std::runtime_error)
{
  const int n = _params.enabled_boards;
  uint32_t full_address[V172X_Params::nboards];
  uint32_t data[V172X_Params::nboards];
  CVErrorCodes ecodes[V172X_Params::nboards];
  uint32_t handles[V172X_Params::nboards];
  int board = 0;
  for(int i=0; i< _params.nboards; i++){
    if(_params.board[i].enabled){
      full_address[board] = _params.board[i].address+address;
      data[board] = write_me;
      ecodes[board]=cvSuccess;
      handles[board]=_handle_board[i];
      board++;
    }
  }
  CVErrorCodes err;
  if(!stupid){
    //This doesn't seem to work...
    err = CAENVME_MultiWrite(_handle_vme_bridge, full_address, data, n, 
			     add_mod, data_width, ecodes);
  }
  else{
    err = cvSuccess;
    for(int i=0; i<n; i++) {
       ecodes[i] = CAENVME_WriteCycle(handles[i],full_address[i],data + i,
			       add_mod[i],data_width[i]);
      if (ecodes[i] != cvSuccess) err = ecodes[i];
    }
  }
  if(err != cvSuccess){
    switch(err){
    case cvBusError:
      _status = BUS_ERROR;
      break;
    case cvCommError:
      _status = COMM_ERROR;
      break;
    default:
      _status = GENERIC_ERROR;
    }
    Message e(EXCEPTION);
    e <<" Errors writing to VME Registers: "<<CAENVME_DecodeError(err)<<"\n";
    for(int i=0; i<n; i++){
      e<<"\t"<<std::hex<<std::showbase<<full_address[i]<<" "<<data[i]<<": "
       <<"\t"<<std::dec<<std::noshowbase<<CAENVME_DecodeError(ecodes[i])
       <<"\t"<<std::endl;
    }
    throw std::runtime_error(e.str());
  }
}

void V172X_Daq::WriteVMERegisters(uint32_t address, uint32_t* write_me) throw(std::runtime_error)
{
  const int n = _params.enabled_boards;
  uint32_t full_address[V172X_Params::nboards];
  CVErrorCodes ecodes[V172X_Params::nboards];
  uint32_t handles[V172X_Params::nboards];
  int board = 0;
  for(int i=0; i< _params.nboards; i++){
    if(_params.board[i].enabled){
      full_address[board] = _params.board[i].address+address;
      ecodes[board] = cvSuccess;
      handles[board]=_handle_board[i];
      board++;
    }
  }
  CVErrorCodes err;
  if(!stupid){
    //This doesn't seem to work...
    err = CAENVME_MultiWrite(_handle_vme_bridge, full_address, write_me, n, 
			     add_mod, data_width, ecodes);
  }
  else{
    err = cvSuccess;
    for(int i=0; i<n; i++) {
      ecodes[i] = CAENVME_WriteCycle(handles[i],full_address[i],write_me + i,
			       add_mod[i],data_width[i]);
  //    Message(INFO) << "writing on handle " <<  handles[i] << " addreess " << std::hex << full_address[i] << std::endl;
      if (ecodes[i] != cvSuccess) err = ecodes[i];
    }
  }
  
  if(err != cvSuccess){
    switch(err){
    case cvBusError:
      _status = BUS_ERROR;
      break;
    case cvCommError:
      _status = COMM_ERROR;
      break;
    default:
      _status = GENERIC_ERROR;
    }
    Message e(EXCEPTION);
    e <<" Errors writing to VME Registers:\n";
    for(int i=0; i<n; i++){
      e<<"\t"<<std::hex<<std::showbase<<full_address[i]<<" "<<write_me[i]<<": "
       <<"\t"<<std::dec<<std::noshowbase<<CAENVME_DecodeError(ecodes[i])
       <<"\t"<<std::endl;
    }
    throw std::runtime_error(e.str());
  }
}

void V172X_Daq::ReadVMERegisters(uint32_t address, 
				 uint32_t *data) throw(std::runtime_error)
{
  const int n = _params.enabled_boards;
  uint32_t* luint_data = data;
  uint32_t full_address[n];
  CVErrorCodes ecodes[n];
  uint32_t handles[V172X_Params::nboards];
  int board = 0;
  for(int i=0; i< _params.nboards; i++){
    if(_params.board[i].enabled){
      full_address[board] = _params.board[i].address+address;
      handles[board]=_handle_board[i];
      board++;
    }
  }
  CVErrorCodes err;
  if(!stupid){
    //This doesn't seem to work...
    err = CAENVME_MultiRead(_handle_vme_bridge, full_address, luint_data, n, 
			     add_mod, data_width, ecodes);
  }
  else{
    err = cvSuccess;
    for(int i=0; i<n; i++) {
      ecodes[i] = CAENVME_ReadCycle(handles[i],full_address[i],luint_data + i,
			       add_mod[i],data_width[i]);
      if (ecodes[i] != cvSuccess) err = ecodes[i];
    }
  }
  
   if(err != cvSuccess){
    switch(err){
    case cvBusError:
      _status = BUS_ERROR;
      break;
    case cvCommError:
      _status = COMM_ERROR;
      break;
    default:
      _status = GENERIC_ERROR;
    }
    Message e(EXCEPTION);
    e <<" Errors reading from VME Registers:\n";
    for(int i=0; i<n; i++){
      e<<"\t"<<std::hex<<std::showbase<<full_address[i]<<": "
       <<"\t"<<std::dec<<std::noshowbase<<CAENVME_DecodeError(ecodes[i])
       <<"\t"<<std::endl;
    }
    throw std::runtime_error(e.str());
  }
}
