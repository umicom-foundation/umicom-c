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
set(expected "profile.schema=1\ncompiler.api=1\ncompiler.features=1767\ncompiler.source-bytes=65536\ncompiler.instructions=256\ncompiler.parse-depth=64\nruntime.api=1\nruntime.features=411\nruntime.instructions=256\nruntime.stack-values=64\nruntime.bytecode-bytes=2312\nruntime.wire-versions=30\ncompiler.local-values=32\ncompiler.identifier-bytes=63\nruntime.local-values=32\n")
if(NOT "${code}" STREQUAL "0" OR NOT output STREQUAL expected OR NOT error STREQUAL "")
    message(FATAL_ERROR "Profile output contract failed: ${code}\n${output}\n${error}")
endif()
execute_process(COMMAND "${TOOL}" profile must-not-open-this-input.uc
    RESULT_VARIABLE code OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
if(NOT "${code}" STREQUAL "2" OR NOT output STREQUAL "" OR
    NOT error MATCHES "Usage:" OR error MATCHES "Cannot open input")
    message(FATAL_ERROR "Extra profile arguments were not rejected before file access: ${code} ${output} ${error}")
endif()
