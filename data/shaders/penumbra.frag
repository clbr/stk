#version 120

uniform sampler2D tex;
uniform vec2 pixel;

void main()
{
	float sum = 0.0;
	float num = 0.0;
	float val = 0.0;

	float movesX[] = float[](pixel.x * -3.5,
				pixel.x * -1.5,
				pixel.x * 0.0,
				pixel.x * 1.5,
				pixel.x * 3.5);

	float movesY[] = float[](pixel.y * -3.5,
				pixel.y * -1.5,
				pixel.y * 0.0,
				pixel.y * 1.5,
				pixel.y * 3.5);

	float weights[] = float[](
					0.0030, 0.0133, 0.0219, 0.0133, 0.0030,
					0.0133, 0.0596, 0.0983, 0.0596, 0.0133,
					0.0219, 0.0983, 0.1621, 0.0983, 0.0219,
					0.0133, 0.0596, 0.0983, 0.0596, 0.0133,
					0.0030, 0.0133, 0.0219, 0.0133, 0.0030
				);

	// Flood-fill
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			vec2 tc = gl_TexCoord[0].xy + vec2(movesX[i], movesY[j]);
			vec3 col = texture2D(tex, tc).xyz;

			if (col.z > 0.8)
			{
				sum += col.y;
				num++;
			}

			val += col.x * weights[i*5 + j];
		}
	}

	if (num < 0.5)
		discard;

	gl_FragColor = vec4(val, sum / num, 1.0, 1.0);
}
