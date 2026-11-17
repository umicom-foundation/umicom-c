/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tools/debugger.c
 *
 * PURPOSE:
 *   Offer bounded interactive stepping and local inspection using the shared Umicom C machine.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "debugger.h"
#include "umicom_c/machine.h"
#include <inttypes.h>
#include <string.h>

/* Read a whole bounded command. Embedded NUL and overlong lines are refused
 * after draining the line; they cannot silently turn into a valid prefix. */
static int command_line(FILE *input, char *text, size_t capacity)
{
    size_t used=0U; int rejected=0, c;
    while((c=fgetc(input))!=EOF && c!='\n') {
        if(c==0 || used+1U>=capacity) rejected=1;
        else text[used++]=(char)c;
    }
    if(ferror(input))return -2;
    if(c==EOF && used==0U && !rejected)return 0;
    if(used && text[used-1U]=='\r')--used;
    text[used]='\0';
    return rejected ? -1 : 1;
}
static size_t source_line(const char *source,size_t length,size_t offset,size_t *column)
{
    UmcDiagnostic d={.source_offset=offset};
    if(source)umc_locate_diagnostic(source,length,&d);
    *column=d.column; return d.line;
}
static int flush_output(FILE *output)
{
    return fflush(output)==0 && !ferror(output);
}
int umc_debug_console(const char *source,size_t length,const UmcInstruction *program,
    size_t count,size_t fuel,FILE *input,FILE *output,FILE *errors)
{
    UmcMachine machine; UmcDiagnostic diagnostic;
    if(!input || !output || !errors)return 2;
    if(umc_machine_init(&machine,program,count,fuel,&diagnostic)!=UMC_OK)return 1;
    fputs("Umicom C debugger: step, line, continue, locals, where, help, quit\n",output);
    for(;;) {
        fputs("umc> ",output); if(!flush_output(output))return 2;
        char command[64]; int read=command_line(input,command,sizeof command);
        if(read==0) {fputs("Cancelled; no completed result.\n",output);return flush_output(output)?0:2;}
        if(read==-2)return 2;
        if(read==-1) {fputs("Command refused: maximum 63 bytes, no NUL.\n",errors);continue;}
        if(!strcmp(command,"quit")) {fputs("Cancelled; no completed result.\n",output);return flush_output(output)?0:2;}
        if(!strcmp(command,"help")) {
            fputs("step: one VM instruction; line: until source line changes; continue: remaining budget\n"
                  "locals: initialized integer slots; where: next instruction; quit: cancel\n",output);continue;
        }
        if(!strcmp(command,"locals")) {
            for(unsigned i=0;i<UMC_LOCAL_LIMIT;++i)
                if(machine.frame.initialized & (UINT32_C(1)<<i))fprintf(output,"local[%u]=%" PRId32 "\n",i,machine.frame.locals[i]);
            continue;
        }
        size_t column=0, line=source_line(source,length,machine.program[machine.frame.pc].source_offset,&column);
        if(!strcmp(command,"where")) {
            fprintf(output,"next=%zu line=%zu column=%zu source-byte=%zu opcode=%s\n",machine.frame.pc,line,column,
                machine.program[machine.frame.pc].source_offset,umc_opcode_text(machine.program[machine.frame.pc].opcode));continue;
        }
        const int single=!strcmp(command,"step") || command[0]=='\0';
        const int next_line=!strcmp(command,"line");
        if(!single && !next_line && strcmp(command,"continue")) {fputs("Unknown command; use help.\n",errors);continue;}
        for(;;) {
            UmcTraceStep step;
            UmcStatus status=umc_machine_step(&machine,&step,&diagnostic);
            size_t step_line=source_line(source,length,step.source_offset,&column);
            fprintf(output,"step=%zu line=%zu column=%zu opcode=%s depth=%zu top=%" PRId32 " status=%s\n",
                step.instruction_index,step_line,column,umc_opcode_text(step.opcode),step.stack_depth,step.top,umc_status_text(status));
            if(status!=UMC_OK) {fprintf(errors,"Stopped at source-byte=%zu: %s\n",diagnostic.source_offset,diagnostic.message);
                return flush_output(output)?1:2;}
            if(machine.frame.finished) {
                UmcResult result;
                if(umc_machine_result(&machine,&result,&diagnostic)!=UMC_OK)return 1;
                fprintf(output,"Result: %" PRId32 "\nInstructions: %zu\n",result.value,result.instructions_executed);
                return flush_output(output)?0:2;
            }
            if(single)break;
            if(next_line && (!source || source_line(source,length,machine.program[machine.frame.pc].source_offset,&column)!=line))break;
        }
    }
}
