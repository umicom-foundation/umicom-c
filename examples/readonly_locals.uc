/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/readonly_locals.uc
 *
 * PURPOSE:
 *   Capture fixed local inputs while calculating a mutable result through the existing integer VM.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


int main(void) {
    const int unit = 6;
    int const count = 7;
    int total = unit * count;
    return total;
}
