uniform sampler2D tex;

void main()
{
	vec4 col = texture2D(tex, gl_TexCoord[0].xy);
	col *= 3.0;

	gl_FragColor = col;
}
