#include "Reader.hh"
#include "Message.hh"
#include "ConfigHandler.hh"
#include "EventHandler.hh"
#include <fstream>
#include <stdexcept>
#include <iomanip>

Reader::Reader(const std::string& filename) : 
  _filename(filename), _fin(0),  _ok(true),
  _current_index(-1), _current_event(), _current_file_index(_unset_file_index),
  _current_file_name(""),
  _end_last_file(false)
{
  
  if(!OpenNextFile()){
    //look for a saved config file
    if(ConfigHandler::GetInstance()->GetSavedCfgFile()==""){
      std::string cfgfile = _current_file_name+".";
      size_t pos;
      bool cfgfound = false;
      while( (pos = cfgfile.rfind('.')) != std::string::npos){
	cfgfile.resize(pos);
	std::string testfile = cfgfile+".cfg";
	Message(DEBUG2)<<"Testing for config file with name "<<testfile<<"\n";
	std::ifstream cfgfin(testfile.c_str());
	if(cfgfin.is_open()){
	  ConfigHandler::GetInstance()->SetSavedCfgFile(testfile);
	  Message(DEBUG)<<"Found saved cfg file at "<<testfile<<"\n";
	  cfgfound = true;
	  break;
	}
	cfgfin.close();
      }
      if(!cfgfound)
	Message(WARNING)<<"Unable to locate configuration file for raw file "
			<<filename<<"\n";
    }
    //try to set the run id for processing modules
    if(_ghead.global_header_version>0)
      EventHandler::GetInstance()->SetRunID(_ghead.run_id);
    else
      EventHandler::GetInstance()->SetRunIDFromFilename(filename);
  }

}

Reader::~Reader()
{
  if(_fin) CloseCurrentFile();
}

bool Reader::ErrorCheck(int bytes_read, int bytes_requested)
{
  //returns true if error encountered
  if(bytes_read == -1){
    Message(ERROR)<<"Error encountered when reading from file.\n";
    _ok = false;
    return true;
  }
  else if(bytes_read == 0){
    Message(DEBUG)<<"End of file reached.\n";
    return true;
  }
  else if(bytes_read < bytes_requested){
    Message(ERROR)<<"Problem reading header for next event.\n";
    _ok = false;
    return true;
  }
  return false;
}


int Reader::ReadNextHeader()
{
  if(!_ok){
    Message(ERROR)<<"Attempt to read from file in bad state.\n";
    return 1;
  }
  //see if we need to open the next file
  if(eof()){
    Message(DEBUG)<<"Reached end of files to search.\n";
    return 1;
  }
  int bytes_read = 0;
  switch(_ghead.event_header_version){
  case 0:
    //use the legacy header
    event_header_v0 head;
    bytes_read = gzread(_fin, &head, sizeof(event_header_v0));
    if(ErrorCheck(bytes_read, sizeof(event_header_v0)))
      return 1;
    // copy legacy header info into the latest header version
    _ehead.event_size = head.event_size;
    _ehead.event_id = head.event_id;
    _ehead.timestamp = head.timestamp;
    _ehead.nblocks = 1;
    break;
  case latest_event_version:
    //use the current header
    bytes_read = gzread(_fin, &_ehead, sizeof(_ehead));
    if(ErrorCheck(bytes_read, sizeof(_ehead))){
      if(bytes_read==0) //could just be EOF, not actual error; try again
	return ReadNextHeader();
      //otherwise its a problem
      return 1;
    }
    break;
  default:
    Message(CRITICAL)<<"Unknown event header version number!\n";
  }
  return 0;
}

z_off_t Reader::SkipNextEvent(bool skip_header)
{
  //see if we need to open the next file
  if(gzeof(_fin) && OpenNextFile()){
    Message(DEBUG)<<"Reached end of files to search.\n";
    return 0;
  }
  
  z_off_t seekpos=0;
  if(skip_header){
    //first byte is event size
    uint32_t esize = 0;
    int bytes_read = gzread(_fin, &esize, sizeof(uint32_t));
    if(ErrorCheck(bytes_read, sizeof(uint32_t)))
      return 0;
    z_off_t seek_length = esize - sizeof(uint32_t);
    seekpos =  gzseek(_fin, seek_length, SEEK_CUR);
  }
  else{
    z_off_t seek_length = _ehead.event_size - sizeof(event_header);
    if(_ghead.event_header_version == 0)
      seek_length = _ehead.event_size - sizeof(event_header_v0);
    seekpos =  gzseek(_fin, seek_length, SEEK_CUR);
  }
  _current_index++;
  return seekpos;
    
}

