#ifndef DBLIB_H
#define DBLIB_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>


using namespace std;

Bool_t DBLIB_DEBUG = kFALSE;
Bool_t DBLIB_PRINT = kTRUE;
string mysql_hostname="blackhole.lngs.infn.it";
string mysql_user="analysis";

//String Manipulation

vector<string> tokenize(const string& str,const string& delimiters)
{
    vector<string> tokens;
    
    // skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    
    // find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);
    
    while (string::npos != pos || string::npos != lastPos)
    {
	// found a token, add it to the vector.
	tokens.push_back(str.substr(lastPos, pos - lastPos));
	
	// skip delimiters.  Note the "not_of"
	lastPos = str.find_first_not_of(delimiters, pos);
	
	// find next "non-delimiter"
	pos = str.find_first_of(delimiters, lastPos);
    }
    
    return tokens;
}

//DataBase related stuff

void sql_interface(TString DBname)
{
    //Test utility assuming a certain DBname and Table names
    printf("message\n");
    TSQLServer *db = TSQLServer::Connect("mysql://localhost/","analysis", "");
    printf("Server info: %s\n", db->ServerInfo());	
    TSQLRow *row;
    TSQLResult *res;
	
    // list databases available on server
    printf("\nList all databases on server %s\n", db->GetHost());
    res = db->GetDataBases();
    while ((row = res->Next())) 
    {
	printf("%s\n", row->GetField(0));
	delete row;
    }
    delete res;
	
    // list tables in database "rundb" (the permission tables)
    printf("\nList all tables in database \"rundb\" on server %s\n",
	   db->GetHost());
    res = db->GetTables(DBname);
    while ((row = res->Next())) 
    {
	printf("%s\n", row->GetField(0));
	delete row;
    }
    delete res;
	
    // list columns in table "daqruns" in database "mysql"
    printf("\nList all columns in table \"daqruns\" in database \"rundb\" on server %s\n",
	   db->GetHost());
    res = db->GetColumns(DBname, "daqruns");
    while ((row = res->Next())) 
    {
	printf("%s\n", row->GetField(0));
	delete row;
    }
    delete res;
	
    // query database and print results
    const char *sql = "SELECT runid, amplification FROM rundb.daqruns WHERE (runid>2000) AND (runid<2040) ORDER BY runid";	
    res = db->Query(sql);
	
    int nrows = res->GetRowCount();
    printf("\nGot %d rows in result\n", nrows);
	
    int nfields = res->GetFieldCount();
    for (int i = 0; i < nfields; i++)
	printf("%50s", res->GetFieldName(i));
    printf("\n");
    for (int i = 0; i < nfields*50; i++)
	printf("=");
    printf("\n");
	
    for (int i = 0; i < nrows; i++) 
    {
	row = res->Next();
	for (int j = 0; j < nfields; j++) 
	{
	    printf("%50s", row->GetField(j));
	}
	printf("\n");
	delete row;
    }
	
    delete res;
    delete db;
	
}

