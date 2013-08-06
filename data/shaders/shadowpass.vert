uniform sampler2D warpx;
uniform sampler2D warpy;

void main()
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
