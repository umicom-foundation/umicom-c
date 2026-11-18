/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/compound_statements.uc
 *
 * PURPOSE:
 *   Keep a temporary answer inside a standalone block and return the copied outer value.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


int main(void) {
    int result = 0;
    {
        const int answer = 42;
        result = answer;
    }
    return result;
}
