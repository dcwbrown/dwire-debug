dwire-debug manual
------------------

###### Connecting to the com port

###### Command format

Most commands are a single letter, a few, like reset and erase, require a full word
to be typed.

Numbers are recognised in most contemporary formats:
  
  base           |  examples
  ---            |  ---
  hexadecimal    |  1234h, $1234, 0x1234
  decimal        |  1234, 1234d, #1234

###### Command line format


###### Full word commands

command           | Does
---               | ---
reset             | Reset processor
help              | Display manual summary
erase addr length | Erase flash in specified range
erase             | Erase all flash 

###### Loading binary files to flash

command       | Does
---           | ---
n filename    | Specify name of binary file
n             | Invoke OS file chooser UI
l addr        | Load binary file at given address
l             | Load binary file at 0