string DB_get_string(TString DBname, TString TABLEname, Int_t run_id, TString var, Int_t channel = 0)
{
    //This service function provides a string corresponding to the value of a variable for a specific row.
    //The row corresponds to a combination of channel and run. For DB where the division in channel is not present
    //it provides a srting where all values are divided by commas.
    //Use DB_get_var to access to the single entry also in case of only run ids. 
    //Channel goes from 0 to N-1. Sometimes channels are excluded hence channels != index
    ostringstream os;
	
    //Check if localhost is the machine where MySQL server is running
    FILE *fp;
    char run_hostnamet[30];
    fp = popen("hostname -f","r");
    fscanf(fp,"%s",run_hostnamet);
    fclose(fp);
    if(DBLIB_DEBUG == kTRUE) printf("%s\n", run_hostnamet);
    string run_hostname(run_hostnamet);
	
    os.str("");
    if(run_hostname == mysql_hostname)
    {
	os << "mysql://localhost/";
    }
    else
    {
	os << "mysql://localhost/";//In future put mysql://deathstar.princeton.edu
    }
    // Connect to database
    TSQLServer *db = TSQLServer::Connect(os.str().c_str(),mysql_user.c_str(), "");
    if(DBLIB_DEBUG == kTRUE)
    {
	if(db)
	{
	    cout << "Connected to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	}
	else
	{
	    cout << "Unable to connect to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	    std::exit(0);
	}
    }
    TSQLRow *row;
    TSQLResult *res;
    string result_string;
	
    os.str("");
    if(TABLEname == "daqruns")
    {//This is for Tables in which different channels ARE NOT different rows
	os << "SELECT " << var << " FROM " << DBname << "." << TABLEname << " WHERE runid = " << run_id;
    }
    if(TABLEname == "lasercalibration")
    {//This is for Tables in which different channels ARE different rows
	os << "SELECT " << var << " FROM " << DBname << "." << TABLEname << " WHERE runid = " << run_id << " and channel=" << channel;
    }
    if(DBLIB_DEBUG == kTRUE) 
	cout << os.str() << endl;
    res = db->Query(os.str().c_str());;
	
    int nrows = res->GetRowCount();
    if(DBLIB_DEBUG == kTRUE)
    {
	cout << "Got " << nrows << " rows in result" << endl;
    }
	
    int nfields = res->GetFieldCount();//If SELECTED multiple variables
    if(DBLIB_DEBUG == kTRUE)
    {
	cout << nfields << " variables selected" << endl; 
    }
    for (int i = 0; i < nrows; i++) 
    {
	row = res->Next();
	for (int j = 0; j < nfields; j++) 
	{
	    if(DBLIB_DEBUG == kTRUE)
	    { 
		cout << var << " = " << row->GetField(j) << endl;
	    }
	    result_string = row->GetField(j);
	}
	delete row;
    }
	
    delete res;
    delete db;
    return result_string;
}


string DB_get_var(TString DBname, TString TABLEname, Int_t run_id, TString var, Int_t channel = 0)
{
    //Use this to get variable from DB.
    //Channel goes from 0 to N-1. Sometimes channels are excluded hence channels != index
    string result_string;
    ostringstream os;
    if(TABLEname == "daqruns")
    {
	os.str("");
	os << DB_get_string(DBname, TABLEname, run_id, "channel_ids", channel);
	vector<string> tokens = tokenize(os.str(), ",");
	Int_t exist_id=-1;
	for(int i=0;i<(int)tokens.size();i++)
	{
	    //cout << "tokens[i].c_str()=" << tokens[i].c_str() 
	    //<< " atoi(tokens[i].c_str())=" << atoi(tokens[i].c_str()) 
	    //<< " channel=" << channel << endl;
	    
	    if(atoi(tokens[i].c_str())==channel)
	    {
		if(DBLIB_DEBUG == kTRUE)
		{
		    cout << "tokens[i].c_str()=" << tokens[i].c_str() 
			 << " atoi(tokens[i].c_str())=" << atoi(tokens[i].c_str()) 
			 << " channel=" << channel << endl;
		    cout << "Channel " << channel << " is active" << endl;
		}
		exist_id=i;
		break;
	    }
	}
	if(exist_id==-1)
	{//Error
	    cout << "Channel " << channel << " was deactivated for the analysis (or does not exist)!" << endl;
	    return 0;
	}
	os.str("");
	os << DB_get_string(DBname, TABLEname, run_id, var, channel);
	vector<string> tokens2 = tokenize(os.str(), ",");
	if(exist_id>(Int_t)tokens2.size())
	{//Error
	    cout << "The variable " << var << " refers to the run and not only to channel " << channel << endl;
	}
	else
	{	
	    if(DBLIB_DEBUG == kTRUE) cout << tokens2[exist_id] << endl;
	    result_string = tokens2[exist_id];
	}
    }
    if(TABLEname == "lasercalibration")
    {
	result_string = DB_get_string(DBname, TABLEname, run_id, var, channel);
    }
    return result_string;
}

Double_t DB_get_spe(TString DBname, TString TABLEname, Int_t run_id, Int_t channel = 0)
{
	
    //Channel goes from 0 to N-1. Sometimes channels are excluded hence channels != index
    ostringstream os;
    Double_t spe=0;
	
    //Check if localhost is the machine where MySQL server is running
    FILE *fp;
    char run_hostnamet[30];
    fp = popen("hostname -f","r");
    fscanf(fp,"%s",run_hostnamet);
    fclose(fp);
	
    string run_hostname(run_hostnamet);
	
    os.str("");
    if(run_hostname == mysql_hostname)
    {
	os << "mysql://localhost/";
    }
    else
    {
	os << "mysql://localhost/";//In future put mysql://deathstar.princeton.edu
    }
    //Connect to database
    TSQLServer *db = TSQLServer::Connect(os.str().c_str(),mysql_user.c_str(), "");
    if(DBLIB_DEBUG == kTRUE)
    {
	if(db)
	{
	    cout << "Connected to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	}
	else
	{
	    cout << "Unable to connect to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	    std::exit(0);
	}
    }
    TSQLRow *row;
    TSQLResult *res;
    string result_string;
    const long max_time_sep = 60*60*24;//60*60*24*14; //two weeks
    os.str("");
    //os << "SELECT runid,pdfmean,ABS(runid-" << run_id << ") AS dr,ABS(UNIX_TIMESTAMP('" 
    //  << DB_get_var(DBname, "daqruns", run_id, "starttime") <<"')-UNIX_TIMESTAMP(runtime)) AS dt FROM " 
    //  << DBname << "." << TABLEname 
    //  << " WHERE useme=true and channel="<< channel 
    //  << " HAVING dt < "<< max_time_sep << " ORDER BY dr ASC";
    
    os << "SELECT runid,pdfmean,ABS(runid-" << run_id 
       << ") AS dr,ABS(UNIX_TIMESTAMP('" << DB_get_var(DBname, "daqruns", run_id, "starttime") 
       <<"')-UNIX_TIMESTAMP(runtime)) AS dt FROM " << DBname << "." << TABLEname 
       << " WHERE useme=true and channel="<< channel 
       << " HAVING dt < "<< max_time_sep 
       << " ORDER BY dt ASC";

    if(DBLIB_DEBUG == kTRUE) cout << os.str().c_str() << endl;
    res = db->Query(os.str().c_str());
    //	
    int nrows = res->GetRowCount();
    if(DBLIB_DEBUG == kTRUE) 
	printf("\nGot %d rows in result\n", nrows);
    if(nrows == 0) 
	return 0.;
    int nfields = res->GetFieldCount();
    for (int i = 0; i < nfields; i++)
	if(DBLIB_DEBUG == kTRUE) printf("%40s", res->GetFieldName(i));
    if(DBLIB_DEBUG == kTRUE) 
	printf("\n");
    for (int i = 0; i < nfields*40; i++)
	if(DBLIB_DEBUG == kTRUE) printf("=");
    if(DBLIB_DEBUG == kTRUE) printf("\n");
	
    for (int i = 0; i < nrows; i++) 
    {
	row = res->Next();
	if (DB_get_var(DBname, "daqruns", run_id, "voltage", channel).compare(DB_get_var(DBname, "daqruns", atoi(row->GetField(0)), "voltage", channel)) == 0){
	    if(DBLIB_DEBUG == kTRUE)
	    {
		for (int j = 0; j < nfields; j++) 
		{
		    printf("%40s", row->GetField(j));
		}
	    }
	    spe = atof(DB_get_var(DBname, "lasercalibration", atoi(row->GetField(0)), "pdfmean", channel).c_str());
	    cout << endl;
	    cout << "Calibration Run" << atoi(row->GetField(0)) << "- Channel[" << channel << "]=";
	    break;
	}
	delete row;
    }
    delete res;
    delete db;
    return spe;
}

bool DB_does_run_exist(TString DBname, TString TABLEname, Int_t run_id)
{
    //CHeck if run exists in database table
    ostringstream os;
	
    //Check if localhost is the machine where MySQL server is running
    FILE *fp;
    char run_hostnamet[30];
    fp = popen("hostname -f","r");
    fscanf(fp,"%s",run_hostnamet);
    fclose(fp);
    if(DBLIB_DEBUG == kTRUE) 
	printf("%s\n", run_hostnamet);
    string run_hostname(run_hostnamet);
	
    os.str("");
    if(run_hostname == mysql_hostname)
	os << "mysql://localhost/";
    else
	os << "mysql://localhost/";//In future put mysql://deathstar.princeton.edu
	
    // Connect to database
    TSQLServer *db = TSQLServer::Connect(os.str().c_str(),mysql_user.c_str(), "");
    if(DBLIB_DEBUG == kTRUE)
    {
	if(db)
	    cout << "Connected to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	else
	{
	    cout << "Unable to connect to " << os.str().c_str() << " as " << mysql_user.c_str() << endl;
	    std::exit(0);
	}
    }
	
    TSQLResult *res;
    os.str("");
    os << "SELECT * FROM " << DBname << "." << TABLEname << " WHERE runid = " << run_id;
	
    if(DBLIB_DEBUG == kTRUE) 
	cout << os.str() << endl;
	
    res = db->Query(os.str().c_str());;
	
    int nrows = res->GetRowCount();
	
    bool exist = false;

    if (nrows > 0)
	exist = true;

    if(DBLIB_DEBUG == kTRUE)
	cout << "Got " << nrows << " rows in result" << endl;
	
    delete res;
    delete db;
    return exist;
}
  
#endif
