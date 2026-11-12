![Umicom](assets/branding/umicom-logo.svg)

# Identify the configured compiler inputs

A successful configure step creates two data files in the standalone build directory:

- qualification/configuration-recipe.json records the selected CMake values.
- qualification/configuration-inputs.json records their bytes and the declared source, generated configuration and tool files.

This is a configuration observation. It does not prove that an executable was rebuilt, that tests passed, or that inputs stayed unchanged throughout compilation. The record explicitly keeps compilationVerified and sourceStableDuringBuildVerified false. The bounded runtime remains independent: including cmake/UmicomCRuntime.cmake in another project does not enable this hosted configuration adapter.

## Configure, build and test

From your Umicom C checkout, use the normal documented preset:

~~~powershell
cmake --preset windows-ucrt64-debug
cmake --build --preset windows-ucrt64-debug
ctest --preset windows-ucrt64-debug --output-on-failure --no-tests=error
~~~

Run each command separately and stop on failure. Use native-debug on a suitable Linux host. Keep compiler changes in a separate build directory. Configuration refreshes the observation; it does not replace building and testing.

## Check without refreshing

The most direct check uses CTest without reconfiguring:

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R '^umicom.c.inputs-current$' --output-on-failure --no-tests=error
~~~

Expect one test to pass when every declared input still matches. The verifier reads data and never writes the receipt, launches the recorded tools, or evaluates its contents as CMake code. It compares the canonical receipt bytes, so reformatting or appending text is also a refusal. Keep this file as generated evidence, not as hand-maintained configuration.

For a controlled exercise, first copy the current record outside the build directory. Add a harmless comment to an example in your own experimental checkout, then repeat the check. Expect failure. Review the source change, reconfigure, rebuild and run the full tests. Preserve the earlier record when comparing the two observations. Do not edit a checksum to force a pass.

The optional umicom-c-check-inputs build target calls the same verifier, but a build tool may automatically regenerate CMake configuration before running a target. Use the CTest command above when specifically investigating whether the existing record is stale.

## What is observed

The inventory includes root CMake files, and files under src, include, tools, cmake, tests, examples and docs/assets/branding. It detects changed contents, added files, deleted files and renamed files. It hashes available configured package files and the Windows resource script. Available CMake, compiler driver, build tool, linker, archiver, indexer, resource compiler and toolchain-file paths are hashed without executing them. An absent optional tool is recorded as null.

The recipe contains selected compiler flags, generator, platform, host and target values. These are resolved values from configuration, not a live reinterpretation of CMakeCache.txt. Ordinary documentation prose, compiler-loaded libraries, implicit SDK dependencies, environment effects, arbitrary launcher behavior and final binaries are outside this observation. The unknownInputs field records these limits. A matching compiler driver hash does not establish that its dynamically loaded dependencies match.

Receipts and recipe files are limited to 16 MiB; the declared source inventory is limited to 4096 files. Linked input paths and control characters in serialized values are refused. The check is an observation of an ordinary local workspace, not protection from privileged modification or concurrent file replacement. Paths and flags can reveal local information: review records before sharing them.

## Exercise the refusal cases

~~~powershell
ctest --test-dir build/windows-ucrt64-debug -R '^umicom.c.input-identity.' --output-on-failure --no-tests=error
~~~

The synthetic fixtures change copies rather than real compiler files. They cover literal placeholder text, malformed and missing records, tool changes, generated inputs, and changes deliberately outside the inventory. Outside-scope changes passing a check demonstrate the limit; they are not claims of binary provenance or reproducibility.

Read [capability profiles](capability-profiles.md) for a different question: what the linked component declares it supports. Framework owns the wider native test-receipt and test-reuse services. This CMake adapter does not select cached passing tests or duplicate those product services.

The implementation uses CMake's existing [file hashing](https://cmake.org/cmake/help/v3.24/command/file.html) and [JSON operations](https://cmake.org/cmake/help/v3.24/command/string.html). No new hashing engine or higher-level product runtime is introduced.
