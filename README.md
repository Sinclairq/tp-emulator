# tp-emulator
A basic 100-loc CPU emulator using the existing capabilities of ntoskrnl.exe that have existed since Winver 1903. Though some of these functions have been exposed in a public light, no PoC exists to illustrate its capability. 

## Usage
The functions used in this project are undocumented by Microsoft. Initialize the defined function prototypes in the header file, then call EmuInstr( ) freely with the required parameters as described by the comments.

Required functions:
- KiTpEmulateInstruction
- KiTpParseInstructionPrefix
- KiTpReadImageData

Additional functions for calculating EFlags output (for those who would like to expand on this emulator):
- KiTpSetFlagsZeroSignParity
- KiTpSetFlagsSub
- KiTpSetFlagsAdd

## Limitations
As mentioned in the header comments, this emulator cannot emulate some common set instructions:
- *JCCs, SETcc, MOVcc, MOVSX, IMUL/IDIV, INC, and DEC.*

Operations like *PUSH/POP/MOV/CMP/SUB/ADD/RET/INT3* are supported...
