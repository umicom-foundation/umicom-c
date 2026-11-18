/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/branch_locals.uc
 *
 * PURPOSE:
 *   Demonstrate one branch-local constant whose value is copied to an outer result.
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
    int result = 0;
    if (1) {
        const int answer = 42;
        result = answer;
    }
    return result;
}
