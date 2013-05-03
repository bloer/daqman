/** @file AsciiWriter.hh
    @brief Defines AsciiWriter module
    @author bloer
    @ingroup modules
*/

#ifndef ASCII_WRITER_h
#define ASCII_WRITER_h

#include "ChannelModule.hh"
#include <fstream>

/** @class AsciiWriter 
    @brief module which dumps the waveform of a channel to a text file
    @ingroup modules
*/
class AsciiWriter : public ChannelModule{
public:
  AsciiWriter();
  ~AsciiWriter();
  
  int Initialize();
  int Finalize();
  int Process(ChannelData* chdata);
  
  static const std::string GetDefaultName(){ return "AsciiWriter"; }
  
  /// Get the default name of the output text file
  std::string GetDefaultFilename(){ return "out.txt"; }
  /// Get the name of the output text file being used
  const std::string& GetFilename(){ return _filename; }
  /// Set the name of the output text file
  void SetFilename(const std::string& s){ _filename = s; }
private:
  std::ofstream _fout;     ///< ofstream to write to
  std::string _filename;   ///< name of the output file
  bool _wrote_header;      ///< has the header been written yet?
};

#endif
