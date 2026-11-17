/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/machine.h
 *
 * PURPOSE:
 *   Expose caller-owned, resumable bytecode execution through the shared checked VM.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_MACHINE_H
#define UMICOM_C_MACHINE_H
#include "trace.h"
#ifdef __cplusplus
extern "C" {
#endif
#define UMC_MACHINE_PROFILE 1U
/* One owner, no heap and no native pointers in guest values. Fields are readable
 * for a debugger but must only be changed through these functions. Initialize
 * before use; do not serialize this host structure or share it concurrently.
 * The program is copied at admission, so its caller may release the input.
 * A step is one VM instruction, not necessarily one source statement or line. */
/* Compact execution state is shared with synchronous execution so ordinary
 * runs keep their original small frame instead of copying a whole program. */
typedef struct UmcMachineFrame {
    int32_t stack[UMC_STACK_LIMIT];
    int32_t locals[UMC_LOCAL_LIMIT];
    size_t count, pc, depth, executed, fuel;
    uint32_t initialized;
    unsigned finished;
} UmcMachineFrame;
typedef struct UmcMachine {
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcMachineFrame frame;
} UmcMachine;
/* All supplied regions must be disjoint and valid for their capacities.
 * Initialization validates the complete program before changing machine.
 * Fuel is a lifetime instruction allowance, including RETURN. */
UmcStatus umc_machine_init(UmcMachine *machine, const UmcInstruction *program,
    size_t count, size_t fuel, UmcDiagnostic *diagnostic);
/* Execute one instruction. On a runtime refusal the whole machine is unchanged;
 * step (when present) describes that refused attempt. Invalid machine metadata
 * or calling after RETURN leaves step unchanged. A completed RETURN sets
 * frame.finished. Its value is then available in frame.stack[0]. Restart with init.
 * State admission catches bounds corruption, not arbitrary hostile host writes. */
UmcStatus umc_machine_step(UmcMachine *machine, UmcTraceStep *step,
    UmcDiagnostic *diagnostic);
/* Publish only a successfully returned result; otherwise preserve output. */
UmcStatus umc_machine_result(const UmcMachine *machine, UmcResult *result,
    UmcDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
