#include "RawWriter.hh"
#include "Message.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include "EventHandler.hh"
#include "runinfo.hh"
#include <time.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <sys/stat.h> //needed for mkdir
#include <zlib.h>

RawWriter::RawWriter() : 
  BaseModule(RawWriter::GetDefaultName(),
	     "Saves the (gzip'ped) raw data from the digitizers to disk"), 
  _fout(), _logout(), _log_messenger(0), _ok(true), _bytes_written(0)
{
  RegisterParameter("filename",_filename = "",
		    "Name of the output file; if it doesn't contain a /, assumed relative to <directory>");
  RegisterParameter("directory",_directory=".", 
		    "Directory in which to place the output file");
  RegisterParameter("create_directory", _create_directory = false,
		    "If true, create a new directory under the <directory> path with the base filename");
  RegisterParameter("filenamebase", _autonamebase = "rawdaq" ,
		    "Base for automatic filenames <base>_yymmddHHMM.###.out");
  RegisterParameter("compression", _compression = Z_BEST_SPEED,
		    "zip compression level of the event structures");
  RegisterParameter("save_config", _save_config = true,
		    "Do we save the configuration along with the data?");
  RegisterParameter("write_database", _write_database = false, 
		    "Save a copy of the runinfo to a database?");
  
  RegisterParameter("max_file_size", _max_file_size = 0x80000000, //2 GiB
		    "Maximum file size before making a new file");
  RegisterParameter("max_event_in_file", _max_event_in_file = 10000 , 
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

RawWriter::~RawWriter()
{
  if(_fout.is_open())
    CloseCurrentFile();
}

std::string RawWriter::GetDefaultFilename() const
{
  //create the default filename
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo;
  timeinfo = localtime(&rawtime);
  char fbuf[100];
  strftime(fbuf,100,"%y%m%d%H%M",timeinfo);
  return _autonamebase+"_"+fbuf;
    
}

int RawWriter::Initialize()
{
  //query user for run metadata
  runinfo* info = EventHandler::GetInstance()->GetRunInfo();
  if(info){
    int ret = info->FillDataForRun(runinfo::RUNSTART);
    if(ret){
      Message(INFO)<<"User cancelled or error filling run metadata; aborting\n";
      return ret;
    }
  }
  if( _filename == ""){
    //was not set manually, so use auto
    _filename = GetDefaultFilename();
  }
  
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
  
  //make a directory before starting the file if requested
  if(_create_directory){
    std::string dirpart="", filepart=_filename;
    size_t slash = _filename.find_last_of('/');
    if(slash != std::string::npos){
      dirpart = _filename.substr(0,slash+1);
      filepart = _filename.substr(slash+1);
    }
    //filepart _should_ have suffixes removed...
    dirpart += filepart;
    Message(INFO)<<"Attempting to create directory "<<dirpart<<std::endl;
    int err = mkdir(dirpart.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    if(err){
	Message(ERROR)<<"Unable to create output directory "<<dirpart<<"\n";
	return err;
    }
    _filename = dirpart + "/" + filepart;
  }
  
  //set the run id
  _ghead.run_id = EventHandler::GetInstance()->GetRunID();
  //set the file index
  _ghead.file_index = 0;
  //open up the log file
  std::string logfilename = _filename+".log";
  _logout.open(logfilename.c_str());
  if(_logout.is_open()){
    _log_messenger = MessageHandler::GetInstance()->
      AddMessenger(DEBUG, MessageHandler::PrintToStream(_logout,false) );
    Message(DEBUG)<<"Started logging messages to "<<logfilename<<".\n";
  }
  else{
    Message(WARNING)<<"Unable to open logfile "<<logfilename
		    <<"; messages will not be logged!\n";
  } 
  //write the partial config file
  if(_save_config)
    SaveConfigFile();
  return OpenNewFile();
}

int RawWriter::Process(EventPtr event)
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
  if(_ghead.nevents>=(uint32_t)_max_event_in_file || 
     _ghead.file_size + ehead->event_size > _max_file_size){
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
  _ghead.nevents++;
  if(_ghead.event_id_min > ehead->event_id)
    _ghead.event_id_min = ehead->event_id;
  _ghead.event_id_max = ehead->event_id;
  _ghead.file_size += ehead->event_size;
  
  return 0;
}

int RawWriter::Finalize()
{
  int status = 0;

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
  

  if(_bytes_written > 0){
    runinfo* info = EventHandler::GetInstance()->GetRunInfo();
    if(info){
      if(_save_config || _write_database){
	int ret = info->FillDataForRun(runinfo::RUNEND);
	if(ret){
	  Message(WARNING)<<"User cancel or error filling end run metadata!\n";
	  status = ret;
	  //return ret;
	}
      }
      if(_write_database){
	VDatabaseInterface* db = EventHandler::GetInstance()->
	  GetDatabaseInterface();
	if(db){
	  status = db->StoreRuninfo(info);
	}
	else{
	  Message(ERROR)<<"RawWriter: write_dabase enabled, but no database "
			<<" configured!\n";
	}
      }
    }
    if(_save_config){
      SaveConfigFile();
    }
  }
  if(_logout.is_open()){
    Message(DEBUG)<<"Closing logfile.\n";
    MessageHandler::GetInstance()->RemoveMessenger(_log_messenger);
    _log_messenger = 0;
    _logout.close();
  }
  return status;
}


int RawWriter::OpenNewFile()
{
  if(_fout.is_open()){
    Message(WARNING)<<"Tried to open new file while current file still open!\n";
    CloseCurrentFile();
  }
  
  //set the filename to filename.###.out, where ### is file_index
  std::stringstream fname;
  fname<<_filename<<"."<<std::setw(3)<<std::setfill('0')
       <<_ghead.file_index<<".out";
  Message(INFO)<<"Opening file "<<fname.str()<<std::endl;
  _fout.open(fname.str().c_str(), std::ios::out|std::ios::binary);
  if(!_fout.is_open()){
    Message(ERROR)<<"Unable to open file "<<fname.str()<<" for output!\n";
    _ok = false;
    return 1;
  }
  //write the "blank" global header 
  _ghead.start_time = time(0);
  //reset nevents
  _ghead.nevents = 0;
  //set min event id to max value so we can set it properly during Process
  _ghead.event_id_min = 0xFFFFFFFF;
  //same for max id
  _ghead.event_id_max = 0;
  //reset the file_size 
  _ghead.file_size = _ghead.global_header_size;
  
  if(!_fout.write((const char*)(&_ghead), _ghead.global_header_size)){
    Message(ERROR)<<"RawWriter: Error writing header to file "<<fname<<"\n";
    return 2;
  }
  
  return 0;
}

int RawWriter::CloseCurrentFile()
{
  if(!_fout.is_open())
    return 0;
  //save the completed global header
  _ghead.end_time = time(0);
  _fout.seekp(0);
  _fout.write((const char*)(&_ghead), _ghead.global_header_size);
  _fout.close();
  //increment the file_index counter
  _ghead.file_index++;
  return 0;
}

void RawWriter::SaveConfigFile()
{
  //strip the '.gz' off the end of the file
  std::string cfgfile(_filename);
  if(_filename.rfind(".gz") != std::string::npos)
    cfgfile.resize(cfgfile.size()-3);
  //strip off .out
  if(_filename.rfind(".out") != std::string::npos)
    cfgfile.resize(cfgfile.size()-4);
  ConfigHandler::GetInstance()->SaveToFile((cfgfile+".cfg").c_str());
  
}
