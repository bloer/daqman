/** @mainpage
    @section _installation_sec Installation
     
    @subsection _git_sec Get the sources from git
    The source code for daqman resides on github. You can clone 
    the full repository via:
    git clone https://github.com/bloer/daqman.git
    
    @subsection _compiling_sec Compiling the source code
    daqman requires several libraries to be installed and properly configured 
    in order to compile and link fully.  Development versions of the libraries 
    (i.e. including the header files) are required.  The required libraries are:
    
       - root (version 5.26 or greater)
       - boost (version 1.35 or greater)

    In addition, to compile the <i>daqman</i> executable which talks to 
    hardware digitizers, you must also have the CAEN VME library installed. 
    (The header files for this library are included in the source distribution.)
    
    Once all the required dependencies are installed in the default system 
    locations (include and library paths), one should be able to compile the 
    whole distribution by calling 'make'.  One can pass options to the compiler
    by using the DEBUGFLAGS variable.  I.e., to compile with optimizations, 
    call  ' make DEBUGFLAGS=-O2 '.

    If it doesn't compile I assume it's your fault  :P

    @subsection _more_instructions Text of an email with some slightly more detailed instructions
    
    Documentation on installing and using it is sparse; what does exist is in this doxygen site.  

I assume you're running some flavor of linux; I've made no attempt to get anything to work under windows.  In order to compile, you'll need to have installed and properly configured the CAENVME library, which you can get from CAEN's website, the makedepend tool, ROOT version 5.24 or greater, and boost 1.38 or greater (at least the thread, date-time, and smart ptr libraries).   I haven't been ambitious enough to write any configure scripts or anything, so you're kind of on your own for figuring out when things go wrong with the make process (though of course you can email me, and I'll try to answer as best as I can).  In particular, the boost libraries get different naming conventions on different distros, so it may be necessary to make a link from boost_<lib>-mt.so to boost_<lib>.so or vice verse.

Hardware wise, you'll need the digitizer itself and the A2818 PCI-fiber interface from CAEN for communication.  It should work either connected directly to the fiber interface on the digitizer or using a V2718 crate controller.   At various points, we've had this software working with V1720, V1721, V1724, and V1731.  It's been a while though, so I won't guarantee anything but the V1720 at this point; each needed some very slight modifications since CAEN can't bother to keep to standard specs.  I've also had issues with the A2818 driver in some linux flavors, and occasionally had to modify its source code to deal with it looking for deprecated headers.  I've also had it refuse to stay installed over a reboot.

If you manage to get this far and get everything compiled, the first thing to try is the boardcheck program.  Its purpose is to scan the provided address section of the VME bus and see what boards are there.  It expects a range of addresses, so if your boards address (defined by the four rotary dials on the back of the digitizer) is 3210 (which I think is default), you would call './boardcheck 32100000 32100000'.  It hasn't been updated in a while, so it may tell you that the type of board is unknown; the important thing is whether it finds it or not.  If it doesn't find it, the most common causes I've found are that the a2818 driver isn't properly installed (to check you can do 'lsmod | grep a2818') or some VME crates don't have the full +12V line, which these boards need.

Finally, to actually take data, you'll use the daqman executable.  All the executables generated can take a -h or --help switch which lists all of the command line options; there's a lot for daqman.  The most common ones are '-f <filename>', '-d <output directory>', '--no-db' (which you should give if you compiled the database stuff since otherwise it will try to load calibration stuff from our database every time you run), '-e <stop after # events>' and '-t <stop after seconds>'.   The rest of the configuration is all handled by the daqman.cfg file.  The modules section handles processing of the events and the live event/spectrum displays, and the V172X section handles the daq configuration (which channels to enable, # of samples to store, etc).  The documentation for these is on the page for the V172X_Params class (and its subclasses) in this doxygen area, or you can use the --show-parameters switch to daqman to get interactive help.  




    @section _getting_started_sec Getting started
    Once the compilation and linking are done, there is a number of ways you 
    can proceed, depending on your needs. Next is a list of available 
    executables and shell scripts that you can use. Furthermore, you can get 
    more information on almost all of the executables with the '-h' flag.

    @subsection _executables_subsec Executables

    - daqman: acquire data from digitizers. Can give command-line options
    - daqroot: same as ROOT, but with some extra functions
    - daqview: view events one-by-one from a data file. Each channel has an ID (0-7 are PMTs, -2 is the sum). If you use the --cfg option to provide a configuration file, you can have peak finders and other nice things
    - ascii_dump: write pulses to text files
    - boardcheck: checks digitizers' status
    - genroot: creates ROOT tree from raw data file
    - laserrun: takes a processes ROOT file and fits the single photon response of each channel; optionally saves the result to the database.
    - run_info: given a raw data file, prints out the run information
    - updatefile: modifies a raw data file according to command-line options and cfg file
    - lasermc: Create MC laser events using real empty events
    - s1mc: Create MC primary signal using real empty events
    - mcoptical: MC full events from optical simulation

    @subsection _shell_scripts_subsec Shell scripts

    - dbupdate.sh: change comment or run type on run database
    - official_run.sh: starts a data-taking run?
    - exportlibs.sh: Warp veto stuff. Probably obsolete

    @section _add_module_sec Adding new processing modules
    All processing modules inherit from BaseModule.  Modules which only 
    process channel-level information and not the overall event should inherit
    instead from ChannelModule.  All modules MUST override at least the 
    Process() function, and may optionally override the Initialize() and 
    Finalize() function.  All modules additionally must define a 
    static member function GetDefaultName which returns the name of the module
    as a std::string.  

    If the new module depends on any other module to run first, these 
    these dependencies must be explicitly declared in the .cc file.  See the 
    documentation for the BaseModule class for more details.
    
    If your module has parameters that you would like to specify via the 
    configuration file, these should be registered in the constructor, 
    using the format ' RegisterParameter("parameter_name", variable) ; '
    
    @section _add_root_section Adding output classes to the ROOT tree
    
    Two steps are necessary to generate a new dictionary to store a class in a 
    ROOT file or tree.  First, in the class header file, you must call the 
    macro 
    ClassDef(CLASSNAME,VERSION)

    
    So the definition would look something like
    
    class MyClass{ <br>
    public:<br>
      MyClass(); <br>
      ~MyClass(); <br>
       //blah blah fun ctions <br>
      ClassDef(MyClass,1) <br>
    };

    Second, you must add your class to the LinkDef.h file in the top level 
    directory.  You must also add any template definitions, such as vectors 
    of your class, if any are to be used.  The link commands must be in the 
    order that they are used, lowest level first.  e.g., if you have a class 
    Container which contains a vector<MyClass> from above, 
    you would add the following lines to LinkDef.h:

    #ifdef __CINT__                                        <br>
    #pragma link C++ class MyClass+;                       <br>
    #pragma link C++ class std::vector<MyClass>+;          <br>
    #pragma link C++ class Container+;                     <br>
    #endif                                                 <br>

    Notice the trailing + at the end of every link command.  
    If you try to link Container before vector<MyClass>, it will not work.
    
    @section _add_daqroot_sec Adding functions to daqroot
    To add classes to the daqroot environment, use the instructrions above 
    to add the classes to the LinkDef file and use the ClassDef macro.  In 
    order to add functions at the global scope, All one has to do is add the 
    word ClassDef somewhere in the header file which defines the function, and
    the Makefile should take care of the rest.

*/
    
