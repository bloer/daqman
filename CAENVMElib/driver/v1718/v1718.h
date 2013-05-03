/*
        ----------------------------------------------------------------------

        --- CAEN SpA - Computing Systems Division ---

        a2818.h

        Header file for the CAEN A2818 CONET board driver.

        June  2004 :   Created.

        ----------------------------------------------------------------------
*/
#ifndef _v1718_H
#define _v1718_H

#ifndef VERSION
	#define VERSION(ver,rel,seq) (((ver)<<16) | ((rel)<<8) | (seq))
#endif	


/*
        Defines for the v1718
*/

#define V1718_MAGIC                     'U'

#define V1718_IOCTL_REV                 _IOWR(V1718_MAGIC, 1, v1718_rev_t)


/*
        ----------------------------------------------------------------------

        Types

        ----------------------------------------------------------------------
*/

// Rev 0.2
/*
	Struct for revision argument in ioctl calls
*/
#define V1718_DRIVER_VERSION_LEN	20
typedef struct v1718_rev_t {
        char 		rev_buf[V1718_DRIVER_VERSION_LEN];
} v1718_rev_t;


#endif
