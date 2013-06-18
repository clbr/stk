uniform vec3 windDir;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec4 vertexPosition = ftransform();
	vertexPosition.xyz += windDir * gl_Color.r;

	gl_Position = vertexPosition;
}
