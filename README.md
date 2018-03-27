# Implementation of Method of Lines Transpose (MOLT)

This project aims to create a proper implementation of MOLT, researched by
Matthew Causley, Andrew Christlieb, and others.

## Program Structure

Because this task is simply so huge, I've decided not only to divide my labor
into multiple pieces, but to also divide the program's labor into multiple
portions. Ideally, initial conditions are fed into the actual simulation
program, and the state at various time intervals are simply recorded for
later use.

This allows for the project to operate in multiple steps, and for various
simulation viewers to exist for various purposes, while not modifying the
core simulation code at all.

### Still To Do

1. Port the Matlab wave solver into C (C89)
2. Extend portions of that serial C program to be parallelized in CUDA.
3. Port Matlab code for a Particle-in-cell (PIC) solver, implementing
   in both C and CUDA.

4. Bolt the two together. Analyze for correctness.
5. Write simulation viewer (either full 3d program, or a series of images
   stitched together.

#### Considerations

1. GPU/CPU optimizations

	* Translate arrays of structures into structures of arrays for increased memory throughput
