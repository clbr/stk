uniform sampler2D tex;
uniform int size;

void main()
{
	vec2 origtc = gl_TexCoord[0].xy;

	// Get total sum
	float first = 1.0, last = 0.0;
	float lower = 0.0;
	float total = 0.0;
	for (int i = 0; i < size; i++)
	{
		vec2 tc = vec2(float(i) / float(size - 1));

		float col = texture2D(tex, tc).x;

		lower += col * step(tc.x, origtc.x);
		total += col;

		if (col > 0.0001)
		{
			first = min(first, tc.x);
			last = max(last, tc.x);
		}
	}

	float res = (lower / total) - origtc.x;

	float r, g;
	r = abs(res * step(res, 0.0));
	g = res * step(0.0, res);

	// Outside the edges?
	if (origtc.x <= first)
	{
		r = origtc.x * 2.1;
		g = 0.0;
	}
	else if (origtc.x >= last)
	{
		r = 0.0;
		g = (1.0 - origtc.x) * 2.1;
	}

	gl_FragColor = vec4(r, g, 0.0, 1.0);
}
