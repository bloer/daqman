#include "runinfo.hh"
#include "TGClient.h"
#include "TApplication.h"
#include "TGFrame.h"
#include "TGCanvas.h"
#include "TGTextEntry.h"
#include "TGDockableFrame.h"
#include "TGButton.h"
#include "TGButtonGroup.h"
#include "TQObject.h"
#include "TGToolTip.h"
#include "TGLabel.h"
#include "TGComboBox.h"
#include "TGMsgBox.h"
#include "TMacro.h"
#include "TList.h"
#include "TObjString.h"

typedef runinfo::stringmap stringmap;
typedef runinfo::stringvec stringvec;
typedef runinfo::DialogField DialogField;
typedef runinfo::FieldList FieldList;

//utility class to read now-obsolete comment parameter and put into metadata
class CommentReader{
  runinfo* _info;
public:
  CommentReader(runinfo* info) : _info(info) {}
  std::istream& operator()(std::istream& in)
  { phrase dummy; 
    if(in>>dummy) _info->SetMetadata("comment",dummy); 
    return in; 
  }
};

runinfo::runinfo(long id) : 
  ParameterList("runinfo","metadata about daq runs")
{
  Init();
  runid = id;
}

//reset variables that can be read from the raw data
void runinfo::ResetRunStats()
{
  starttime = 0;
  endtime = 0;
  triggers = 0;
  events = 0;
}

void runinfo::Init(bool reset)
{
  runid=-1;
  starttime=0;
  endtime=0;
  triggers=0;
  events=0;
  
  metadata.clear();
  prerun_dialog_fields.clear();
  postrun_dialog_fields.clear();
  force_prerun_dialog = false;
  force_postrun_dialog = false;
  channel_metadata.clear();
  
  if(!reset)
    InitializeParameterList();
}

void runinfo::InitializeParameterList()
{
  RegisterParameter("runid", runid, "unique ID number for the run");
  RegisterParameter("starttime",starttime,"Timestamp at start of run");
  RegisterParameter("endtime",endtime, "Timestamp at end of run");
  RegisterParameter("triggers",triggers,"Total triggers requested during run");
  RegisterParameter("events", events, "Number of events recorded during run");
  
  RegisterParameter("metadata", metadata, 
		    "User-defined per run info categories");
  RegisterParameter("prerun_dialog_fields", prerun_dialog_fields,
		    "List of fields to query user for at start of run");
  RegisterParameter("postrun_dialog_fields", postrun_dialog_fields,
		    "List of fields to query user for at end of run");
  RegisterParameter("force_prerun_dialog", force_prerun_dialog,
		    "Show prerun dialog even if all required fields valid");
  RegisterParameter("force_postrun_dialog", force_postrun_dialog,
		    "Show postrun dialog even if all required fields valid");
  
  RegisterParameter("channel_metadata", channel_metadata,
		    "map of channel ID to per-channel metadata");
  
  RegisterReadFunction("comment",CommentReader(this),
		       "Handle obsolete separate comment parameter (now in metadata)");
}


int runinfo::LoadSavedInfo(TMacro* mac)
{
  //first, read the macro into a stringstream
  TList* lines = mac->GetListOfLines();
  if(!lines || !lines->GetEntries())
    return 1;
  std::stringstream s;
  TIter next(lines);
  TObjString *obj;
  while ((obj = (TObjString*) next()))
    s<<obj->GetName()<<"\n";
  //now use the ParameterList methods to read from the stream
  try{
    ReadFromByKey(s, GetDefaultKey());
  }
  catch(std::exception& e){
    //there was an error reading
    return 2;
  }
  //if we get here, we should have been successful
  return 0;
}

//////// Everything below here is related to the metadata fill dialogs /////

bool AreAllFieldsValid(FieldList* fields, stringmap* metadata)
{
  //see if all fields are valid
  bool allvalid = true;
  FieldList::iterator field;
  for(field = fields->begin(); field != fields->end(); ++field){
    if( !field->IsValueValid( (*metadata)[field->fieldname] ) ){
      allvalid = false;
      break;
    }
  }
  return allvalid;
}

DialogField::DialogField(const std::string& field, 
			 const std::string& desc,
			 bool required_, 
			 const std::string& default_) : 
  ParameterList(field,"Specify a metadata field to query the user for"),
  fieldname(field), description(desc), required(required_), 
  defaultvalue(default_) 
{ 
  RegisterParameter("fieldname", fieldname);
  RegisterParameter("description", description);
  RegisterParameter("allowed_values", allowed_values);
  RegisterParameter("required",required);
  RegisterParameter("defaultvalue",defaultvalue);
}


