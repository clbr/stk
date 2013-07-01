varying vec3 nor;
uniform mat4 invtworldm;

void main() {

	nor = (invtworldm * vec4(gl_Normal, 0.0)).xyz;
	nor = normalize(nor);
	nor = nor * 0.5 + 0.5;

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();

}
