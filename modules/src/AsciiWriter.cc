#include "AsciiWriter.hh"
#include "ConvertData.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "intarray.hh"
AsciiWriter::AsciiWriter() : 
  ChannelModule(GetDefaultName(),
		"Print out each sample for each channel's waveform to a text file") 
{
  AddDependency<ConvertData>();
  RegisterParameter("filename", _filename = GetDefaultFilename(),
		    "Name of the output text file");
  ConfigHandler::GetInstance()->
    AddCommandSwitch('f',"filename", "Set output ASCII file",
		     CommandSwitch::DefaultRead<std::string>(_filename), 
		     "file");
}

AsciiWriter::~AsciiWriter()
{
  
}

int AsciiWriter::Initialize()
{
  _wrote_header = false;
  if(_filename.find(".txt") == std::string::npos)
    _filename.append(".txt");
  Message(INFO)<<"Saving text waveforms to file "<<_filename<<std::endl;
  _fout.open(_filename.c_str());
  if(!_fout.is_open()){
    Message(CRITICAL)<<"Unable to open file "<<_filename<<std::endl;
    return 1;
  }
  return 0;
}

int AsciiWriter::Finalize(){
  if(_fout.is_open())
    _fout.close();
  return 0;
}

int AsciiWriter::Process(ChannelData* chdata)
{
  if(!_wrote_header){
    //assume only one channel, or all identical channels
    _fout<<chdata->nsamps<<"\t"<<chdata->sample_rate<<"\t"
	 <<chdata->trigger_index<<std::endl;
    _wrote_header = true;
  }
  double* wave = chdata->GetWaveform();
  for(int samp = 0; samp < chdata->nsamps; samp++){
      _fout<<wave[samp]<<"\t";
  }
  _fout<<std::endl;
  return 0;
}
