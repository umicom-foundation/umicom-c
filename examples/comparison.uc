/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/comparison.uc
 *
 * PURPOSE:
 *   Demonstrate arithmetic-before-comparison precedence with an eight-instruction, zero-or-one result.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

int main(void) { return 2 + 3 < 2 * 3; }
