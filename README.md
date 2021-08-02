[![DOI](https://zenodo.org/badge/9839943.svg)](https://zenodo.org/badge/latestdoi/9839943)

# README for daqman 
## Installation on RHEL7
On a clean RHEL7 install, need to install the following package to include:
 - imake
 - boost-devel
 - root
 - root-spectrum
 - zlib-devel

### Notes on compiling, running, etc with the daqman system:

General info on daqman can be found on the github pages:
http://bloer.github.io/daqman
In particular, there is some doxygen code documentation at 
http://bloer.github.io/daqman/doc/html

#### Adding a ROOT class for saving:
Two steps are necessary to generate a new dictionary to store a class in a ROOT file or tree.  First, in the class header file, you must call the macro 
```c++
ClassDef(CLASSNAME,VERSION)
```

So the definition would look something like

```c++
class MyClass{
public:
  MyClass();
  ~MyClass();
  //blah blah functions
  ClassDef(MyClass,1)
};
```

Second, you must add your class to the LinkDef.h file in the top level directory.  You must also add any template definitions, such as vectors of your class, if any are to be used.  The link commands must be in the order that they are used, lowest level first.  e.g., if you have a class Container which contains a `vector<MyClass>` from above, you would add the following lines to LinkDef.h:

```c++
#ifdef __CINT__
#pragma link C++ class MyClass+;
#pragma link C++ class std::vector<MyClass>+;
#pragma link C++ class Container+;
#endif
```

Notice the trailing `+` at the end of every link command.  If you try to link Container before vector<MyClass>, it will not work.
