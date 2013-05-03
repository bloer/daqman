/** @file kbhit.h
    @brief defines the keyboard utility class for terminal getch emulation
    @ingroup daqman
*/

///////////////////////////////////////////////////////////
// KBHIT.H

#ifndef KBHITh
#define KBHITh

#include <termios.h>
/** @class keyboard
    @brief allows single character commands on a linux terminal
    @ingroup daqman
*/
class keyboard
{
public:

  keyboard();
  ~keyboard();
  int kbhit();
  int getch();
  
 private:
  
  struct termios initial_settings, new_settings;
  int peek_character;

};

#endif 