RawEventPtr Reader::GetNextEvent(bool read_header)
{
  if(read_header) 
    ReadNextHeader();
  if(!_ok){
    Message(ERROR)<<"Attempt to read from file in bad state.\n";
    return RawEventPtr();
  }
  if(eof()){
    return RawEventPtr();
  }
  //read depends on file version
  RawEventPtr next(new RawEvent);
  
  switch(_ghead.event_header_version){
  case 0:{
    //event data in this generation of file is not internally zipped
    //consists only of V172X blocks after the legacy header
    uint32_t evsize = _ehead.event_size - sizeof(event_header_v0);
    int blockn = next->AddDataBlock(RawEvent::CAEN_V172X, evsize);
    int bytes_read = gzread(_fin, next->GetRawDataBlock(blockn), evsize);
    if(ErrorCheck(bytes_read, evsize))
      return RawEventPtr();
    break;
  }
  case latest_event_version: {
    //this event structure has individually zipped data blocks
    datablock_header bh;
    uint32_t thisblock = 0;
    int bytes_read = 0;
    while(thisblock < _ehead.nblocks && 
	  (uint32_t)bytes_read<_ehead.event_size-sizeof(event_header)){
      int head_read = gzread(_fin, &bh, sizeof(bh));
      if(ErrorCheck(head_read, sizeof(bh)))
	return RawEventPtr();
      //create datablock in the raw event
      int blockn = next->AddDataBlock(bh.type, bh.datasize);
      //read the compressed block into a temporary buffer
      std::vector<char> buf(bh.total_blocksize_disk);
      int block_read = gzread(_fin, &(buf[0]), 
			      bh.total_blocksize_disk-sizeof(bh));
      if(ErrorCheck(block_read, bh.total_blocksize_disk-sizeof(bh))){
	Message(ERROR)<<"Incorrect blocksize for block "<<thisblock<<" in event "
		      <<_ehead.event_id;
	return RawEventPtr();
      }
      //unzip the buffer into the RawEvent
      uLongf decomp = bh.datasize;
      int err = uncompress(next->GetRawDataBlock(blockn), &decomp,
			   (Bytef*)(&(buf[0])), block_read);
      if(err != Z_OK){
	Message(ERROR)<<"uncompress function returned "<<err
		      <<" while reading event!\n";
	return RawEventPtr();
      }
      next->SetDataBlockSize(blockn,decomp);
      //done with this block
      thisblock++;
      bytes_read += head_read + block_read;
    }
    //make sure everything got read
    if((uint32_t)bytes_read != _ehead.event_size-sizeof(event_header) || 
       thisblock != _ehead.nblocks){
      Message(ERROR)<<"The event with id "<<_ehead.event_id
		    <<" was not fully read out!\n";
      return RawEventPtr();
    }
    break;
  }
  default:
    Message(CRITICAL)<<"Unknown event header version number!\n";
    return RawEventPtr();
  }//end switch on event head version
  
  //if we get here everything seems ok
  next->SetID(_ehead.event_id);
  next->SetTimestamp(_ehead.timestamp);
  if(_ghead.global_header_version > 0)
    next->SetRunID(_ghead.run_id);
  
  _current_index++;
  _current_event = next;
  return next;
}

RawEventPtr Reader::GetLastEvent()
{
  if(!_ok){
    Message(ERROR)<<"Attempt to read from file in bad state.\n";
    return RawEventPtr();
  }
  int last_id=0;
  while(!ReadNextHeader()){
    last_id = _ehead.event_id;
    SkipNextEvent(false);
  }
  if(!_ok) 
    return RawEventPtr();
  Reset();
  return GetEventWithID(last_id);
}
 

RawEventPtr Reader::GetEventWithIndex(int index)
{
  if(!_ok){
    Message(ERROR)<<"Attempt to read from file in bad state.\n";
    return RawEventPtr();
  }
  if(index == _current_index) 
    return _current_event;
  //we can't read backward one event at a time, so if requested index
  //is lower than current, we have to rewind the whole file
  if(index < _current_index){
    Message(DEBUG)<<"Rewinding raw file...\n";
    Reset();
  }
  //Now we can go looking for the right one
  while(_ok && _current_index < index-1){
    if(SkipNextEvent())
      return RawEventPtr();
  }
  //if here, either we are ready to read the next event, or there was an error
  if(!_ok || _end_last_file)
    return RawEventPtr();
  else if(_current_index == index-1)
    return GetNextEvent();
  else{
    Message(ERROR)<<"Unknown problem occurred trying to seek to index "
		  <<index<<"\n";
    return RawEventPtr();
  }
}

