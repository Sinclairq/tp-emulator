# tp-emulator
A basic 100-loc CPU emulator using the existing code of ntoskrnl.exe

## Usage
Initialize the defined function prototypes in the header file, then call EmuInstr( ) freely.

## Limitations
As mentioned in the header comments, this emulator cannot emulate some common set instructions such as JCCs, SETcc, MOVcc, MOVSX, IMUL/IDIV, INC, and DEC.
Operations like PUSH/POP/MOV/CMP/SUB/ADD/RET/INT3 are supported...
