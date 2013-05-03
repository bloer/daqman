#include "MCRawWriter.hh"
#include "Message.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include <time.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <zlib.h>

MCRawWriter::MCRawWriter() : 
  BaseModule(MCRawWriter::GetDefaultName(),
	     "Saves the (gzip'ped) raw data from the digitizers to disk"), 
  _fout(), _ok(true), _bytes_written(0)
{
  _filename = GetDefaultFilename();
  RegisterParameter("filename",_filename,
		    "Name of the output file; if it doesn't contain a /, assumed relative to <directory>");
  RegisterParameter("directory",_directory=".", 
		    "Directory in which to place the output file");
  RegisterParameter("compression", _compression = Z_BEST_SPEED,
		    "zip compression level of the event structures");
  RegisterParameter("save_config", _save_config = true,
		    "Do we save the configuration along with the data?");
  RegisterParameter("max_file_size", _max_file_size = 0x80000000, //2 GiB
		    "Maximum file size before making a new file");
  RegisterParameter("max_event_in_file", _max_event_in_file = 1000, 
		    "Maximum number of events before making a new file");

  ConfigHandler* config = ConfigHandler::GetInstance();
  config->AddCommandSwitch('f',"filename", "File for saving raw data",
			   CommandSwitch::DefaultRead<std::string>(_filename),
			   "file");
  config->AddCommandSwitch('d',"directory","Output directory for raw file",
			   CommandSwitch::DefaultRead<std::string>(_directory),
			   "directory");
  config->AddCommandSwitch('c',"compression","Raw data compression level",
			   CommandSwitch::DefaultRead<int>(_compression),
			   "level");
}

MCRawWriter::~MCRawWriter()
{
  if(_fout.is_open())
    CloseCurrentFile();
}

std::string MCRawWriter::GetDefaultFilename() const
{
  //create the default filename
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo;
  timeinfo = localtime(&rawtime);
  char fbuf[40];
  strftime(fbuf,40,"rawdaq_%y%m%d_%H%M.out",timeinfo);
  return fbuf;
  
}

int MCRawWriter::Initialize()
{
  //see if we need to prepend the directory
  if( _filename.find("/") ==std::string::npos && _directory != ""){
    std::string temp = _directory;
    if(temp[temp.size()-1] != '/')
      temp.append("/");
    temp.append(_filename);
    _filename=temp;
  }
  //see if we need to remove the .out suffix
  if( _filename.rfind(".out") != std::string::npos)
    _filename.resize(_filename.size()-4);
  //set the run id
  // _ghead.run_id = EventHandler::GetInstance()->GetRunID();
  _mchead.run_id = 0;  // set via SetRunID()
  //set the file index
  _mchead.file_index = 0;
  //write the partial config file
  // if(_save_config)
  //   SaveConfigFile();
  return OpenNewFile();
}

