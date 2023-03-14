/*@create-file:build.opt@
// An embedded build.opt file using a "C" block comment. The starting signature
// must be on a line by itself. The closing block comment pattern should be on a
// line by itself. Each line within the block comment will be space trimmed and
// written to build.opt, skipping blank lines and lines starting with '//', '*'
// or '#'.

-fexceptions
*/


#ifndef __MQTT_INO_GLOBALS_H__
#define __MQTT_INO_GLOBALS_H__
/*
#if !defined(__ASSEMBLER__)
// Defines kept away from assembler modules
// i.e. Defines for .cpp, .ino, .c ... modules
#endif

#if defined(__cplusplus)
// Defines kept private to .cpp and .ino modules
//#pragma message("__cplusplus has been seen")
//#define EXAMPLE "Full"
#endif

#if !defined(__cplusplus) && !defined(__ASSEMBLER__)
// Defines kept private to .c modules
//#define EXAMPLE "Full"
#endif

#if defined(__ASSEMBLER__)
// Defines kept private to assembler modules
#endif
*/

#endif