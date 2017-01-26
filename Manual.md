Manual
------

### Goal

The current implementation (24th May 2015) supports connecting to hardcoded
ports com6/ttyUSB0, and the `reset`, `r`, `rn`, `p` and `t` commands.

The following document fleshes out my aspiration for the full command set.


#### Command line and parameters

The dwire-debug executable is `dwdebug.exe` on Windows and `dwdebug` on Linux.

Command line parameter text is parsed as commands in exactly the same way as
commands read from standard input.

Multiple commands my be entered on one line by separating them with ';'.

#### Connecting to the com port

By default, dwdebug uses the first usb connected serial port it finds. To
override the default, use the port command. For example: `port com7` or
`port ttyUSB2`.


#### Command format

Most commands are a single letter while a few require a full word to be typed.

No space is required between single letter commands and their parameter.

Numbers are recognised in most contemporary formats:

Base         |  Examples
---          |  ---
hexadecimal  |  1234, 1234h, $1234, 0x1234
decimal      |  1234, 1234d, #1234

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

#### Full word commands

Cmd     | Args        | Does
---     | ---         | ---
`help`  |             | Display manual summary
`port`  | `port-name` | Specify serial port. E.g. 'com6' or 'ttyUSB0'
`reset` |             | Reset processor
`erase` | `addr len`  | Erase flash in specified range
`erase` |             | Erase all flash


#### Loading binary files to flash

Cmd | Args       | Does
--- | ---        | ---
`n` | `filename` | Specify name of binary file
`n` |            | Invoke OS file chooser UI
`l` | `addr`     | Load binary file to flash at given address
`l` |            | Load binary file to flash at 0


#### Memory and register display

Cmd  | Args       | Does
---  | ---        | ---
`s`  |            | Show stack as bytes and as words
`r`  |            | Display R0 to R31, SP and SREG
`rn` |            | Display register n
`x`  |            | Display X register (r27:r26 as a word)
`y`  |            | Display Y register (r29:r28 as a word)
`z`  |            | Display Z register (r31:r30 as a word)
`d`  | `addr len` | Hex dump of data memory (registers, io and ram) as bytes
`dw` | `addr len` | Hex dump of data memory (registers, io and ram) as words
`e`  | `addr len` | Hex dump of eeprom as bytes
`ew` | `addr len` | Hex dump of eeprom as words
`f`  | `addr len` | Hex dump of flash memory as bytes
`fw` | `addr len` | Hex dump of flash memory as words

For the dump commands, addr defaults to where the previous dump finished, and
len defaults to 128 for flash memory dumps and 32 for ram and eprom dumps.


#### Disassembly

Cmd | Args       | Does
--- | ---        | ---
`u` | `addr len` | Unassemble - display disassembly of flash memory

Like the dump commands, the unassemble command defaults to start where the
previous disassembly finshed. The default length is 8 instructions.

The dwdebug prompt is normally a dissassembly of the current instruction (at PC).
Keep pressing 'u' without parameters to continue disassembling beyond the current
instruction.


#### Execution

Cmd | Args   | Does
--- | ---    | ---
`b` | `addr` | Sets the one and only DebugWIRE hardware breakpoint address.
`p` | `addr` | Sets PC (addr is an instruction address)
`s` | `addr` | Sets SP
`g` |        | Start executing. The processor will continue until it hits a breakpoint instruction or the breakpoint set with the `b` command.
`t` |        | Trace - execute 1 instruction
`t` | count  | Execute count instructions. Each instruction executed is shown.


