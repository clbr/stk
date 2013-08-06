uniform sampler2D tex;
uniform int size;

void main()
{
	vec2 origtc = gl_TexCoord[0].xy;
	origtc.y = 1.0 - origtc.y;

	// Get total sum
	float lower = 0.0;
	float total = 0.0;
	for (int i = 0; i < size; i++)
	{
		vec2 tc = vec2(float(i) / float(size - 1));

		float col = texture2D(tex, tc).x;

		lower += col * step(tc.y, origtc.y);
		total += col;
	}

	float res = (lower / total) - origtc.y;

	float r, g;
	r = abs(res * step(res, 0.0));
	g = res * step(0.0, res);

	gl_FragColor = vec4(r, g, 0.0, 1.0);
}
