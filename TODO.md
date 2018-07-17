# To Do

This is simply a list of things that need to get done on the project before it
is completed.

These are the big math chunks that need to still get done, in to be completed
order.

1. Rewrite GF_Quad
	Green's Function Quadriture
2. Clean up Khari's particle integration work
3. Domain Decomposition
4. Particle Integration


#### This is an expansion of those things
Khari's Work Clean Up

* Clean up file input and output routines
* Create solid input routines
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

