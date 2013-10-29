#include "Message.hh"
#include "ParameterList.hh"
#include "ConfigHandler.hh"
#include "CommandSwitchFunctions.hh"
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>

using namespace std;

template<class T> void f(T& t){ cout<<"First implementation"<<endl; }
void f(double d) { cout<<"Second implementation"<<endl; }
template<class T> void f(T* t) { cout<<"Third implementation"<<endl; }
void f(int* i){ cout<<"Fourth implementation"<<endl; }

template<class T> void f(std::vector<T>& v){ cout<<"Fifth implementation"<<endl;}

int main(int argc, char** argv)
{
  
  int a;
  double b;
  f(a);
  f(b);
  f(&a);
  f(&b);
  
  std::vector<int> v;
  f(v);
  
  ParameterList list("mylist","a test list");
  int i=14;
  list.RegisterParameter("i",i,"an integer");
  
  string s="string";
  list.RegisterParameter("s",s,"a string");
  
  std::map<std::string, double> smap;
  smap["pi"] = 3.14159;
  smap["x"] = -2.11;
  list.RegisterParameter("smap", smap, "string:double map");
  
  vector<int> ivec;
  ivec.push_back(3);
  ivec.push_back(11);
  ivec.push_back(-4);
  list.RegisterParameter("ivec",ivec, "A vector of integers");
  
  vector<string> svec;
  svec.push_back("Hello");
  svec.push_back("World");
  svec.push_back("A much longer string");
  list.RegisterParameter("svec",svec, "A vector of strings");
  
  
  vector< vector<string> > nest;
  vector<string> filler;
  filler.push_back("a line");
  filler.push_back("a second line");
  nest.push_back(filler);
  filler.clear();
  filler.push_back("one");
  filler.push_back("2");
  filler.push_back("III");
  nest.push_back(filler);
  filler.clear();
  filler.push_back("done");
  nest.push_back(filler);
  list.RegisterParameter("nest",nest, "Nested vectors of strings");
  


  cout<<list<<endl;

  std::vector<std::string> sv2;
  //ParameterIOimpl::readlist<std::vector<int> >(cin, nest2);
  //ParameterIOimpl::writeit(cout, nest2.begin(), nest2.end());
  //cout<<endl;
  std::map<int, std::string> imap;
  
  ParameterList list2("list2");
  list2.RegisterParameter("sv2" , sv2 , "vector of strings");
  list2.RegisterParameter("smap",smap,"string:double map");
  list2.RegisterParameter("imap",imap,"int:string map");
  cin>>list2;
  cout<<list2<<endl;

  
  return 0;
}
