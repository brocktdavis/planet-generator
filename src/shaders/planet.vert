R"zzz(
#version 330 core

// Default shader, no projection on to sphere

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float ocean_height;

in vec3 vertex_position;

out float elevation;
out float lattitude;

void main() {
	// Get elevation and lattitude
	elevation = length(vertex_position) - ocean_height;
	lattitude = abs(vertex_position.y); // on unit sphere, just  make it abs(y)

	mat4 mvp = projection * view * model;
	
	vec3 pos;
	if (elevation < 0) {
		pos = ocean_height * normalize(vertex_position);
	} else {
		pos = vertex_position;
	}

	gl_Position = mvp * vec4(pos, 1.0f);
}
)zzz"
