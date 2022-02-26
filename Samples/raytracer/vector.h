#ifdef HAVE_SSE
typedef int v4sf __attribute__ ((mode(V4SF)));
v4sf __builtin_ia32_addps (v4sf, v4sf);
v4sf __builtin_ia32_subps (v4sf, v4sf);
v4sf __builtin_ia32_mulps (v4sf, v4sf);
#endif

struct Vec {
	float x, y, z;
#ifdef	HAVE_SSE
	float sse_pad;
#endif

	ctor (float X, float Y, float Z) {
		x = X, y = Y, z = Z;
	}

	float length () {
		return sqrtf (x*x + y*y + z*z);
	}

	float sqr_length () {
		return x*x + y*y + z*z;
	}

	void external_product (Vec v1, Vec v2) {
		x = v1.y*v2.z - v1.z*v2.y;
		y = v1.z*v2.x - v1.x*v2.z;
		z = v1.x*v2.y - v1.y*v2.x;
	}

	void norm () {
		float m = 1.0f / sqrtf (x*x + y*y + z*z);
		x *= m, y *= m, z *= m;
	}

	void add (Vec v) {
#ifdef	HAVE_SSE
		(*(v4sf*)this) = __builtin_ia32_addps (*(v4sf*)this, *(v4sf*)&v);
#else
		x += v.x, y += v.y, z += v.z;
#endif
	}

	void sub (Vec v) {
#ifdef	HAVE_SSE
		(*(v4sf*)this) = __builtin_ia32_subps (*(v4sf*)this, *(v4sf*)&v);
#else
		x -= v.x, y -= v.y, z -= v.z;
#endif
	}

	void fmul (float f) {
		x *= f, y *= f, z *= f;
	}

	float inprod (Vec v) {
		return x*v.x + y*v.y + z*v.z;
	}

	void printf (const char*);
}
#ifdef HAVE_SSE
	__attribute__ ((aligned (16)));
#endif
;
