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

/* matprint : function to print out an NxN matrix */
void matprint(double *mat, int n);

/* matinv : given an n x n matrix, perform Gauss-Jordan elimination, in place */
int matinv(double *mat, int n);
