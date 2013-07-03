uniform sampler2D tex;
uniform vec3 ambient;

void main()
{
	vec4 col = texture2D(tex, gl_TexCoord[0].xy);

	col.xyz += ambient;

	gl_FragColor = col;
}
