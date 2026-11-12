/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/conditional_value.uc
 *
 * PURPOSE:
 *   Select a fallback integer without evaluating a division by zero.
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
    return divisor != 0 ? 42 / divisor : 7;
}