RawEventPtr Reader::GetEventWithID(uint32_t id)
{
  if(!_ok){
    Message(ERROR)<<"Attempt to read from file in bad state.\n";
    return RawEventPtr();
  }
  if(id == 0){
    Reset();
    return GetNextEvent();
  }
  if(id == _ehead.event_id)
    return _current_event;
  //we can't read backward one event at a time, so if requested id
  //is lower than current, we have to rewind the whole file
  if(id != 0 && id < _ehead.event_id ){
    Message(DEBUG)<<"Rewinding raw file...\n;";
    if(Reset())
      return RawEventPtr();
  }
  //see if we're in the right file
  while( _ghead.global_header_version > 0 && _ghead.event_id_max > 0 &&
	 _ghead.nevents>0 && id > _ghead.event_id_max ){
    if(OpenNextFile()){
      Message(ERROR)<<"Event with id "<<id
		    <<" is not present in this file set.\n";
      return RawEventPtr();
    }
  }
  
  while(_ok && !_end_last_file ){
    //read in the current header so we know how long to seek
    if(ReadNextHeader())
      return RawEventPtr();
    if(_ehead.event_id == id){
      return GetNextEvent(false);
    }
    else if(_ehead.event_id > id){
      Message(ERROR)<<"Event with id "<<id<<" is not present in this file.\n";
      return RawEventPtr();
    }
    //still deeper in the file; skip to next
    SkipNextEvent(false);
  }
  //if here, either we are ready to read the next event, or there was an error
  if(!_ok || _end_last_file)
    return RawEventPtr();
  //we shouldn't really get here...
  return _current_event;
}

bool Reader::GetAssociatedParameter(VParameterNode* par, std::string key)
{	
  if(key == "") 
    key = par->GetDefaultKey();
  bool cfg_found = true;
  if(!(par->ReadFromFile((_filename+".cfg").c_str(),key,true))){
    //ok, that didn't work, try removing a possible .gz suffix
    std::string fnamecopy(_filename);
    fnamecopy.resize(fnamecopy.size()-3);
    if(!(par->ReadFromFile((fnamecopy+".cfg").c_str(),
			   key, true))){
      //that still didn't work.  Print a message and give up
      Message(INFO)<<"Unable to find associated config file.\n";
      cfg_found = false;
    }
  }
  return cfg_found;
}

int Reader::Reset()
{
  if(!_ok)
    return 0;
  if(_fin)
    CloseCurrentFile();
  _current_file_index = _unset_file_index;
  _current_index = -1;
  _current_event = RawEventPtr();
  _ehead.reset();
  _end_last_file = false;
  return OpenNextFile();
}

int Reader::CloseCurrentFile()
{
  if(_fin)
    gzclose(_fin);
  _fin = 0;
  return 0;
}

