#include <math.h>
#include <stdlib.h>

static inline float sqr (float x)
{
	return x*x;
}

int randup (int i)
{
	return (int) (((float) i) * rand ()/(RAND_MAX+1.0));
}

/* returns a uniform variable from 0 to 1 */
double random_uniform01()
{
	return (rand() & 0xfffffff) / (double) 0xfffffff;
}

double random_uniform11()
{
	return ((rand() & 0xfffffff) - 0x7ffffff) / (double) 0x7ffffff;
}

/* returns a mean 0 variance 1 random variable
   see numerical recipies p 217 */
double random_gaussian()
{
static	int iset = 0;
static	double gset;
	double fac, r, v1, v2;

	if (0 == iset) {
		do {
			v1 = random_uniform11();
			v2 = random_uniform11();
			r = v1 * v1 + v2 * v2;
		} while (r >= 1.0 || r == 0.0);
		fac = sqrt(-2.0 * log(r)/r);
		gset = v1 * fac;
		iset = 1;
		return v2 * fac;
	}
	iset = 0;
	return gset;
}

void normalize (float r[], int n)
{
	float max = 0;
	int i;

	for (i = 0; i < n; i++)
		if (fabs (r [i]) > max) max = fabs (r [i]);
	for (i = 0; i < n; i++) r [i] /= max;
}

void t2_function (float r[], int n, int d, float slope, int timescale)
{
	int i, j, lf = n / d, N = n*timescale;
	float A, dA = (1.0 - slope) / (float) lf;
	float phi, f, fo = 2.0*M_PI/(float)n;
	float dt = 1.0 / (float) timescale;

	for (i = 0; i < N; i++) r [i] = 0.0;

	for (i = 1; i < lf; i++) {
		A = 1.0 - dA * i;
		phi = random_uniform11 () * M_PI;
		f = ((float)i) * fo * dt;
		for (j = 0; j < N; j++)
			r [j] += A * sin (f * j + phi);
	}

	normalize (r, N);
}

#if 0
void bell_patch (float r[], int co, int d, int n, float A)
{
	int i, n2 = n / 2;
	float f = 500.0 / (float) (d*d);

	for (i = 0; i < n; i++)
		r [(i+co)%n] += sqr (A / (1 + f * sqr (i-n2)));
}

void t2_function (float r[], int n, int d, float slope)
{
	int i;

	t_function (r, n, d, slope);
//	for (i = 0; i < 100; i++)
//		bell_patch (r, randup (n), 150 + randup (n/3), n,
//			    random_uniform11 ()*0.4);
//	normalize (r, n);
}
#endif
