uniform sampler2D tex1;
uniform sampler2D tex2;

void main()
{
	vec4 col1 = texture2D(tex1, gl_TexCoord[0].xy);
	vec4 col2 = texture2D(tex2, gl_TexCoord[0].xy);

	gl_FragColor = col1 * col2;
}