int Reader::OpenNextFile()
{
  using std::string;
  if(_fin)
    CloseCurrentFile();
  if(_current_file_index == _unset_file_index && 
     ( _filename.substr(_filename.size()-7) == ".out.gz" ||
       _filename.substr(_filename.size()-4) == ".out"       ) ){
    //we haven't tried to open anything yet, so this may be the first file
    //we don't know if this is the old or new file system, try the old first
    Message(DEBUG)<<"Attempting to open file "<<_filename.c_str()
		  <<" as legacy format."<<std::endl;
    _current_file_name = _filename;
    _fin = gzopen(_current_file_name.c_str(),"rb");
  }
  if(!_fin && _current_file_name != _filename){
    //we have already opened a previous split file
    std::stringstream fname;
    fname<<_current_file_name.substr(0,_current_file_name.size()-7)
	 <<std::setw(3)<<std::setfill('0')<<_current_file_index+1 <<".out";
    _current_file_name = fname.str();
    Message(DEBUG)<<"Next file in series should be "<<_current_file_name
		  <<"\n";
    _fin = gzopen(_current_file_name.c_str(),"rb");
  }
  if(!_fin){
    Message(DEBUG)<<"No file with name "<<_filename<<" exists; "
    		  <<"trying alterations for new format.\n";
    //try opening the first split
    //format is FILE.###.out, ### is file index
    string filename = _filename;
    size_t last_slash = filename.rfind('/');
    if(last_slash == string::npos) last_slash = 0;
    string dirpart="", filepart=filename;
    if(last_slash == filename.size()-1){
      dirpart = filename;
      filepart = "";
    }
    else if(last_slash != string::npos){
      dirpart = filename.substr(0, last_slash+1);
      filepart = filename.substr(last_slash+1, string::npos);
    }
    
    //first, try removing suffixes one at a time from the filename part
    // and appending .###.out
    while(!filepart.empty()){
      std::stringstream fname;
      fname<<dirpart<<filepart<<"."
	   <<std::setw(3)<<std::setfill('0')<<_current_file_index+1<<".out";
      _current_file_name = fname.str();
      Message(DEBUG2)<<"Testing filename "<<_current_file_name<<" ...\n";
      _fin = gzopen(_current_file_name.c_str(),"rb");
      if(_fin)
	break;
      size_t last_dot = filepart.rfind('.');
      if(last_dot == string::npos)
	break;
      filepart.resize(last_dot);
    }

    if(!_fin){
      //we may be pointing to a directory, which could be either the filepart
      // or last bit of dirpart
      if(!filepart.empty()){
	dirpart += filepart + "/";
	std::stringstream fname;
	fname<<dirpart<<filepart<<"."
	     <<std::setw(3)<<std::setfill('0')<<_current_file_index+1<<".out";
	_current_file_name = fname.str();
	Message(DEBUG2)<<"Testing filename "<<_current_file_name<<" ...\n";
	_fin = gzopen(_current_file_name.c_str(),"rb");
      }
      else{
	//if we get here, the filename command ended in /, so try to extract it
	if(!dirpart.empty() && dirpart[dirpart.size()-1]=='/'){
	  last_slash = dirpart.rfind('/',dirpart.size()-2);
	  if(last_slash != string::npos){
	    filepart = dirpart.substr(last_slash+1,string::npos);
	    filepart.resize(filepart.size()-1);
	    std::stringstream fname;
	    fname<<dirpart<<filepart<<"."
		 <<std::setw(3)<<std::setfill('0')<<_current_file_index+1
		 <<".out";
	    _current_file_name = fname.str();
	    Message(DEBUG2)<<"Testing filename "<<_current_file_name<<" ...\n";
	    _fin = gzopen(_current_file_name.c_str(),"rb");
	  }
	}
      }//end check on dirpart empty
    }//end trying a directory
    if(_fin){
      _current_file_index++;
      //_filename = dirpart + filepart;
    }
  }
  
  if(!_fin){
    if(_current_file_index == _unset_file_index){
      Message(ERROR)<<"Unable to open file "<<_filename<<" for reading!\n";
      _ok = false;
    }
    _end_last_file = true;
    return 1;
  }
  
  _ehead.reset();
  //read in the global file header
  //assume we're using the latest header, then check to make sure
  gzread(_fin, &_ghead, _ghead.global_header_size);
  //check the magic number in the first 4 bytes
  if(_ghead.magic_num_check != magic_number){
    //we are in a legacy (pre-header) file
    _ghead.global_header_version = 0;
    _ghead.event_header_version = 0;
    _ghead.run_id = 0;
    _ghead.nevents = 0;
    _ghead.event_id_min = 0;
    _ghead.event_id_max = 0;
    _current_file_index = 0;
    gzrewind(_fin);
  }
  else{ 
    if(_ghead.global_header_version != latest_global_version || 
       _ghead.event_header_version != latest_event_version){
      //handle future version number updates here
      Message(CRITICAL)<<"Header version number stored in this file is larger"
		       <<" than latest version!\n";
      throw std::out_of_range("incorrect header version number");
    }
    else if(_ghead.global_header_size != sizeof(global_header) || 
	    _ghead.event_header_size != sizeof(event_header) ){
      Message(CRITICAL)<<"Incorrect size of headers in raw file!\n";
      throw std::out_of_range("incorrect header size");
    }
    Message(DEBUG)<<"Successfully opened file "<<_current_file_name
		  <<"\nFile header information:"
		  <<"\n\tMagic number: 0x"<<std::hex<<_ghead.magic_num_check
		  <<std::dec
		  <<"\n\tHeader size: "<<_ghead.global_header_size
		  <<"\n\tHeader version: "<<_ghead.global_header_version
		  <<"\n\tEvent header size: "<<_ghead.event_header_size
		  <<"\n\tEvent header version: "<<_ghead.event_header_version
		  <<"\n\tStart time: "<<_ghead.start_time
		  <<"\n\tEnd time: "<<_ghead.end_time
		  <<"\n\tRun ID: "<<_ghead.run_id
		  <<"\n\tFileIndex: "<<_ghead.file_index
		  <<"\n\tEvents: "<<_ghead.nevents
		  <<"\n\tMin Event: "<<_ghead.event_id_min
		  <<"\n\tMax Event: "<<_ghead.event_id_max
		  <<std::endl;
    _current_file_index = _ghead.file_index;
  }
  return 0;
}
