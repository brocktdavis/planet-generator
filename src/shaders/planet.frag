R"zzz(
#version 330 core

uniform vec3 eye_location;
uniform float ocean_height;
uniform float max_elevation;

uniform vec3 ocean_color;
uniform vec3 snow_color;
uniform vec3 coast_color;
uniform vec3 vegetation_color;

in float elevation;
in float lattitude;

out vec4 fragment_color;

vec3 get_snow_color(float normalized_elevation) {
	// Base it on extremes of elevation or lattitude
	float snow_const = pow(normalized_elevation, 14.0) * 175.0 + pow(lattitude, 16.0);
	snow_const = clamp(snow_const, 0.0, 1.0);
	return snow_color * snow_const;
}

vec3 get_coast_vegetation_color(float normalized_elevation) {
	float coast_t = 1 / (1 + pow(10e15, -normalized_elevation + 0.02));
	return mix(coast_color, vegetation_color, coast_t);
}

void main() {

	vec3 color = vec3(0.0, 0.0, 0.0);
	// If it's in the ocean
	if (elevation < 0) {	
		color += ocean_color;
		color += get_snow_color(0.0) / 5.0;
		color = clamp(color, 0.0, 1.0);

	// Part of the terrain, get color based on that
	} else {
		// Calculate elevation as percentage of max possible elevation
		float normalized_elevation = elevation / max_elevation;
		// Get gradient of three colors based on lattitude and elevation
		color += get_snow_color(normalized_elevation);
		color += get_coast_vegetation_color(normalized_elevation);
		// Clamp color between 0 and 1
		color = clamp(color, 0.0, 1.0);
	}
	fragment_color = vec4(color, 1.0);
}
)zzz"
