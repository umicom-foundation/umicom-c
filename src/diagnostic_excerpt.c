/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/diagnostic_excerpt.c
 *
 * PURPOSE:
 *   Render a caller-owned, bounded diagnostic excerpt after validating the complete borrowed source span.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/diagnostic_excerpt.h"

static size_t append_byte(char *text, size_t used, unsigned char byte)
{
    static const char hex[] = "0123456789ABCDEF";
    if (byte == (unsigned char)'\\') {
        text[used++] = '\\'; text[used++] = '\\';
    } else if (byte == (unsigned char)'\t') {
        text[used++] = '\\'; text[used++] = 't';
    } else if (byte >= 32U && byte <= 126U) {
        text[used++] = (char)byte;
    } else {
        text[used++] = '\\'; text[used++] = 'x';
        text[used++] = hex[byte >> 4U]; text[used++] = hex[byte & 15U];
    }
    return used;
}

UmcStatus umc_diagnostic_excerpt(const char *source, size_t length,
    size_t offset, UmcDiagnosticExcerpt *output)
{
    if (source == NULL || output == NULL || length > UMC_SOURCE_LIMIT || offset > length)
        return UMC_INVALID_ARGUMENT;

    /* Compute with a local candidate so refused requests never publish partial
     * text or metadata. Fixed source/window bounds also bound every append. */
    UmcDiagnosticExcerpt candidate = {0};
    UmcDiagnostic location = {0};
    location.source_offset = offset;
    umc_locate_diagnostic(source, length, &location);
    candidate.line = location.line;
    candidate.column = location.column;
    size_t line_begin = 0U;
    for (size_t i = 0U; i < offset; ++i) {
        if (source[i] == '\r') {
            line_begin = i + 1U;
            if (line_begin < length && source[line_begin] == '\n') {
                ++line_begin;
                if (i + 1U < offset) ++i;
            }
        } else if (source[i] == '\n') {
            line_begin = i + 1U;
        }
    }
    size_t line_end = line_begin;
    while (line_end < length && source[line_end] != '\r' && source[line_end] != '\n')
        ++line_end;
    const size_t focus = offset < line_begin ? line_begin : offset;
    size_t begin = line_begin;
    if (focus - begin > UMC_EXCERPT_SOURCE_BYTES / 2U)
        begin = focus - UMC_EXCERPT_SOURCE_BYTES / 2U;
    size_t end = line_end;
    if (end - begin > UMC_EXCERPT_SOURCE_BYTES) end = begin + UMC_EXCERPT_SOURCE_BYTES;
    /* Near the right edge, spend remaining capacity on useful preceding bytes. */
    if (end - begin < UMC_EXCERPT_SOURCE_BYTES && begin > line_begin) {
        const size_t available = UMC_EXCERPT_SOURCE_BYTES - (end - begin);
        begin -= available < begin - line_begin ? available : begin - line_begin;
    }
    candidate.source_begin = begin;
    candidate.source_end = end;
    candidate.clipped_left = begin > line_begin;
    candidate.clipped_right = end < line_end;
    size_t used = 0U;
    if (candidate.clipped_left) {
        candidate.text[used++] = '.'; candidate.text[used++] = '.'; candidate.text[used++] = '.';
    }
    for (size_t i = begin; i < end; ++i) {
        if (i == focus) candidate.caret_column = used;
        used = append_byte(candidate.text, used, (unsigned char)source[i]);
    }
    if (focus == end) candidate.caret_column = used;
    if (candidate.clipped_right) {
        candidate.text[used++] = '.'; candidate.text[used++] = '.'; candidate.text[used++] = '.';
    }
    candidate.text[used] = '\0';
    *output = candidate;
    return UMC_OK;
}