bool DialogField::IsValueValid(const std::string& val) const
{
  if(!allowed_values.empty() && 
     std::find(allowed_values.begin(), allowed_values.end(), val) == 
     allowed_values.end() )
    return false;
  if(required)
    return val != "";
  return true;
}

class DialogFieldFrame : public TGVerticalFrame{
public:
  DialogFieldFrame(const DialogField* field, stringmap* metadata, 
		   const TGWindow* p=0);

  virtual ~DialogFieldFrame(){}
  
  TGTextEntry* GetTextEntry() const { return _textentry; }
  
  virtual Bool_t Notify(); //override this method to receive change signals
private:
  const DialogField* _field;
  stringmap* _metadata;
  TGTextEntry* _textentry;
  TGComboBox* _combobox;
};

DialogFieldFrame::DialogFieldFrame(const DialogField* field, stringmap* metadata,
				   const TGWindow* p) :
  TGVerticalFrame(p), _field(field), _metadata(metadata),
  _textentry(0), _combobox(0)
{
  std::string defaultvalue = field->defaultvalue;
  if((*metadata)[field->fieldname] != "")
    defaultvalue = (*metadata)[field->fieldname];

  TGHorizontalFrame* hframe = new TGHorizontalFrame(this);
  AddFrame(hframe, new TGLayoutHints(kLHintsExpandX|kLHintsTop,0,0,5));
  std::string label = "   ";
  if(field->required)
    label = "*  ";
  label += field->fieldname;
  hframe->AddFrame(new TGLabel(hframe, label.c_str()), new TGLayoutHints(kLHintsLeft));
  if(field->description != ""){
    std::string desc = "( ";
    desc += field->description + " )";
    hframe->AddFrame(new TGLabel(hframe, desc.c_str()), new TGLayoutHints(kLHintsRight));
  }
  
  if(field->allowed_values.empty()){
    _textentry = new TGTextEntry(this, defaultvalue.c_str());
    AddFrame(_textentry, new TGLayoutHints(kLHintsExpandX, 10));
    _textentry->Connect("TextChanged(const char*)","TGFrame",this,"Notify()");
  }
  else{
    _combobox = new TGComboBox(this);
    stringvec::const_iterator opt;
    int id=-1;
    for(opt = field->allowed_values.begin(); opt != field->allowed_values.end(); ++opt){
      _combobox->AddEntry(opt->c_str(), ++id);
      if(*opt == defaultvalue)
	_combobox->Select(id,false);
    }
    _combobox->SetHeight(20);
    AddFrame(_combobox, new TGLayoutHints(kLHintsExpandX,10));
    _combobox->Connect("Selected(const char*)","TGFrame",this,"Notify()");
  }
  
  Notify();
}

Bool_t DialogFieldFrame::Notify()
{
  std::string newval="";
  if(_textentry)
    newval = _textentry->GetText();
  else if(_combobox){
    TGTextLBEntry* e = (TGTextLBEntry*)(_combobox->GetSelectedEntry());
    if(e)
      newval = e->GetTitle();
  }
  (*_metadata)[_field->fieldname] = newval;
  int color = kYellow-9;
  if( _field->IsValueValid(newval) )
    color = kWhite;
  Pixel_t pixel = gVirtualX->GetPixel(color);
  if(_textentry)
    _textentry->SetBackgroundColor(pixel);
  else if(_combobox){
    _combobox->SetBackgroundColor(pixel);
    _combobox->SetEnabled(true);
  }

  return false;
}

class RunInfoFillHelper : public TGTransientFrame{
public:
  static int MetadataDialog(stringmap* metadata, FieldList* fields);
private:
  static int _returnval;
  RunInfoFillHelper(stringmap* metadata, FieldList* fields);
  virtual ~RunInfoFillHelper();
public:
  ///Steal the unused Activate function to handle button events
  virtual Bool_t ProcessMessage(Long_t b, Long_t, Long_t); 
private:
  stringmap* _metadata;
  FieldList* _fields;
  TGCanvas* can; // won't auto cleanup, so must do on our own
  TGCompositeFrame* mf2; //container of TGCanvas, need to delete manually
};

RunInfoFillHelper::~RunInfoFillHelper()
{
}

int RunInfoFillHelper::_returnval=1; //default value for window closed
int RunInfoFillHelper::MetadataDialog(stringmap* metadata, FieldList* fields)
{
  if(!gApplication)
    new TApplication("_app",0,0);
  gApplication->NeedGraphicsLibs();
  gApplication->InitializeGraphics();
  new RunInfoFillHelper(metadata, fields);
  return _returnval;
}

