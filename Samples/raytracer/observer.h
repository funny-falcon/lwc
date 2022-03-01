Vec DV1, DVx, DVy;

#define SCREENRATIO 0.75f
float VIEWANGLE = 2.53f;

static void normalizeDs ()
{
	float l1 = DV1.length ();
	float l2 = DVx.length ();
	float l3 = DVy.length ();

	DV1.fmul (1.0f / l1);
	DVx.fmul (VIEWANGLE / l2);
	DVy.fmul (VIEWANGLE / l3);
	//DVy.fmul (SCREENRATIO * VIEWANGLE / l3);
}

static void rotate_observer (float tan_phi)
{
	float l1 = tan_phi * sqrtf (DV1.sqr_length () / DVx.sqr_length ());

	DVx.fmul (l1);
	DV1.add (DVx);
	DVx.external_product (DVy, DV1);
	normalizeDs ();
}

static void move_fwd (float d)
{
	Vec tmp = DV1;
	tmp.fmul (d);
	Viewer.add (tmp);
}

static void move_up (float d)
{
	Vec tmp = DVy;
	tmp.fmul (d);
	Viewer.add (tmp);
}
