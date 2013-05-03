/** @file intarray.hh
    @brief Defines the intarray utility class
    @author bloer
    @ingroup modules
*/

#ifndef INTARRAY_h
#define INTARRAY_h
#include <stdint.h>
#include <limits.h>

/** @class intarray
    @brief Treat an array of any-sized integers as a generic pointer/iterator
    @ingroup modules
*/
class intarray : public std::iterator<std::random_access_iterator_tag,uint64_t>{
  //utilities:
public:
  /// Get the largest value storable in this type of integer
  uint64_t GetMax(){ return _mask; }
  
public:
  /// Constructor; must specify the number of bits of the underlying array
  intarray(int nbits, void* start = 0) : 
    _nbits(nbits), _start((char*)start)
  { 
    if(nbits <= CHAR_BIT) _nchars = 1;
    else if(nbits <=2*CHAR_BIT) _nchars = 2;
    else if(nbits <=4*CHAR_BIT) _nchars = 4;
    else _nchars = 8;
    _mask = 0;
    --_mask;
    _mask = (uint64_t)
      (_mask >> (sizeof(uint64_t)*CHAR_BIT - nbits));
  }
  intarray(const intarray& right) ///< copy constructor
  {
    if( this != &right){
      _nbits = right._nbits;
      _start = right._start;
      _nchars = right._nchars;
      _mask = right._mask;
    }
  }
  intarray& operator=(const intarray& right) ///< assignment operator
  {
    if( this != &right){
      _nbits = right._nbits;
      _start = right._start;
      _nchars = right._nchars;
      _mask = right._mask;
    }
    return *this;
  }
  intarray& operator=(void* start) ///< assignment operator to a raw pointer
  {
    _start = (char*)start;
    return *this;
  }
  
  ~intarray() {}
  

  //operator overloads
  bool operator==(void* start) const
  {
    return _start == (char*)(start);
  }
  bool operator==(const intarray& right) const
  {
    return (_start == right._start && _nbits == right._nbits);
  }
  bool operator!=(const intarray& right) const
  {
    return !operator==(right);
  }
  uint64_t operator[](int index) const
  {
    return (*((uint64_t*)(_start + index*_nchars))) & _mask;
  }
  
  uint64_t operator*() const 
  {
    return (*(uint64_t*)(_start)) & _mask;
  }
  /*
  uint64_t& operator*()
  {
    return *((uint64_t*)(_start));
  }
  */
  intarray& operator++() //prefix++
  {
    _start += _nchars;
    return *this;
  }
  intarray operator++(int) //postfix++
  {
    intarray ans = *this;
    _start += _nchars;
    return ans;
  }
  intarray& operator--() //prefix--
  {
    _start -= _nchars;
    return *this;
  }
  intarray operator--(int) //postfix--
  {
    intarray ans = *this;
    _start -= _nchars;
    return ans;
  }
  intarray operator+(int d) const
  {
    return intarray(_nbits, _start+_nchars*d);
  }
  intarray operator-(int d) const
  {
    return intarray(_nbits, _start-_nchars*d);
  }
  int operator-(const intarray& a) const
  {
    return  (_start-a._start)/_nchars;
  }
  intarray& operator+=(int d)
  {
    _start += _nchars*d;
    return *this;
  }
  intarray& operator-=(int d)
  {
    _start -=_nchars*d;
    return *this;
  }

private:
  int _nbits;
  char* _start;
  int _nchars;
  uint64_t _mask;

public:
  //typedefs for interator_traits to use:
  typedef int difference_type ;
  typedef uint64_t value_type;
  typedef uint64_t* pointer ;
  typedef uint64_t& reference ;
  typedef std::random_access_iterator_tag iterator_category ;
};

#endif