Bool_t RunInfoFillHelper::ProcessMessage(Long_t b, Long_t, Long_t)
{
  if(b==1){ //OK was pressed
    //see if all fields are valid
    bool allvalid = true;
    for(FieldList::iterator field = _fields->begin(); field != _fields->end(); ++field){
      if( !field->IsValueValid( (*_metadata)[field->fieldname] ) ){
	allvalid = false;
	break;
      }
    }
    if(!allvalid){
      new TGMsgBox(gClient->GetRoot(), this, "Invalid Fields","One or more required fields (marked yellow) do not have values assigned. Please enter valid values in these fields and submit again.");
      return false;
    }
    else //all fields successfully validated
      _returnval = 0;
  }
  else if(b==2){ // cancel was pressed
    _returnval = 2;
  }
  CloseWindow();
  return false;
}

//do all work in constructor
RunInfoFillHelper::RunInfoFillHelper(stringmap* metadata, FieldList* fields) : 
  TGTransientFrame(gClient->GetRoot()),
  _metadata(metadata), _fields(fields)
{
  SetWindowName("Please fill out run information");
  //build up frames to put text entry fields in
  TGCanvas* can = new TGCanvas(this,10, 10, kChildFrame);
  AddFrame(can, new TGLayoutHints(kLHintsExpandX|kLHintsExpandY,0,0,10));
  TGCompositeFrame* mf2 = new TGVerticalFrame(can->GetViewPort(),10,10);
  can->SetContainer(mf2);
  
  //Add OK and cancel buttons
  TGButtonGroup* bg = new TGButtonGroup(this,"",kChildFrame|kHorizontalFrame|kFixedSize);
  bg->Resize(300,70);
  AddFrame(bg,new TGLayoutHints(kLHintsBottom|kLHintsCenterX));
  TGTextButton* ok = new TGTextButton(bg,"OK");
  /*TGTextButton* cancel = */new TGTextButton(bg,"Cancel");
  bg->SetLayoutHints(new TGLayoutHints(kLHintsExpandX|kLHintsExpandY,10,10,3,3));
  bg->Connect("Clicked(Int_t)","TGCompositeFrame",this,
	      "ProcessMessage(Long_t,Long_t,Long_t)");
  
  TGTextEntry* first = 0, *last = 0;
  
  for(FieldList::const_iterator field = _fields->begin(); 
      field != _fields->end(); ++field){
    DialogFieldFrame* frame = new DialogFieldFrame(&(*field), metadata, mf2);
    mf2->AddFrame(frame, new TGLayoutHints(kLHintsExpandX,10,10,5,5));
    TGTextEntry* te = frame->GetTextEntry();
    if(te){
      //connect to the OK button 
      te->Connect("ReturnPressed()","TGButton",ok,"Clicked()");
      
      if(!first) first = te;
      //connect to previous with tab commands
      if(last){
	last->Connect("TabPressed()","TGTextEntry",te,"SetFocus()");
	last->Connect("TabPressed()","TGTextEntry",te,"SelectAll()");
	te->Connect("ShiftTabPressed()","TGTextEntry",last,"SetFocus()");
	te->Connect("ShiftTabPressed()","TGTextEntry",last,"SelectAll()");
      }
      last = te;
    }
  } //end loop over field list
  
  //connect the first and last to wrap tabbing
  if(first && last && first != last){
    first->SetFocus();
    first->Connect("ShiftTabPressed()","TGTextEntry",last,"SetFocus()");
    first->Connect("ShiftTabPressed()","TGTextEntry",last,"SelectAll()");
    last->Connect("TabPressed()","TGTextEntry",first,"SetFocus()");
    last->Connect("TabPressed()","TGTextEntry",first,"SelectAll()");
  }
    
  //make sure we clean up properly at the end
  SetCleanup(kDeepCleanup);
  can->GetViewPort()->SetCleanup(kDeepCleanup);
  //draw the window
  MapSubwindows();
  Resize(500,500);
  CenterOnParent();
  MapWindow();
  gClient->WaitFor(this);
}







////// Finally here's our outside-accessible function to query run data ///////

int runinfo::FillDataForRun(runinfo::FILLTIME when)
{
  FieldList* fields = (when == RUNSTART ? &prerun_dialog_fields : 
		                          &postrun_dialog_fields );
  bool forcedialog = (when == RUNSTART ? force_prerun_dialog : 
		                         force_postrun_dialog );
  if(fields && !fields->empty() && 
     (forcedialog || !AreAllFieldsValid(fields, &metadata)) )
    return RunInfoFillHelper::MetadataDialog(&metadata, fields);
  return 0;
}
