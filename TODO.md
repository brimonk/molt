# To Do

This is simply a list of things that need to get done on the project before it
is completed.

These are the big math chunks that need to still get done, in to be completed
order.

1. Rewrite GF_Quad (Green's Function Quadriture)
2. Clean up Khari's particle integration work
3. Domain Decomposition
4. Particle Integration


### This is an expansion of those things
#### Khari's Work Clean Up

* Clean up file input and output routines
* Include a small allocator, to avoid static, global buffers of memory (helps
		avoid issues when parallelizing)
* Initial parameters from command line arguments or stdin
* Use the following, instead of a collection of dangling floats

```
struct velocity_t {
	double x, y, z;
};

struct position_t {
	double x, y, z;
};

struct particle_t {
	/* other particle properties */
	struct velocity_t vel;
	struct position_t pos;
};
```

#### Rewrite GF\_Quad\_m

* Find a linear algebra library to perform initial work
* Find or write routines to perform similar Matlab functions as zeroes or a:dx:b

#### General Program Work

This is where all of the decidedly computer science only problems are detailed.

* Input/Output subsystem
	* Answer the question:
		* generic io subsystem?
		* simple io subsystem
* Simple, efficient, and thread safe allocator
	* Ensures full PCI bus saturation with data copying from device to host,
	* Independent threads run the jobs, and write output data to storage/stdout
* Initial Arguments as Command Line Parameters
	* Initial and Final Time
	* Time Step
	* Number of Particles
		* Initial Particle Position

