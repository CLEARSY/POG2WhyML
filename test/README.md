# Tests for POG2WhyML

This directory contains tests for POG2WhyML, a software program that translates POG proof obligations to the WhyML format.
The tests are conducted using CMake and CTest (<https://cmake.org/cmake/help/book/mastering-cmake/chapter/Testing%20With%20CMake%20and%20CTest.html>).

A test consists of applying an POG2WhyML to a POG file, and compare the resulting WhyML file with a reference result.
POG file is produced from a B MACHINE (.mch) file in two steps :

- BXML generation, with the bxml executable, taking as input a B MACHINE file, and redirecting its output to a file with extension `.bxml`.
- POG generation, taking as input a `.bxml` file and producing a `.pog` file.

(see test/scripts/update_references.sh for details).

The result is the combination of:

- the produced WhyML files, if any,
- the text produced on the standard output channel
- the text produced on the standard error channel
- the exit code.

Reference results have been validated by applying tools and manual inspection :

- The application of [Why3](https://why3.org) is successful. This application is parameterized to produce output in TPTP format.
- The TPTP output file of this application is validated with dolmen.
- The produced TPTP output file is kept in the reference section, although it is not produced during the execution of the test suite.

You can also use the `test/script/check_reference.sh` script to run [dolmen](https://github.com/Gbury/dolmen) validator over TPTP reference outputs.
You will first have to set a path do `dolmen` binary.

The POG files used to test the encoders have been produced by the bxml and pog tools distributed with Atelier B (<https://atelierb.eu>) from B source files.
Note that the source code of the bxml tool is available at <https://github.com/CLEARSY/BCOMPILER>.

The source files corresponding to the POG files are also registered in this directory.

## Receipt to create a new test

Assuming the test originates from a B source file, e.g., `afeature.mch`, with corresponding POG file `afeature.pog`.

1. Create a new test identifier, e.g. test42 (consult file CMakeLists.txt or directory input/).
2. Create the input test directory, e.g., `input/test42`
3. Populate the input test directories with files `afeature.mch` and `afeature.pog` in that directory.
4. Create the output reference test directory, e.g., `output/test42/reference`.
5. Populate the output reference test directories with the following commands, e.g.

```bash
test=test42
comp=afeature

pog2why -A -i input/$test/$comp.pog -o output/$test/reference/$comp.why > output/$test/reference/stdout 2> output/$test/reference/stderr
echo $? > output/$test/reference/SMT/exitcode
why3 prove -L . -D input/$test/$comp.why > output/$test/reference/$comp.p
```

6. Validate the contents of the output reference test directories
7. Update file CMakeLists.txt in this directory to add the new test to the test suite.

## Get a coverage report

It is possible to generate a HTML code coverage report for `pog2why`:

```bash
cmake -B build-coverage -DCMAKE_BUILD_TYPE=Coverage .
cmake --build build-coverage
cmake --build build-coverage --target pog2why_coverage
```