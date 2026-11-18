/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/boolean_literals.uc
 *
 * PURPOSE:
 *   Select a safe integer result using exact Boolean literal spellings and existing short-circuit evaluation.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


int main(void) {
    const int enabled = true;
    int ready = false;
    if (enabled && !ready) {
        ready = 42;
    }
    return enabled ? ready : 1 / 0;
}
