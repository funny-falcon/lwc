extern Vec DV1, DVx, DVy;

static Vec preDVx [SCRX];

template raytrace (WORLD, SKY)
{{

	int i, j;
	scrdata_t *scr = screen;
	float dx = 1.0f/SCRX, dy = 1.0f/SCRY, x, y;

	for (x = -0.5f, i = 0; i < SCRX; i++, x += dx) {
		preDVx [i] = DVx;
		preDVx [i].fmul (x);
	}

	for (y = 0.5f, i = 0; i < SCRY; i++, y -= dy) {
		Vec tmp = DVy;
		tmp.fmul (y);
		tmp.add (DV1);
		for (j = 0; j < SCRX; j++) {
			Ray = tmp;
			Ray.add (preDVx [j]);
			Ray.norm ();
			*scr++ = WORLD.hits () ? WORLD.hitcolor () : SKY;
		}
	}
}}
