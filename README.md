# Method Of Lines Transpose (MOLT) : Single and Threaded Implementation

This project aims to create a truly parallel implementation of MOLT, researched by Matthew Causley,
Andrew Christlieb, et al.

## Purpose

When I started this thesis, no one had yet taken the [Method of Lines Transpose
(MOLT)](https://arxiv.org/pdf/1306.6902.pdf) and implemented it in anything other than Matlab, or in
parallel. All of the mathematics had come together to show off that the algorithm devised in that
paper _should_ be embarrassingly parallel; however, it hadn't been proven.

That's what this project set out to do: implement the MOLT algorithm in parallel, and prove the
theory.

Among other things, the Kettering Undergraduate Thesis project is _supposed_ to show off what an
engineer can accomplish in a single 12 week period (the length of a single co-op term). I've taken
WAY more time than that, and I hope the features and thought that went into this shows, and is a
half decent demonstration of my skills as a software engineer.

## Setup

### Easy Setup

```
sudo apt install build-essential
git clone https://github.com/brimonk/molt.git
cd molt/
make
```

### Dependencies

If you plan to use the CPU only functionality, single-threaded or multi-threaded, you can get by
with just these:

* `gcc`
* `GNU Make`

If you need access to the CUDA build, you'll have to install the CUDA toolkit for your platform. You
can find more information [here](https://developer.nvidia.com/cuda-zone).

## Usage

## Implementation Details

When using / analyzing the main program (everything that starts with `main()`), you might see
something you don't quite recognize.

### Config File

Every run of the main, simulation program expects to have a handful of values dictated in a config
file. The included config file `default.cfg` includes some documentation; however, it's more clearly
laid out here.

#### Dimension Settings

Each simulation needs to know the dimensionality of the simulation it's running. This is defined
with _integral_ numbers, between 1 - 2^^32 - 1. The values that define problem dimensionality are:

* X (Spacial) Values
	* `x_start`
	* `x_stop`
	* `x_step`
* Y (Spacial) Values
	* `y_start`
	* `y_stop`
	* `y_step`
* Z (Spacial) Values
	* `z_start`
	* `z_stop`
	* `z_step`
* T (Temporal) Values
	* `z_sart`
	* `z_sop`
	* `z_sep`

So, to create a mesh 100x100x100, we would have config file entries as follows:

```
x_start:    0
x_stop :  100
x_step :    1

y_start:    0
y_stop :  100
y_step :    1

z_start:    0
z_stop :  100
z_step :    1
```

However, to be clear, this dimentionality definition doesn't have any bearing in the real world. To
scale this to the real world, we need to scale the integral dimentions to a float point scale. This
can be done with `scale_space`.

So, to make our 100x100x100 cube into a 1cmx1cmx1cm cube, we can enter the value:

```
scale_space: 1e-2
```

And that's it. Similarly, we can enter values for time (`t_*`), and `scale_time`, and get a similar
mapping.

#### Simulation Setup

To avoid modifying the core of the simulation program, a user can, optionally, write a custom
program that reads a spacial coordinate from stdin (X,Y,Z), and writes the corresponding charge on a
single line to stdout.

There are two commands that facilitate this: `initvel` and `initamp`. `initvel` initializes the
velocity of the wave in 3d, and `initamp` initializes the amplitude of the wave, also in 3d. The
'main' program (`molt`) achieves this with a bi-directional pipe, and threads to ensure the read and
write ends of the pipe are always being handled.

Writing a program that takes a single argument (`--vel`, and `--amp`) for velocity and amplitude can
be used in the following way:

```
# initial velocity command
initvel: experiments/test --vel

# initial amplitude command
initamp: experiments/test --amp
```

And at startup, the program will call those (other) programs with the command line given, and the
simulation will be initialized.

#### Core MOLT Implementation Library

While there are multiple implementations of the core MOLT algorithm in the source (single-threaded
CPU, multi-threaded CPU, multi-threaded GPU), only the single-threaded CPU implementation is
included in the `molt` binary. To use another implementation, you must declare that you are using it
in the config file. This is done with the `library` config value. Referencing the library is done
from the current working directory. This can be done with the following:

```
# Linux / Unix
library: ./moltthreaded.so

# Win32
library: moltcuda.dll
```

### Tensor Transposition

At this point you should know the data the program operates on is a 3d tensor. In the beginning, the
tensor's data is organized in X, Y, Z. However, to avoid numerical error leaning in a particular
dimension, we perform the MOLT on different spacial dimensions of the tensor.

The common operation then looks like this:

```c
f64 *data;

molt_sweep(data, 'X');
molt_sweep(data, 'Y');
molt_sweep(data, 'Z');

// ... etc
```

In Matlab, this sweeping is achieved by pulling single vectors of the matrix out one dimension at a
time, and operating on them. However, there is no way to communicate to Matlab how a Matlab script
intends to use a tensor. It can only be assumed that fetching a specific vector for all Xs for a
certain Y / Z for instance, is achieved by pulling specific values from a tensor where the Y and Z
condition is met.

In Matlab:

```
xVector = data(:,4,4);
```

However, because the implementation is supposed to be fast by design, we can make a pretty big
optimization.

Instead of copying values from a larger tensor into a working array, and stuffing them back in, we
can reorganize the data to optimize for the access pattern.

Assume I have a 3x3 Matrix. In an array, we'll use the index to seed the matrix value like so:

```c
f32 *data = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
```

We can access the data in this "one dimensional" array using this:

```c
#define IDX2D(x, y, ylen)          ((x) + (y) * (ylen))
```

To give us the ability to treat the 1d array as a 2d array, or a matrix.

Ergo:

```c
f32 first = data[0];
f32 alsoFirst = data[IDX2D(0, 0, 3)];

// IDX2D expands to ((0) + (0) * (3)) => 0

f32 middle = data[4];
f32 alsoMiddle = data[IDX2D(1, 1, 3)];

// IDX2D expands to ((1) + (1) * (3)) => 4
```

Keeping our access patterns tied to the known dimensionality of the "matrix", we can, with a careful
access pattern, get a logical matrix:

```
0 1 2
3 4 5
6 7 8
```

The matrix above assumes an X major ordering. Assume that we instead want to operate on Y ordered
data. By transposing the matrix above into this:

```
0 3 6
1 4 7
2 5 8
```

The in-memory array becomes:

```c
f32 *data = { 0, 3, 6, 1, 4, 7, 2, 5, 8 };
```

The MOLT algorithm actually requires vectors for all X, Y, and Z at different times. Using the
defined macros `IDX2D` (above) and `IDX3D`:

```c
#define IDX3D(x, y, z, ylen, zlen) ((x) + (y) * (ylen) + (z) * (zlen) * (ylen))
```

We can operate on all of the Xs, as an example:

```c
int y, z;

for (y = 0; y < ylen; y++) {
	for (z = 0; z < zlen; z++) {
		int idx = IDX3D(0, y, z, ylen, zlen);
		f32 *ptr = data + idx;
		// ptr now points to element 0 of your X ordered matrix
	}
}
```

### File Format

