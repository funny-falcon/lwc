
	class A {
		volatile virtual;
		int i;
	virtual	int f ()	{ return 1; }
	};

	class B : A {
		int f ()	{ return 2; }
	};

	int main ()
	{
		A *a = new A;
		B *b = new B;

		void *p = a->_v_p_t_r_;

		a->f ();	// returns 1

		b->_v_p_t_r_ = p;

		a->f ();	// returns 2!
	}
