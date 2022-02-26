
float fabsf (float);

#define lim_zero(f) (fabsf(f) < 0.0001f)

void gauss (float *a)
{
#define SWAPCOEFF(r1, r2) \
	{\
		float tmp [4];\
		__builtin_memcpy (tmp, a + r1, sizeof tmp);\
		__builtin_memcpy (a + r1, a + r2, sizeof tmp);\
		__builtin_memcpy (a + r2, tmp, sizeof tmp);\
	}

	float m;
	float s1, s2, s3;

	if (lim_zero (a [0]))
		if (lim_zero (a [4])) SWAPCOEFF (0, 8)
		else SWAPCOEFF (0, 4)
	else;
	if (lim_zero (a [5])) SWAPCOEFF (4, 8)

	m = a[4]/a[0];
	a [5] -= 1 [a] * m;
	a [6] -= 2 [a] * m;
	a [7] -= 3 [a] * m;
	m = a[8]/a[0];
	a [9] -= 1 [a] * m;
	a [10] -= 2 [a] * m;
	a [11] -= 3 [a] * m;
	m = a[9]/a[5];
	a [10] -= 6 [a] * m;
	a [11] -= 7 [a] * m;

	s1 = a [11] / a [10];
	s2 = (a [7] - s1 * a [6]) / a [5];
	s3 = (a [3] - s1 * a [2] - s2 * a [1]) / a [0];

	a [0] = s3;
	a [1] = s2;
	a [2] = s1;
}

int gauss_check (float *a)
{
	float m;
	float s1, s2, s3;

	if (lim_zero (a [0]))
		if (lim_zero (a [4])) SWAPCOEFF (0, 8)
		else if (!lim_zero (a [8])) SWAPCOEFF (0, 4)
		else return -1;
	else;
	if (lim_zero (a [5])) 
		if (!lim_zero (a [9])) SWAPCOEFF (4, 8)
		else return -1;
	else;
	if (lim_zero (a [10]))
		return -1;

	m = a[4]/a[0];
	a [5] -= 1 [a] * m;
	a [6] -= 2 [a] * m;
	a [7] -= 3 [a] * m;
	m = a[8]/a[0];
	a [9] -= 1 [a] * m;
	a [10] -= 2 [a] * m;
	a [11] -= 3 [a] * m;
	m = a[9]/a[5];
	a [10] -= 6 [a] * m;
	a [11] -= 7 [a] * m;

	s1 = a [11] / a [10];
	s2 = (a [7] - s1 * a [6]) / a [5];
	s3 = (a [3] - s1 * a [2] - s2 * a [1]) / a [0];

	a [0] = s3;
	a [1] = s2;
	a [2] = s1;

	return 0;
}
