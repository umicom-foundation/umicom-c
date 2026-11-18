/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/empty_statements.uc
 *
 * PURPOSE:
 *   Demonstrate source-only empty statements while retaining four arithmetic VM instructions.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


int main(void) {
    ;
    { ;; }
    ;
    return 6 * 7;
    ;;
}
