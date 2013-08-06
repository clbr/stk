uniform sampler2D tex;
uniform vec2 pixel;
uniform int size;

void main()
{
	float res = 0.0;
	vec2 tc = vec2(0.0);

	for (int i = 0; i < size; i++)
	{
		float col = texture2D(tex, tc).x;
		res = max(col, res);

		tc += pixel;
	}

	gl_FragColor = vec4(res);
}
