varying vec3 normal;
varying vec4 vertex_color;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	vertex_color = gl_Color;

	normal = normalize(gl_NormalMatrix*gl_Normal);
}
