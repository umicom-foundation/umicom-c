/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/short_circuit.uc
 *
 * PURPOSE:
 *   Guard division with a logical condition whose false path skips the divisor.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

int main(void) {
    int divisor = 0;
    return divisor != 0 && 42 / divisor > 1;
}
