/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/conditional_chain.uc
 *
 * PURPOSE:
 *   Select the first matching condition through checked forward branches.
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
    int value = 1;
    if (0) {
        value = 7;
    } else if (1) {
        value = 42;
    } else {
        value = 9;
    }
    return value;
}
