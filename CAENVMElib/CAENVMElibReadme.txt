
        ----------------------------------------------------------------------

                    --- CAEN SpA - Computing Systems Division --- 

        ----------------------------------------------------------------------
        
        CAENVMElib Library Readme file
        
        ----------------------------------------------------------------------

        Package for Linux kernels 2.4, 2.6

        September 2007


 The complete documentation can be found in the User's Manual on CAEN's web
 site at: http://www.caen.it.


 Content
 -------

 CAENVMELibReadme.txt       : This file.

 CAENVMELibReleaseNotes.txt : Release Notes of the last software release.

 driver                     : Directory containing the drivers.

 lib                        : Directory containing the library binary file
                              and an install script .

 upgrade                    : Directory containing a console application for
                              the firmware upgrade of the CAEN V1718/V2718
                              modules.

 labview-6.0                : Directory containing the LabView library of VIs.

 sample                     : Directory containing a sample program with
                              source code.

 include                    : Directory containing the relevant header files.


 System Requirements
 -------------------

 - CAEN V1718 USB-VME Bridge and/or CAEN V2718 PCI-VME Bridge
 - Linux kernel Rel. 2.4 or 2.6 with gnu C/C++ compiler


 Installation notes
 ------------------

  - Login as root

  - Copy the needed files on your work directory

To install the V2718 device driver:

  - Go to the V2718 driver directory and
  
  - Excecute: cp ./Makefile.2.4 Makefile (for 2.4 kernel) cp ./Makefile.2.6 Makefile (for 2.6 kernel)

  - Execute: make

  - Execute: sh v2718_load.2.4 (for 2.4 kernel) 
             or sh v2718_load.2.6 (for 2.6 kernel)

To install the V1718 device driver:

  - Go to the V1718 driver directory and

  - Excecute: cp ./Makefile.2.4 Makefile (for 2.4 kernel) cp ./Makefile.2.6 Makefile (for 2.6 kernel)

  - Execute: make

  - Execute: sh v1718_load.2.4 ( for 2.4 kernel) 
             or sh v1718_load.2.6 ( for 2.6 kernel)

To install the dynamic library:

  - Go to the library directory

  - Execute: sh install

 The installation copies and installs the library in /usr/lib.

