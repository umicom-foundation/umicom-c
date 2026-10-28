/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/checked_failure.uc
 *
 * PURPOSE:
 *   Show a runtime arithmetic rejection without undefined signed overflow.
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
    return 2147483647 + 1;
}
