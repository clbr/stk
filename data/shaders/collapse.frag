uniform sampler2D tex;
uniform vec2 pixel;
uniform vec2 multi;
uniform int size;

void main()
{
	float res = 0.0;
	vec2 tc = gl_TexCoord[0].xy;
//	tc.y = 1.0 - tc.y;
	tc *= multi;

	for (int i = 0; i < size; i++)
	{
		float col = texture2D(tex, tc).x;
		res = max(col, res);

		tc += pixel;
	}

	gl_FragColor = vec4(res);
}
