/** @file ParameterIOimpl.hh
    @brief defines helper functions and non-templated functions for Parameter IO
    @author bloer
*/

#ifndef PARAMETERIOIMPL_h
#define PARAMETERIOIMPL_h

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include "boost/type_traits.hpp"
#include "boost/shared_ptr.hpp"

#include "VParameterNode.hh"
/** @namespace ParameterIOimpl
    @brief defines helper functions and non-templated functions for Parameter IO
 */
namespace ParameterIOimpl{
  unsigned long ReadUnsignedInt(std::istream& in);

  //lowest level needs meta template to give indent to parameters
  //T inherits from VParameterNode:
  template <class T> inline std::ostream& writeleaf(std::ostream& out, 
						    const T& t,
						    bool showhelp, int indent, 
						    const boost::true_type&)
  { return t.WriteTo(out, showhelp, indent); }
  //T is not a VParameterNode: 
  template <class T> inline std::ostream& writeleaf(std::ostream& out, 
						    const T& t,
						    bool showhelp, int indent, 
						    const boost::false_type&)
  { return out<<t; }
  
  
  inline std::ostream& write(std::ostream& out, const VParameterNode& p,
			     bool showhelp=false, int indent=0)
  {
    std::cerr<<"Writing VParameterNode\n";
    return p.WriteTo(out, showhelp, indent+1);
  }
  
  //basic template
  template<class T> inline std::istream& read(std::istream& in, T& t)
  { return in >> t; }
  
  template<class T> inline std::ostream& write(std::ostream& out, const T& t,
					       bool showhelp=false, 
					       int indent=0)
  { return writeleaf(out, t, showhelp, indent, 
		     boost::is_base_of<VParameterNode,T>()); }
  
  //overload for pointers and shared pointers
  
  template<class T> inline std::istream& read(std::istream& in, T* t)
  { return in >> *t; }
  
  template<class T> inline std::ostream& write(std::ostream& out, const T* t,
					       bool showhelp=false, 
					       int indent=0)
  { return writeleaf(out, *t, showhelp, indent, 
		     boost::is_base_of<VParameterNode,T>()); }
  
  template<class T> inline std::istream& read(std::istream& in, boost::shared_ptr<T>& t)
  { return in >> *(t.get()); }
  
  template<class T> inline std::ostream& write(std::ostream& out, const boost::shared_ptr<T>& t,
					       bool showhelp=false, 
					       int indent=0)
  { return writeleaf(out, *(t.get()), showhelp, indent, 
		     boost::is_base_of<VParameterNode,T>()); }
  
  
  
  
  
  //overload for bools and unsigned ints
  std::istream& read(std::istream& in, bool& b);
  inline std::ostream& write(std::ostream& out, const bool& b, bool, int)
  { return out<<std::boolalpha<< b <<std::noboolalpha; }
  
  inline std::istream& read(std::istream& in, unsigned& u)
  { u = ReadUnsignedInt(in); return in; }
  
  inline std::istream& read(std::istream& in, unsigned char& u)
  { u = ReadUnsignedInt(in); return in; }
  inline std::istream& read(std::istream& in, unsigned short& u)
  { u = ReadUnsignedInt(in); return in; }
  inline std::istream& read(std::istream& in, unsigned long& u)
  { u = ReadUnsignedInt(in); return in; }
  inline std::istream& read(std::istream& in, unsigned long long& u)
  { u = ReadUnsignedInt(in); return in; }
  

  
  inline std::ostream& write(std::ostream& out, const unsigned& u, bool, int)
  { return out<<std::hex<<std::showbase<< u <<std::noshowbase<<std::dec; }
  inline std::ostream& write(std::ostream& out, const unsigned char& u, bool, int)
  { return out<<std::hex<<std::showbase<< u <<std::noshowbase<<std::dec; }
  inline std::ostream& write(std::ostream& out, const unsigned short& u, bool, int)
  { return out<<std::hex<<std::showbase<< u <<std::noshowbase<<std::dec; }
  inline std::ostream& write(std::ostream& out, const unsigned long& u, bool, int)
  { return out<<std::hex<<std::showbase<< u <<std::noshowbase<<std::dec; }
  inline std::ostream& write(std::ostream& out, const unsigned long long& u, bool, int)
  { return out<<std::hex<<std::showbase<< u <<std::noshowbase<<std::dec; }
  
  
  
  //container overloads
  //overload for vector
  template<class T> std::istream& read(std::istream& in, std::vector<T>& v);
  template<class T> std::ostream& write(std::ostream& out, 
					const std::vector<T>& v,
					bool showhelp=false, int indent=0);
  //overload set
  template<class T> 
  std::istream& read(std::istream& in, std::set<T>& s);
  template<class T>
  std::ostream& write(std::ostream& out, const std::set<T>& s,
		      bool showhelp=false, int indent=0);

  //overload map
  template<class A, class B>
  std::istream& read(std::istream& in, std::map<A,B>& m);
  template<class A, class B>
  std::ostream& write(std::ostream& out, const std::map<A,B>& m,
		      bool showhelp=false, int indent=0);

  
  //overload for std::string
  std::istream& read(std::istream& in, std::string& s);
  std::ostream& write(std::ostream& out, const std::string& s, bool, int);
  //overload for std::pair
  template<class A, class B> std::istream& read(std::istream& in, 
						std::pair<A,B>& p);
  template<class B> std::istream& read(std::istream& in, 
				       std::pair<std::string,B>& p);
  template<class A, class B> std::ostream& write(std::ostream& out,
						 const std::pair<A,B>& p,
						 bool showhelp=false,
						 int indent=0);
						 
