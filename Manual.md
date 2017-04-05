# DwDebug manual


![Simple out of circuit hardware](https://github.com/dcwbrown/dwire-debug/blob/master/simple-hardware.jpg?raw=true)

#### Contents

&nbsp;&nbsp;&nbsp;&nbsp;[**Goals**](#Goals)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Command line and parameters**](#command-line-and-parameters)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Connecting to the debugWIRE device**](#connecting-to-the-debugwire-device)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Loading a program to flash**](#loading-a-program-to-flash)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**User interface**](#user-interface)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Command format**](#command-format)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Interaction**](#interaction)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Data, flash, register, and stack display commands**](#data-flash-register-and-stack-display-commands)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Running the program**](#running-the-program)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[**Disassembly**](#disassembly)<br>



#### Goals

 - Provide a simple debugger for Atmel devices which support the debugWIRE protocol.
 - Use simple and cheap hardware - an FT232R USB to serial adapter and a diode.
 - Work independently of Atmel Studio.
 - Support Linux and Windows (via mingw on cygwin).


#### Command line and parameters

The dwire-debug executable is `dwdebug.exe` on Windows and `dwdebug` on Linux.

The dwdebug command parser reads text first from the command line and then from standard input.

Commands are separated by comma or by end of line.

For example

```
dwdebug f0,q
```

Finds and connects to a dwire device, dumps the first 128 bytes of flash, and quits before reading from standard input:

```
$ ./dwdebug f0,q
................
Connected to DebugWIRE device on USB serial port ttyUSB0 at baud rate 8069
Device recognised as ATtiny84
0000:   0e c0 18 95  26 c0 18 95   18 95 18 95  18 95 18 95   ....&...........
0010:   18 95 18 95  25 c0 18 95   18 95 18 95  18 95 0f e7   ....%...........
0020:   0d bf 00 e0  0e bf 08 e1   07 bb 07 e0  08 bb 01 e0   ................
0030:   05 bb 00 e2  0b bf 02 e0   0a bd 04 e0  03 bf 07 e2   ................
0040:   09 bd 00 e1  00 e3 05 bf   22 b7 33 27  44 27 78 94   ........".3'D'x.
0050:   ff cf c0 99  03 c0 a2 b6   a2 1a 3a 0d  22 b7 18 95   ..........:."...
0060:   0f 92 1f 92  0f b6 c0 9b   03 c0 a2 b6  a2 1a 3a 0d   ..............:.
0070:   3d 31 08 f0  4e e1 22 b7   33 27 44 23  61 f0 4a 95   =1..N.".3'D#a.J.
$
```


#### Connecting to the debugWIRE device

The ```device``` command can be used to specify both the port and baud rate, or to initiate a scan for a suitable device.

Use the ```device``` command with no parameters to find the first connected device and its baud rate. For
example (here on Windows):

```
$ ./dwdebug device
..............
Connected to DebugWIRE device on USB serial port COM4 at baud rate 7839
Device recognised as ATtiny85
0000: c00e  rjmp  001e (+15)            >
```

The ```device``` command can be given the port name and baud rate, saving the time taken by the baudrate determination algorithm. For example (here on Linux):

```
$ ./dwdebug device 0 8060

Connected to DebugWIRE device on USB serial port ttyUSB0 at baud rate 8060
Device recognised as ATtiny84
0000: c00e  rjmp  001e (+15)            >
```

You can use the `verbose` command to show the port and baud rate hunting in action:

```
$ ./dwdebug verbose,device
Trying COM1, baud rate 150000, break length 0050. Cannot set this baud rate, probably not an FT232.
Trying COM4, baud rate 150000, break length 0050, skipping [00], received 00000000, scale 20%
Trying COM4, baud rate 030000, break length 0050, skipping [0], received 01111000: 8 010110000, scale 40%
Trying COM4, baud rate 012000, break length 0050, skipping [0], received 10010010: 7 032000000, scale 85%
Trying COM4, baud rate 010200, break length 0050, skipping [0], received 01001011: 8 042000000, scale 85%
Trying COM4, baud rate 008670, break length 0050, skipping [0], received 10100101: 7 051000000, scale 95%
Trying COM4, baud rate 008236, break length 0050, skipping [0], received 01010101: expected result.
Finding upper bound.
Trying COM4, baud rate 008400, break length 0012, skipping [0], received 11010101: 6 060000000
Finding lower bound.
Trying COM4, baud rate 008071, break length 0012, skipping [0], received 01010101: expected result.
Trying COM4, baud rate 007909, break length 0012, skipping [0], received 01010101: expected result.
Trying COM4, baud rate 007750, break length 0012, skipping [0], received 01010101: expected result.
Trying COM4, baud rate 007595, break length 0012, skipping [0], received 01010101: expected result.
Trying COM4, baud rate 007443, break length 0012, skipping [0], received 01010101: expected result.
Trying COM4, baud rate 007294, break length 0012, skipping [0], received 10010101: 7 051000000
, skipping [0]
Connected to DebugWIRE device on USB serial port COM4 at baud rate 7839
Device recognised as ATtiny85
0000: c00e  rjmp  001e (+15)            >
```

#### Loading a program to flash

Dwdebug can load flash from either a pure binary file, or an ELF formatted file.

DwDebug minimises flash wear as follows:

 - Flash pages that already contain the data being loaded are not touched.

 - Flash pages are not erased unless the data being loaded contains '1' bits where the flash contains '0's.

 - When loading a page that differs from flash and is all 0xFF, the flash page will only be erased (which by itself leaves the flash as all 0xFFs.)

When loading an ELF file, DwDebug extracts line number and symbol information and uses these to annotate the disassembly. See the dissamble command below for more details.



#### User interface

The dwdebug prompt is '>' which is displayed to the right of output. This way a sequence of dump or unassemble commands form a continuous listing. For example:

```
$ ./dwdebug device 0 8060

Connected to DebugWIRE device on USB serial port ttyUSB0 at baud rate 8060
Device recognised as ATtiny84
0000: c00e  rjmp  001e (+15)            > d
0000:   ff ef ff ff  ff ff ff ff   ff ff ff ff  ff ff 00 02   ................
0010:   30 ff 00 00  00 7f ff ff   ff fd 4f f9  ff ff 00 02   0.........O.....
0020:   00 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00   ................
0030:   00 00 00 00  00 00 00 00   00 00 00 00  00 00 ff 00   ................
0040:   00 00 00 81  00 00 03 00   00 00 00 00  00 00 00 00   ................
0050:   00 78 00 00  01 30 00 00   00 00 00 00  00 5f 02 82   .x...0......._..
0060:   88 e7 4a 6c  25 37 11 51   2e c5 16 48  02 a4 01 32   ..Jl%7.Q...H...2
0070:   0d d8 04 4c  24 13 af 59   56 9f 89 67  22 75 1e 54   ...L$..YV..g"u.T  > d
0080:   84 0a 0b c4  a4 90 42 78   06 f4 90 c3  ab 6d 8e 71   ......Bx.....m.q
0090:   0e 1a 02 cc  46 af 22 d4   ee a7 26 ec  26 49 0e 72   ....F."...&.&I.r
00a0:   0d 85 02 83  81 6c 22 44   42 9b 29 e5  ac 05 ae c7   .....l"DB.).....
00b0:   d6 00 83 c1  fc 88 01 7d   5c 25 a3 49  28 3f ad 01   .......}\%.I(?..
00c0:   62 06 cb 6c  27 25 80 d4   f6 50 24 90  83 18 03 43   b..l'%...P$....C
00d0:   2a 77 42 c4  00 81 3c 67   df 10 2c a3  4c 4a 32 c4   *wB...<g..,.LJ2.
00e0:   b8 75 10 e4  74 77 07 13   b6 bc 02 65  02 00 03 d9   .u..tw.....e....
00f0:   1e 5c 02 a8  48 cc 1c e1   dc 88 22 e0  65 a4 93 d1   .\..H.....".e...  > d
0100:   b2 cf 0f ec  2c 48 8f f1   07 65 08 48  0c 10 37 24   ....,H...e.H..7$
0110:   de 33 b2 74  0a 18 53 13   86 16 e3 ed  44 ae 2a 04   .3.t..S.....D.*.
0120:   6e 40 b6 26  04 a1 41 b1   ce 70 2a e6  45 95 c6 e5   n@.&..A..p*.E...
0130:   82 14 45 0b  18 cc e7 44   da 99 07 ca  6c a0 62 10   ..E....D....l.b.
0140:   4e 48 27 c2  39 2c 37 07   ca 82 a2 c1  d4 20 8e 31   NH'.9,7...... .1
0150:   86 04 92 d2  a5 46 af c1   ae 3b ec b8  1d 61 56 38   .....F...;...aV8
0160:   0e 0e 40 18  14 55 28 51   8c 10 64 f4  8e d5 33 45   ..@..U(Q..d...3E
0170:   df 0e 00 e2  8e 1c 4d 73   9b 68 90 84  02 52 26 c1   ......Ms.h...R&.  >
```

The ```h``` command displays simple help:

```
  b         - Set breakpoint
  bc        - Clear breakpoint
  d         - Dump data bytes
  dw        - Dump data words
  f         - Dump flash bytes
  fw        - Dump flash words
  l         - Load file
  g         - Go
  p         - PC set / query
  q         - Quit
  r         - Display registers
  s         - Stack
  t         - Trace
  te        - Timer enable
  td        - Timer disable
  u         - Unassemble
  h         - Help
  reset     - Reset processor
  help      - Help
  device    - Device connection port
  verbose   - Set verbose mode
  gdbserver - Start server for GDB
```


#### Command format

Most commands are a single letter while a few require a full word to be typed.

Parameters are separated by spaces.

No space is required between a command and its first parameter. Use spaces to
separate multiple parameters.

Numbers are recognised in most contemporary formats:

Base         |  Examples
---          |  ---
hexadecimal  |  `1234`, `1234h`, `$1234`, `0x1234`
decimal      |  `1234`, `1234d`, `#1234`

`addr` and `len` arguments default to hexadecial. The trace instruction
`count` parameter, and register numbers default to decimal.


#### Interaction

The prompt in most cases is the current instruction, for example, following
reset:

````
0000: c00f  rjmp  0010 (+16 words)      >
````

The following transcript shows use of the `t` trace command and `r` register
command assuming a previous `reset` command:

````
0000: c00f  rjmp  0010 (+16 words)      > t
0010: 2400  clr   r0                    > t
0011: 27ff  clr   r31                   > t
0012: e1ee  ldi   r30, $1e              > t
0013: 9202  st    r0, -Z                > t
0014: 23ee  tst   r30                   > t
0015: f7e9  brne  0013 (-2 words)       > t
0013: 9202  st    r0, -Z                > t
0014: 23ee  tst   r30                   > t
0015: f7e9  brne  0013 (-2 words)       > r
r0 00   r4 00    r8 00   r12 00   r16 10   r20 14   r24 18   r28 00
r1 00   r5 00    r9 00   r13 00   r17 11   r21 15   r25 00   r29 c0
r2 00   r6 00   r10 00   r14 00   r18 12   r22 16   r26 00   r30 1c
r3 00   r7 00   r11 00   r15 00   r19 13   r23 17   r27 00   r31 00
SREG i t h s v n z c    PC 0015  SP 015f   X 0000   Y c000   Z 001c
0015: f7e9  brne  0013 (-2 words)       > t
0013: 9202  st    r0, -Z                > t
0014: 23ee  tst   r30                   > t
0015: f7e9  brne  0013 (-2 words)       > t
0013: 9202  st    r0, -Z                > t
0014: 23ee  tst   r30                   > t
0015: f7e9  brne  0013 (-2 words)       > r
r0 00   r4 00    r8 00   r12 00   r16 10   r20 14   r24 18   r28 00
r1 00   r5 00    r9 00   r13 00   r17 11   r21 15   r25 00   r29 c0
r2 00   r6 00   r10 00   r14 00   r18 12   r22 16   r26 00   r30 1a
r3 00   r7 00   r11 00   r15 00   r19 13   r23 17   r27 00   r31 00
SREG i t h s v n z c    PC 0015  SP 015f   X 0000   Y c000   Z 001a
0015: f7e9  brne  0013 (-2 words)       >
````

#### Data, flash, register, and stack display commands

Data and flash may be displayed by byte or by word.

Command | Does
------- | ----
`d`     | Display data memory content by byte
`dw`    | Display data memory content by word
`f`     | Display flash memory content by byte
`fw`    | Display flash memory content by word

Data addressing is as used by processor data access instruction. That is, the first 32 bytes of the data address space correspond to the registers r0 .. r32. Subsequent data address space contains the io register set, followed by RAM.

Each command accepts none, one or two space separated parameters: being the start address and length.

If no parameter is specified, the dump starts where the previous dump left off.

If no length is specified, DwDebug dumps 128 (0x80) bytes.

For example, to display the first 16 bytes of flash by word:

```
0000: c00e  rjmp  001e (+15)            > fw0 10
0000:   c00e  9518   c026  9518    9518  9518   9518  9518    ....&...........  >
```

Command | Does
------- | ----
`r`     | Display all processor registers (not IO registers)
`s`     | Display 16 bytes of stack

As well as appearing at address 0 in the data display, the registers may be displayed in an annotated layout with the ```r``` command:

```
0000: c00e  rjmp  001e (+15)            > r
r0 00   r4 00    r8 00   r12 20   r16 30   r20 00   r24 55   r28 00
r1 00   r5 00    r9 00   r13 88   r17 44   r21 10   r25 bf   r29 95
r2 00   r6 00   r10 a4   r14 40   r18 1a   r22 00   r26 20   r30 04
r3 00   r7 01   r11 10   r15 00   r19 00   r23 20   r27 09   r31 00
SREG I t h s v n Z c    PC 0000  SP 007f   X 0920   Y 9500   Z 0004
0000: c00e  rjmp  001e (+15)            >
```

The ```s``` command displays 16 bytes of data memory starting at SP+1. (SP addresses the byte below the last pushed data.)


#### Running the program

The following commands may be used to control execution:

Command | Argument      | Does | Notes
------- | ----          | ---- | -----
`p`     | flash address | Set PC to address | Address is a flash byte address and must be even
`s`     | data address  | Set SP to nnn.|
`b`     | flash address | Set the execution breakpoint address | Address is a flash byte address and must be even
`bc`    |               | Clear execution breakpoint |
`g`     |               | Go: begin or continue execution at current PC | Stops at breakpoint, or press return to regain control
`t`     | count         | Trace instructions one at a time | count defaults to 1
`te`    |               | Timer enable |
`td`    |               | Timer disable |


##### Notes

 - Remember that push leaves SP one byte below the pushed data, therefore the `s` command displays memory starting at SP+1.
 - Breakpoint functionality uses the breakpoint hardware built into all debugWIRE devices. Although there is only one breakpoint available, it has the advantage that it does not involve modifying the program flash: it causes no flash wear.
 - The `g` command starts execution at PC. Control will return automatically to DwDebug if the breakpoint address is reached. To break in to a running program, press the return key.
 - To start execution at another address use the `p` command before the `g` command.

By default, device timers are disabled when running the program. This may make sense for early debugging when interrupts or other behaviour dependent on timers may make the debugging process more difficult.

Device timers are enabled with the `te` command. With timers enabled, timers will count while the device is running, and therefore may cause interrupts.


#### Disassembly

Cmd | Args       | Does
--- | ---        | ---
`u` | `addr len` | Unassemble - display disassembly of flash memory

Like the dump commands, the unassemble command defaults to start where the
previous disassembly finshed. The default length is 8 instructions.

The dwdebug prompt is normally a dissassembly of the current instruction (at PC).
Keep pressing 'u' without parameters to continue disassembling beyond the current
instruction.

If the program was loaded from an ELF format file, the disassembly will be annotated with line numbers and symbols.

To produce a suitable ELF file, use the `-gstabs` parameter on the avr-as command.

For example:

```
$ ../dwdebug device COM4 7840

Connected to DebugWIRE device on USB serial port COM4 at baud rate 7840
Device recognised as ATtiny85
0000: c00e  rjmp  001e (+15)            > l bellpush.elf
Loading 158 flash bytes from ELF text segment.
Unchanged   $0000 - $003f
Unchanged   $0040 - $007f
Unchanged   $0080 - $00bf

interrupt_vectors:
bellpush.s[46]      0000: c00e  rjmp  reset (+15)           > u
bellpush.s[47]      0002: 9518  reti
bellpush.s[48]      0004: c026  rjmp  bellpush (+39)
bellpush.s[49]      0006: 9518  reti
bellpush.s[50]      0008: 9518  reti
bellpush.s[51]      000a: 9518  reti
bellpush.s[52]      000c: 9518  reti
bellpush.s[53]      000e: 9518  reti
bellpush.s[54]      0010: 9518  reti                        > u
bellpush.s[55]      0012: 9518  reti
bellpush.s[56]      0014: c025  rjmp  cycle (+38)
bellpush.s[57]      0016: 9518  reti
bellpush.s[58]      0018: 9518  reti
bellpush.s[59]      001a: 9518  reti
bellpush.s[60]      001c: 9518  reti

reset:
bellpush.s[66]      001e: e70f  ldi   r16, $7f
bellpush.s[67]      0020: bf0d  out   SPL ($3d), r16        > u
bellpush.s[69]      0022: e000  ldi   r16, bellpush.o ($0)
bellpush.s[70]      0024: bf0e  out   SPH ($3e), r16
bellpush.s[81]      0026: e108  ldi   r16, PORTB ($18)
bellpush.s[82]      0028: bb07  out   DDRB ($17), r16
bellpush.s[84]      002a: e007  ldi   r16, $7
bellpush.s[85]      002c: bb08  out   PORTB ($18), r16
bellpush.s[87]      002e: e001  ldi   r16, $1
bellpush.s[88]      0030: bb05  out   PCMSK ($15), r16      > u
bellpush.s[90]      0032: e200  ldi   r16, $20
bellpush.s[91]      0034: bf0b  out   GIMSK ($3b), r16
bellpush.s[96]      0036: e002  ldi   r16, $2
bellpush.s[97]      0038: bd0a  out   TCCR0A ($2a), r16
bellpush.s[99]      003a: e004  ldi   r16, $4
bellpush.s[100]     003c: bf03  out   TCCR0B ($33), r16
bellpush.s[102]     003e: e207  ldi   r16, CYCLETIME ($27)
bellpush.s[103]     0040: bd09  out   OCR0A ($29), r16      >
```

There is no separate command to load symbols without loading flash. Just use
the load command, it does not erase and write flash when the content is already
present.