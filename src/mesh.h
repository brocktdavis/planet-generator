#ifndef MESH_H
#define MESH_H

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#include <SimplexNoise.h>

class Region {
public:
	Region(std::vector<size_t> idx_data, std::vector<glm::vec3>* vertices);

	// Simulations
	void set_elevation(SimplexNoise sn);
	void update_affected(std::vector<int>& owned_regions, std::vector<float>& multipliers);

	// Populating mesh data
	void addMeshData(std::vector<glm::uvec2>& lines, std::vector<glm::uvec3>& faces);

private:
	std::vector<glm::vec3>* vertices;
	std::vector<size_t> indices;
	size_t center_idx;

	float elevation_multiplier;
};

class Mesh {
public:

	Mesh(unsigned num_points, unsigned noise_seed);

	// Generator points and convex hull data
	std::vector<glm::vec3> hull_points;
	std::vector<glm::uvec2> hull_indices;
	std::vector<glm::uvec3> hull_faces;

	// Voronoi mesh data
	std::vector<glm::vec3> vertices;
	std::vector<glm::uvec2> lines;
	std::vector<glm::uvec3> faces;

private:
	std::vector<Region *> regions;

	// Initialization functions
	void generate_vertices(unsigned num_points, int iterations);
	void make_regions(std::vector<std::vector<size_t>> groups);

	// Simulation functions
	void elevation_sim(unsigned noise_seed);

	// Populating mesh data
	void populate_mesh_data();
};
#endif