  //combine IO for all containers using generic definitions
  template<class ConstIterator> std::ostream& writeit(std::ostream& out,
						      ConstIterator a,
						      ConstIterator b,
						      unsigned container=0,
						      bool showhelp=false,
						      int indent=0);
  template<class T, class C> std::istream& readlist(std::istream& in,
						    C& container);
  
  template<class T> inline void insert(const T& t, std::vector<T>& v)
  { v.push_back(t); }
  template<class T> inline void insert(const T& t, std::set<T>& s) 
  { s.insert(t); }
  
  template<class A, class B> 
  inline void insert(const std::pair<A,B>& p, std::map<A,B>& m) 
  { m[p.first]=p.second; }
  
  const char opener[]="[{(";
  const char closer[]="]})";

  //default construction for all classes
  template<class T> struct Constructor {
    static T make() { return T(); }
  };
  template<class T> struct Constructor<T*> {
    static T* make() { return new T(); }
  };
  template<class T> struct Constructor<boost::shared_ptr<T> >{
    static boost::shared_ptr<T> make() { return boost::shared_ptr<T>(new T());}
  };
  
};


//overload for std::vector
template<class T> 
inline std::istream& ParameterIOimpl::read(std::istream& in, std::vector<T>& v)
{
  return readlist<T>(in, v);
}

template<class T> 
inline std::ostream& ParameterIOimpl::write(std::ostream& out, 
					    const std::vector<T>& v,
					    bool showhelp, int indent)
{
  return writeit(out, v.begin(), v.end(), 0, showhelp, indent);
}

//overload for std::set
template<class T> inline 
std::istream& ParameterIOimpl::read(std::istream& in, std::set<T>& s)
{
  return readlist<T>(in, s);
}

template<class T> inline
std::ostream& ParameterIOimpl::write(std::ostream& out, 
				     const std::set<T>& s, 
				     bool showhelp, int indent)
{
  return writeit(out, s.begin(), s.end(), 0, showhelp, indent);
}

//overload for std::map
template<class A, class B> inline
std::istream& ParameterIOimpl::read(std::istream& in, std::map<A,B>& m)
{
  return readlist<std::pair<A,B> >(in, m);
}

template<class A, class B> inline
std::ostream& ParameterIOimpl::write(std::ostream& out, const std::map<A,B>& m,
				     bool showhelp, int indent)
{
  return writeit(out, m.begin(), m.end() , 1, showhelp, indent);
}



  
//overload for std::pair
template<class A, class B> inline
std::istream& ParameterIOimpl::read(std::istream& in, std::pair<A,B>& p)
{
  //allow an opening parenth
  char start='0';
  in>>start;
  const char* startchar = std::find(opener, opener+sizeof(opener), start);
  size_t offset = startchar-opener;
  if(offset >= sizeof(opener))
    in.unget();

  //require a separating colon or comma. specialized for std::strings
  read(in, p.first);
  char sep='0';
  in>>sep;
  if(sep != ':' && sep != ','){
    std::cerr<<"Error reading std::pair; expected : or , between values, got '"
	     <<sep<<"'\n";
    in.setstate(std::ios::failbit);
    return in;
  }
  read(in, p.second);
  if(offset < sizeof(opener)){
    char end='0';
    in>>end;
    //check for end = correct closer? 
  }
  return in;
}

//specialize std::pair read for case where first par is a string
template<class B> inline
std::istream& ParameterIOimpl::read(std::istream& in, 
				    std::pair<std::string,B>& p)
{
  //require a separating colon (:). specialized for std::strings
  read(in, p.first);
  if(p.first[p.first.size()-1] != ':'){
    char sep='0';
    in>>sep;
    if(sep != ':'){
      std::cerr<<"Error reading std::pair; expected ':' between values, got '"
	       <<sep<<"'\n";
      in.setstate(std::ios::failbit);
      return in;
    }
  }
  else{
    p.first.resize(p.first.size()-1);
  }
  return read(in, p.second);
}

template<class A, class B> inline
std::ostream& ParameterIOimpl::write(std::ostream& out,const std::pair<A,B>& p,
				     bool showhelp, int indent)
{
  write(out, p.first, showhelp, indent)<<" : ";
  return write(out,p.second, showhelp, indent);
}




template<class ConstIterator>
inline std::ostream& ParameterIOimpl::writeit(std::ostream& out,
					      ConstIterator a,
					      ConstIterator b,
					      unsigned container,
					      bool showhelp,
					      int indent)
{
  if(container >= sizeof(opener))
    container = 0;
  
  out<<opener[container]<<' ';
  while(a != b){
    write(out, *a, showhelp, indent);
    out<<" ";
    if(++a != b)
      out<<", ";
  }
  return out<<closer[container];
}


template<class T, class C> inline 
std::istream& ParameterIOimpl::readlist(std::istream& in,C& container)
{
  //read one character at a time until we reach an opener 
  bool emptyfirst = true;
  char start='0';
  in>>start;
  if(start == '+'){
    emptyfirst = false;
    in>>start;
  }
  
  const char* startchar = std::find(opener, opener+sizeof(opener), start);
  size_t offset = startchar-opener;
  
  if( offset >= sizeof(opener) ){
    std::cerr<<"Error reading dynamic list; expect one of "<<opener
	     <<" at beginning of list, got '"<<start<<"'\n";
    in.setstate(std::ios::failbit);
    return in;
  }
  
  if(emptyfirst)
    container.clear();
  
  char next;
  while( in >> next && next != closer[offset] ){
    if(next == ',')
      continue;
    else if(next == '#'){ //comment, so kill the rest of the line
      std::string dummy;
      std::getline(in, dummy);
      continue;
    }
    in.unget();
    T t(Constructor<T>::make());
    if(!read(in,t))
      return in;
    insert(t, container);
  }
  
  return in;
  
}

#endif
