/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/source_warnings.uc
 *
 * PURPOSE:
 *   Show a never-read local separately from a source check and runtime execution.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


int main(void)
{
    int omitted = 6;
    int answer = 40;
    answer += 2;
    return answer;
}
