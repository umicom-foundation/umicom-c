#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_profile.cmake
#
# PURPOSE:
#   Check read-only component profile output and reject extra CLI arguments.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

if(NOT DEFINED TOOL)
    message(FATAL_ERROR "TOOL is required")
endif()
execute_process(COMMAND "${TOOL}" profile RESULT_VARIABLE code
    OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
string(REPLACE "\r\n" "\n" output "${output}")

# The former compiler mask was 7 (arithmetic, comparison, logical not).
# Adding the independent spelling bits 32 and 64 makes 103; runtime stays 27.
# Preserve the previous exact profile while checking the additive slot keys.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=103\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=27\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=6\n")
endif()
# Forward branches and source logical operators add independent capability bits.
# Preserve the previous exact profile while checking the new canonical format.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=231\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=155\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=14\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Conditional expressions add source bit 1024 without changing runtime or wire masks.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=743\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the pre-compound compiler report; runtime and wire masks stay fixed.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=1767\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the pre-conditional-statement profile for review. The new compiler
# bit advertises source lowering; runtime and wire capabilities are unchanged.
if(FALSE)
    set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=3815\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Else-if adds compiler bit 8192; retain the former exact report for review.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=7911\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the prior exact capability report; remainder adds shared bit 16384 and wire format 5.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=16103\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the prior report; complement adds compiler-only bit 32768.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=32487\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=16795\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=62\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the format-5 complement capability response for review.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=65255\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=16795\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=62\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the earlier source-only mask; runtime profile and wire versions stay unchanged.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=130791\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the previous source mask; digit separators add only compiler bit 262144.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=261863\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the prior source report; const qualification adds only compiler bit 524288.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=524007\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Retain the prior mask: lexical branch scope adds compiler bit 1048576 only.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=1048295\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the previous source feature mask; standalone scopes add bit 2097152.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=2096871\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the prior source mask; optional warning reports add a compiler-only bit.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=4194023\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Preserve the format-6 profile; current linked features add checked shifts.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=8388327\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=82331\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=126\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Empty statements add one compiler-only bit; retain the previous output contract.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=16776935\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=8470939\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=254\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# ASCII character grammar adds a source-only bit; preserve the previous output.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=33554151\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=8470939\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=254\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Declaration lists add a source-only bit; retain the prior exact CLI contract.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=67108583\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=8470939\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=254\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
# Boolean literals add a compiler-only spelling bit; preserve the previous report.
if(FALSE)
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=134217447\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=8470939\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=254\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
endif()
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=268435175\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=8470939\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=254\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
if(NOT "${code}" STREQUAL "0" OR NOT output STREQUAL expected OR NOT error STREQUAL "")
    message(FATAL_ERROR "Profile output contract failed: ${code}\n${output}\n${error}")
endif()
execute_process(COMMAND "${TOOL}" profile must-not-open-this-input.uc
    RESULT_VARIABLE code OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
if(NOT "${code}" STREQUAL "2" OR NOT output STREQUAL "" OR
    NOT error MATCHES "Usage:" OR error MATCHES "Cannot open input")
    message(FATAL_ERROR "Extra profile arguments were not rejected before file access: ${code} ${output} ${error}")
endif()