int MCRawWriter::Process(MCEventPtr event)
{
  if(!_ok){
    Message(ERROR)<<"Attempt to write to file in bad state!\n";
    return 1;
  }
  
  typedef Reader::datablock_header datablock_header;
  //compress all of the datablocks into a separate buffer
  //each block has compressed size, including header, uncompressed data size, 
  //and type as header
  //determine the total size of the output buffer
  uint32_t bufsize = sizeof(Reader::event_header);
  for(size_t i = 0; i<event->GetRawEvent()->GetNumDataBlocks(); i++){
    bufsize += compressBound(event->GetRawEvent()->GetDataBlockSize(i)) + 
      sizeof(datablock_header);
  }
  //zip the data into the buffer
  std::vector<char> buf(bufsize);
  size_t zipsize=sizeof(Reader::event_header);
  for(size_t i = 0;i<event->GetRawEvent()->GetNumDataBlocks(); i++){
    //write the data into a space after the header
    uLong thistransfer = bufsize-zipsize-sizeof(datablock_header);
    int err = 0;
    if(_compression == Z_DEFAULT_COMPRESSION){
      err = compress((Bytef*)(&buf[zipsize+sizeof(datablock_header)]),
		     &thistransfer,
		     event->GetRawEvent()->GetRawDataBlock(i),
		     event->GetRawEvent()->GetDataBlockSize(i));
    }
    else{
      err = compress2((Bytef*)(&buf[zipsize+sizeof(datablock_header)]),
		      &thistransfer,
		      event->GetRawEvent()->GetRawDataBlock(i),
		      event->GetRawEvent()->GetDataBlockSize(i),
		      _compression);
    }
    if(err != Z_OK){
      Message(ERROR)<<"Unable to compress event datablocks in memory\n";
      return -1;
    }
    //write the header
    datablock_header* db_head = (datablock_header*)(&buf[zipsize]);
    db_head->total_blocksize_disk = sizeof(datablock_header)+thistransfer;
    db_head->datasize = event->GetRawEvent()->GetDataBlockSize(i);
    db_head->type = event->GetRawEvent()->GetDataBlockType(i);
    zipsize += db_head->total_blocksize_disk;
    
  }
  //set values in the event header
  Reader::event_header* ehead = (Reader::event_header*)(&buf[0]);
  ehead->event_size = zipsize;
  ehead->event_id = event->GetRawEvent()->GetID();
  ehead->timestamp = event->GetRawEvent()->GetTimestamp();
  ehead->nblocks = event->GetRawEvent()->GetNumDataBlocks();

  //see if we need to make a new file
  if(_mchead.nevents>=(uint32_t)_max_event_in_file || 
     _mchead.file_size + ehead->event_size > _max_file_size){
    //  if(((ehead->event_id+1) % _max_event_in_file == 0) || _mchead.file_size + ehead->event_size > _max_file_size){
    if( CloseCurrentFile() || OpenNewFile() ){
      Message(ERROR)<<"Error occurred when trying to open new file!\n";
      return 1;
    }
  }
  
  //actually write the event
  if(!_fout.write((const char*)(&buf[0]), zipsize)){
    Message(ERROR)<<"Error occurred when writing event "<<ehead->event_id
		  <<"to disk!\n";
    return -1;
  }
  _bytes_written += ehead->event_size;
  //update info for global header
  _mchead.nevents++;
  if(_mchead.event_id_min > ehead->event_id)
    _mchead.event_id_min = ehead->event_id;
  _mchead.event_id_max = ehead->event_id;
  _mchead.file_size += ehead->event_size;
  return 0;
}

int MCRawWriter::Finalize()
{
  if(_fout.is_open()){
    CloseCurrentFile();
    Message(INFO)<<_bytes_written/1024/1024<<" MiB saved to "<<_filename<<"\n";
    if(_bytes_written==0){
      //Message(WARNING)<<"0 bytes saved; deleting file."<<std::endl;
      //char command[40];
      //sprintf(command,"rm -f %s",_filename.c_str());
      //int result = system(command);
      //if(result)
      //Message(ERROR)<<"Unable to delete file!\n";
    }
  }
  if(_save_config && _bytes_written > 0){
    SaveConfigFile();
  }
  return 0;
}


int MCRawWriter::OpenNewFile()
{
  if(_fout.is_open()){
    Message(WARNING)<<"Tried to open new file while current file still open!\n";
    CloseCurrentFile();
  }
  
  //set the filename to filename.###.out, where ### is file_index
  std::stringstream fname;
  fname<<_filename<<"."<<std::setw(3)<<std::setfill('0')
       <<_mchead.file_index<<".out";
  Message(INFO)<<"Opening file "<<fname.str()<<std::endl;
  _fout.open(fname.str().c_str(), std::ios::out|std::ios::binary);
  if(!_fout.is_open()){
    Message(ERROR)<<"Unable to open file "<<fname.str()<<" for output!\n";
    _ok = false;
    return 1;
  }
  //write the "blank" global header 
  _mchead.start_time = time(0);
  //reset nevents
  _mchead.nevents = 0;
  //set min event id to max value so we can set it properly during Process
  _mchead.event_id_min = 0xFFFFFFFF;
  //reset the file_size 
  _mchead.file_size = _mchead.global_header_size;
  
  _fout.write((const char*)(&_mchead), _mchead.global_header_size);
  
  return 0;
}

int MCRawWriter::CloseCurrentFile()
{
  if(!_fout.is_open())
    return 0;
  //save the completed global header
  _mchead.end_time = time(0);
  _fout.seekp(0);
  _fout.write((const char*)(&_mchead), _mchead.global_header_size);
  _fout.close();
  //increment the file_index counter
  _mchead.file_index++;
  return 0;
}

void MCRawWriter::SaveConfigFile()
{
  //strip the '.gz' off the end of the file
  std::string cfgfile(_filename);
  if(_filename.rfind(".gz") != std::string::npos)
    cfgfile.resize(cfgfile.size()-3);
  //strip off .out
  if(_filename.rfind(".out") != std::string::npos)
    cfgfile.resize(cfgfile.size()-4);
  _mcinfo.SaveToFile((cfgfile+".cfg").c_str());
  
}
