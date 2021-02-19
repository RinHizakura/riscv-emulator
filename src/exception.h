#ifndef RISCV_EXCEPTION
#define RISCV_EXCEPTION

/* the term 'exception' refer to an unusual condition occurring at run time
associated with an instruction in the current RISC-V hart */
typedef struct {
    enum {
        InstructionAddressMisaligned = 0,
        InstructionAccessFault = 1,
        IllegalInstruction = 2,
        Breakpoint = 3,
        LoadAddressMisaligned = 4,
        LoadAccessFault = 5,
        StoreAMOAddressMisaligned = 6,
        StoreAMOAccessFault = 7,
        EnvironmentCallFromUMode = 8,
        EnvironmentCallFromSMode = 9,
        EnvironmentCallFromMMode = 11,
        InstructionPageFault = 12,
        LoadPageFault = 13,
        StoreAMOPageFault = 15,
    } exception;
} riscv_exception;

/* the term 'trap' refer to the transfer of control to a trap handler caused by
 * either an exception or an interrupt */
typedef struct {
    enum {
        /* The trap is visible to, and handled by, software running inside the
         * execution environment */
        Contained,
        /* The trap is a synchronous exception that is an explicit call to the
         * execution environment requesting an action on behalf of software
         * inside the execution environment */
        Requested,
        /* The trap is handled transparently by the execution environment and
         * execution resumes normally after the trap is handled */
        Invisible,
        /* The trap represents a fatal failure and causes the execution
         * environment to terminate execution */
        Fatal,
    } trap;
} riscv_trap;

#endif
