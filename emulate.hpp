#pragma once
#include <ntifs.h>

using BYTE = UINT8;
using PBYTE = BYTE*;

#define Print( ... )                                                                                                   \
    {                                                                                                                  \
        DbgPrintEx( 0, 0, __VA_ARGS__ );                                                                               \
    }
	
typedef struct TpEmuData
{
    EXCEPTION_RECORD *Record;
    PCONTEXT Context;
    ULONG64 PreviousMode;
};

// Incomplete structure with no intention to complete. 
// We won't be using the structure much in any case but it is nice to visualize some fields...

typedef struct TpEmuInstr
{
    BYTE a1;
    BYTE OperandSize;
    BYTE MovSize;
    BYTE a4;
    BYTE a5;
    BYTE BufferLength;
    BYTE InstructionBuffer[ 15 ];
    BYTE a8;
    BYTE a9;
    BYTE ImmediateOperandSize;
    BYTE InstrSize;
    BYTE RexByte;
    BYTE Modrm;
    BYTE a14;
    BYTE a15;
    BYTE a16;
    BYTE a17;
    BYTE a18;
    ULONG64 ImmediateOperand;
    BYTE a20;
    BYTE a21;
    BYTE a22;
    BYTE a23;
    BYTE a24;
    BYTE a25;
    BYTE a26;
};


// Tp functions... some of which can be used to further emulate unsupported instructions (ex. INC, SUB, etc)
using _KiTpEmulateInstruction = NTSTATUS ( * )( _Inout_ TpEmuInstr *EmuInstr, _Inout_ TpEmuData *EmuCtx );
using _KiTpParseInstructionPrefix = NTSTATUS ( * )( _In_ TpEmuInstr *InInstr );
using _KiTpReadImageData = NTSTATUS ( * )( OPTIONAL PEPROCESS Process, BOOLEAN ShouldValidate, _In_ PVOID Source,
                                           _Out_ PVOID Buffer, _In_ ULONG Size );
using _KiTpSetFlagsZeroSignParity = VOID(*)(TpEmuInstr* Instr, TpEmuData* EmuCtx, ULONG64 AndRes);
using _KiTpAccessMemory = VOID(*)(TpEmuInstr* Instr, ULONG64 Source, ULONG64 Buffer, CHAR PrevMode, CHAR Param5, BYTE Width, BYTE Param7);
using _KiTpSetFlagsSub = VOID(*)(TpEmuInstr* Instr, TpEmuData* Cpu, ULONG64 SubResult, ULONG64 Original, ULONG64 SubBy);
using _KiTpSetFlagsAdd = VOID ( * )( TpEmuInstr *Instr, TpEmuData *Cpu, ULONG64 AddResult, ULONG64 Original,
                                     ULONG64 AddBy );
			
// Required functions to emulate the base-line set of instructions... to be set to the correct symbol within ntoskrnl.			
_KiTpEmulateInstruction KiTpEmulateInstruction = nullptr;
_KiTpParseInstructionPrefix KiTpParseInstructionPrefix = nullptr;
_KiTpReadImageData KiTpReadImageData = nullptr;

//
// Instr: The address of the instruction to be emulated...
// Context: An input buffer to the starting context that will contain the updated context upon return
// Record: An input exception record parameter that is populated if an error occurs during emulation (ex. access violations) 
//
TpEmuInstr EmulateInstr( PVOID Instr, PCONTEXT Context, PEXCEPTION_RECORD Record )
{
    if ( !KiTpReadImageData || !KiTpParseInstructionPrefix || !KiTpEmulateInstruction )
    {
        Print( "EmuInstr: NT API was not initialized correctly. Halting...\n" );
        return {};
    }

    if ( !Instr || !MmIsAddressValid( Instr ) )
    {
        return {};
    }

    BOOLEAN UseContext = TRUE;
    if ( !Record || !Context )
    {
        UseContext = FALSE;
    }

    // Only use Record/Context as a parameter if it is valid to avoid unintentional crashes...
    TpEmuInstr InstrData{ 0 };
    TpEmuData Data{ Record, Context, KernelMode };
    TpEmuData *EmuData = ( UseContext == TRUE ) ? &Data : nullptr;

    SIZE_T CopySize = min( PAGE_SIZE - ( ( ULONG64 )Instr & 0xFFF ), sizeof( InstrData.InstructionBuffer ) );

    auto Result = KiTpReadImageData( nullptr, FALSE, Instr, &InstrData.InstructionBuffer[ 0 ], CopySize );
    if ( !NT_SUCCESS( Result ) )
    {
        // This should never happen... you will just BSOD :P
        // Edit: Nvm you won't...SEH is wrapped internally so this function will always work... maybe....
        return {};
    }

    InstrData.BufferLength = CopySize;
    Result = KiTpParseInstructionPrefix( &InstrData );

    if ( !NT_SUCCESS( Result ) )
    {
        if ( Result == STATUS_NOT_SUPPORTED )
        {
            Print( "EmuInstr: Failed to parse instruction @%p, unsupported instruction?\n", Instr );
            return InstrData;
        }
        else
        {
            Print( "KiTpParseInstructionPrefix result: 0x%02x\n", Result );
            Print( "EmuInstr: Failed to parse instruction @%p...\n", Instr );
            return InstrData;
        }

        return {};
    }

    //
    // First call is to collect InstrData (just how this function was designed...)
    // Second call performs the same emulation and saves the context of the CPU...
    //

    Result = KiTpEmulateInstruction( &InstrData, nullptr );
    Result |= KiTpEmulateInstruction( &InstrData, EmuData );

    if ( !NT_SUCCESS( Result ) )
    {
        //
        // Opcode not supported (JCCs, SETcc, MOVcc, MOVSX, IMUL/IDIV, INC, DEC)
        //

        if ( Result == STATUS_NOT_SUPPORTED )
        {
            Print( "EmuInstr: Failed to emulate unsupported instruction instruction @%p\n", Instr );
            return InstrData;
        }
        else
        {
            Print( "KiTpEmulateInstruction result: 0x%02x\n", Result );
            Print( "EmuInstr: Failed to emulate instruction @%p...\n", Instr );
            return InstrData;
        }
    }

    Print( "Instruction Size: 0x%02x\n", InstrData.InstrSize );
    return InstrData;
}
