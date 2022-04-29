/**
 * @file config.hpp
 * @author Francisco Alcaraz
 * @brief Configuration constants
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif

#ifndef CONFIG__HH_INCLUDED
#define CONFIG__HH_INCLUDED

#define MAXNAME 30
#define MAXNUMBER 30
#define MAXATTRS 256
#define MAXARGS 256
#define MAXSTR 4096 /* Biggest text */
#define MAXPATTERNS 100
#define MAXVARS 100

#endif
