# LLVM MPI-Checker

This repository contains new added features regarding MPI-IO.
This project is still in progress.

## Build LLVM
- [Setup](https://llvm.org/docs/GettingStarted.html#id18)

## Clang-Tidy Checks
- Verify if the buffer type and MPI_Datatype matches
- Wrong referenced buffers are detected
- run `clang-tidy -checks='*,mpi-type-mismatch,mpi-buffer-deref' file.c`

## Clang Static Analyzer
- detect MPI_File_close called twice for a file handle 
- `scan-build -enable-checker optin.mpi.MPI-Checker --use-cc=clang make`
