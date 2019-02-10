/*
 * Brian Chrzanowski
 * Tue Jan 22, 2019 21:51
 *
 * a collection of fancy Matlab operations that aren't trivial for computers
 *
 * Includes:
 *   matinv   - Matrix Inversion (Gauss-Jordan Elim)
 *   matprint - Matrix Printing (Debugging Only)
 */

#ifndef MOLT_CALCS
#define MOLT_CALCS

/* get_exp_weights : construct local weights for int up to order M */
void get_exp_weights(double *nu, double **wl, double **wr,
		int nulen, int orderm);

/* get_exp_ind : get indexes of X for get_exp_weights */
int get_exp_ind(int i, int n, int m);

/* matvander : function to write a vandermonde matrix in mat from vect */
void matvander(double *mat, double *vect, int n);

/* matprint : function to print out an NxN matrix */
void matprint(double *mat, int n);

/* invvan : create an inverted vandermonde matrix */
void invvan(double *mat, double *vect, int len);

/* polyget : find coeff of a polyynomial with roots in src. store in dst */
void polyget(double *dst, double *src, int dstlen, int srclen);

/* polydiv : use synthetic division to find the coefficients the poly */
void polydiv(double *dst, double *src, double scale, int dstlen, int srclen);

/* polyval : evaluate a polynomial at x = z */
double polyval(double *vect, double scale, int vectlen);

/* cumsum : perform a cumulative sum over elem along dimension dim */
void cumsum(double *elem, int len);

/* exp_coeff : find the exponential coefficients, given nu and M */
void exp_coeff(double *phi, int philen, double nu);

/* exp_int : perform an exponentially recursive integral (???) */
double exp_int(double nu, int sizem);

/* vm_mult : perform vector matrix multiplication */
void vm_mult(double *out, double *invect, double *inmat, int singledim);

#endif
