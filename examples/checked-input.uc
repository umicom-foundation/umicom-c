/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/checked-input.uc
 *
 * PURPOSE:
 *   Demonstrate an integer parameter supplied by a native host or browser.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

int umicom_run(int input)
{
    int total = input * 2;
    if (input > 0) {
        total += 2;
    }
    return total;
}
