varying vec3 lpos;
varying vec3 lnorm;

void main() {
	gl_Position = ftransform();
	lpos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	lnorm = gl_NormalMatrix * gl_Normal;
}
