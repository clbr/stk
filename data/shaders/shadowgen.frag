uniform sampler2D full;
uniform sampler2D halft;
uniform sampler2D quarter;
uniform sampler2D eighth;

void main()
{
	vec3 val[4];
	val[0] = texture2D(full, gl_TexCoord[0].xy).xyz;
	val[1] = texture2D(halft, gl_TexCoord[0].xy).xyz;
	val[2] = texture2D(quarter, gl_TexCoord[0].xy).xyz;
	val[3] = texture2D(eighth, gl_TexCoord[0].xy).xyz;

	// Find the first level with a penumbra value
	int i;
	float q = 0.0;
	for (i = 0; i < 4; i++)
	{
		if (val[i].z > 0.01)
		{
			q = 1.0 - val[i].y;
			break;
		}
	}

	float outval = 1.0;
	if (q > 0.01)
	{
		q *= 8.0;
		q = max(1.0, q);
		q = log2(q);
		q = min(2.9, q);

		// q is now between 0 and 2.9.
		int down = int(floor(q));
		int up = down + 1;
		float interp = q - float(down);

		outval = 1.0 - mix(val[up].x, val[down].x, interp);
	}

	gl_FragColor = vec4(vec3(outval), 1.0);
}
