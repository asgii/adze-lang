# Adze

An LLVM front-end for compiling a toy language, currently only to LLVM IR.

One nice thing about the repository is that it illustrates an imperative language, as opposed, e.g., to the functional language in the LLVM tutorial.

## Building

You'll need LLVM.

To build:
```
git init
git clone https://github.com/asgii/adze-lang.git
make clang
```
or
```
make gcc
```

To run:
```
./adze examples/example.adze
```