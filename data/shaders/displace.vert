varying float camdist;

void main() {
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	camdist = length((gl_ModelViewMatrix * gl_Vertex).xyz);
}
