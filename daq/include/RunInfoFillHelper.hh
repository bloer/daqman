#ifndef RUNINFOFILLHELPER_h
#define RUNINFOFILLHELPER_h

#include "RQ_OBJECT.h"
#include "TGFrame.h"
#include <map>
#include <string>

class TGCanvas;
class TGCompositeFrame;
#include "ParameterList.hh"


/**@class DialogField
   @brief utility class to handle querying user for metadata
   Completely hide this from root!
 */

class DialogField : public ParameterList{

public:
  DialogField(const std::string& field_="", const std::string& desc = "",
              bool required_=true, const std::string& default_="");
  virtual ~DialogField() {}
  bool IsValueValid(const std::string& val) const;
  std::string fieldname;
  std::string description;
  std::vector<std::string> allowed_values;
  bool required;
  std::string defaultvalue;
};

typedef std::vector<DialogField> DialogFieldList;


class RunInfoFillHelper : public TGTransientFrame{
  RQ_OBJECT("RunInfoFillHelper")
public:
  typedef std::map<std::string, std::string> stringmap;
  static int MetadataDialog(stringmap* metadata, DialogFieldList* fields);
private:
  static int _returnval;
  RunInfoFillHelper(stringmap* metadata, DialogFieldList* fields);
  virtual ~RunInfoFillHelper();
public:
  bool HandleClick(Int_t btn);
private:
  stringmap* _metadata;
  DialogFieldList* _fields;
  TGCanvas* can; // won't auto cleanup, so must do on our own
  TGCompositeFrame* mf2; //container of TGCanvas, need to delete manually
};


#endif
