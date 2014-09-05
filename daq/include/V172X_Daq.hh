/** @file V172X_Daq.hh
    @brief Defines V172X_Daq class which interfaces with CAEN V172X digitizers
    @author bloer
    @ingroup daqman
*/
#ifndef V172X_Daq_h
#define V172X_Daq_h

#include "BaseDaq.hh"
#include "V172X_Params.hh"
#include "stdint.h"
#include "CAENVMElib.h"

//forward declaration
namespace std{
  class runtime_error;
}

/**
   @class V172X_Daq
   @brief Concrete implementation of BaseDaq for CAEN V172X digitizers
   @ingroup daqman
*/
class V172X_Daq : public BaseDaq
{
  //Inherited members from BaseDaq
public:
  /// Default constructor
  V172X_Daq();
  /// Destructor
  ~V172X_Daq();
  
  ///Get the parameters for this daq setup
  V172X_Params* GetParameters(){ return &_params; }
  
  /// Initialize the hardware communication
  int Initialize();
  /// Update the hardware parameters
  int Update();
  private:
  /// Acquire the data from the hardware and store in _events_queue
  void DataAcquisitionLoop();
    
  /// Initialize parameters for a single board
  int InitializeBoard(int boardnum);
  
  //---------defined in V172X_Daq_Helpers.cc-------
  /*The following helper functions all throw a uint32_t exception to denote 
    the address which generated it, and set the eStatus enum of the class 
    to the appropriate value.
  */
  
  ///Write to a register of a digitizer only
  void WriteDigitizerRegister(uint32_t address, uint32_t write_me, 
			      int32_t handle) throw(std::runtime_error);
  ///Read from a digitizer register
  uint32_t ReadDigitizerRegister(uint32_t address, int32_t handle) throw(std::runtime_error);
  
  /**
     Writes the data <write_me> to the VME address <address>
     @param address The address to write the data to
     @param write_me The data to write
     @param handle The CAENVMElib handle for the connection
  */
  void WriteVMERegister(uint32_t address, 
			uint32_t write_me, int32_t handle) throw(std::runtime_error);

  /// Read data from the VME register <address>
  uint32_t ReadVMERegister(uint32_t address, int32_t handle) throw(std::runtime_error);

  /** Writes data to the VME register <address> on EACH board
      @param address The least-significant half of the address for each board
      @param write_me The data to write to each boad's register
  */
  void WriteVMERegisters(uint32_t address, 
			 uint32_t write_me) throw(std::runtime_error);

  /**
     Write different data to the same address on each board
     @param address leas significant half of the address to write for each board
     @param write_me array of data to write, should be size of nboards
  */ 
  void WriteVMERegisters(uint32_t address, 
			 uint32_t* write_me) throw(std::runtime_error);

  /**
     Read the VME register <address> from each board, store the result in <data>
     @param address least significant address half to read from each board
     @param data array to store the results in
  */
  void ReadVMERegisters(uint32_t address, 
			uint32_t * data) throw(std::runtime_error);
  
  int32_t _handle_vme_bridge;///< caenvme opaque id for vme_bridge
  int32_t _handle_board[V172X_Params::nboards];  ///< caenvme opaque id for boards
  bool _initialized;         ///< Is hardware initialized?
  V172X_Params _params;      ///< parameters for the boards
  long _triggers;            ///< total triggers received so far
  boost::mutex _vme_mutex;   ///< mutex protecting write access to VME
  //std::vector<uint8_t*> raw_buffer;
  //std::vector<boost::mutex*> buffer_mutex;
  
};

#endif
