uniform sampler2D tex;

void main()
{
	vec2 coords = gl_TexCoord[0].xy;
	coords.y = 1.0 - coords.y;

	vec4 col = texture2D(tex, coords);
	col *= 3.0;

	gl_FragColor = col;
}
