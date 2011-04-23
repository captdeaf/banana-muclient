/** telnet.h
 *
 * Defines the known telnet opcodes, etc.
 */

#ifndef _MY_TELNET_H_
#define _MY_TELNET_H_

#define IAC    "\xFF"
#define DONT   "\xFE"
#define DO     "\xFD"
#define WONT   "\xFC"
#define WILL   "\xFB"
#define SB     "\xFA"
#define GA     "\xF9"
#define SE     "\xF0"

#define _IAC    0xFF
#define _DONT   0xFE
#define _DO     0xFD
#define _WONT   0xFC
#define _WILL   0xFB
#define _SB     0xFA
#define _GA     0xF9
#define _SE     0xF0

#define IS      "\0"
#define SEND    "\x01"

#define _IS     0
#define _SEND   1

#define ECHO           "\x01"
#define TTYPE          "\x18"
#define NAWS           "\x1F"
#define LINEMODE       "\x22"
#define REMOTEFLOW     "\x21"
#define BINARY         "\x00"
#define EOR            "\x19"
#define MSSP           "\x46"

#define _ECHO           0x01
#define _TTYPE          0x18
#define _NAWS           0x1F
#define _LINEMODE       0x22
#define _REMOTEFLOW     0x21
#define _BINARY         0x00
#define _EOR            0x19
#define _MSSP           0x46


#endif
