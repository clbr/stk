uniform sampler2D tex;
uniform int size;
uniform vec2 pixel;

void main()
{
	vec2 origtc = gl_TexCoord[0].xy;

	// Get total sum
	float first = 1.0, last = 0.0;
	float lower = 0.0;
	float total = 0.0;
	vec2 tc = pixel * 0.5;

	for (int i = 0; i < size; i++)
	{
		float col = texture2D(tex, tc).x;

		lower += col * step(tc.y, origtc.y);
		total += col;

		if (col > 0.0001)
		{
			first = min(first, tc.y);
			last = max(last, tc.y);
		}

		tc += pixel;
	}

	float res = (lower / total) - origtc.y;

	float r, g;
	r = abs(res * step(res, 0.0));
	g = res * step(0.0, res);

	// Outside the edges?
	if (origtc.y <= first)
	{
		r = origtc.y * 2.1;
		g = 0.0;
	}
	else if (origtc.y >= last)
	{
		r = 0.0;
		g = (1.0 - origtc.y) * 2.1;
	}

	gl_FragColor = vec4(r, g, 0.0, 1.0);
}
