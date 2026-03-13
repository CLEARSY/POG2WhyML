# POG2WhyML

[![License](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)

`POG2WhyML` is a translator from pog to WhyML, the language of the [Why3](https://why3.org) platform for deductive program verification.

## Getting the sources

`POG2WhyML` uses submodules to link other software components. Do not forget to use the adequate git options, e.g., 

```sh
git clone --recurse-submodules <url>
```

## Compilation

To compile the project with CMake:

```sh
cmake -B build
cmake --build build
```

## Contributing

We welcome external contributors to `POG2WhyML`!

Please carefully read the CONTRIBUTING.md file in this repository in case you consider contributing.

## Licensing

This code is distributed under the license: "GNU GENERAL PUBLIC LICENSE, Version 3".
See the LICENSE file in this repository.

## Acknowledgments

This project has been developed and is maintained by [CLEARSY](https://www.clearsy.com/). It has been partly financed by the [Agence Nationale de la Recherche](https://anr.fr) with

- grant ANR-21-CE25-0015 for the
[ICSPA](https://anr.fr/Project-ANR-21-CE25-0015) project (Enhancing B Language Reasoners with SAT and SMT Techniques).
- grant ANR-12-INSE-0010 for the [BWare](https://anr.fr/Project-ANR-12-INSE-0010) project (A Proof-Based Mechanized Plate-Forme for the Verification of B Proof Obligations).
