#include "mesh.h"
#include "voronoi.h"
#include "config.h"

#include <iostream>
#include <algorithm>

#include <glm/gtc/random.hpp>

Mesh::Mesh(unsigned num_points, unsigned noise_seed)
{
	std::cout << "Generating " << num_points << " vertices." << std::endl;
	generate_vertices(num_points, 1);


	std::cout << "Generating voronoi regions." << std::endl;
	// Generate num_points random points on the surface of the unit sphere
	Voronoi voronoi(hull_points);

	// Get convex hull indices/faces
	voronoi.getHullIndices(hull_indices);
	voronoi.getHullFaces(hull_faces);

	// Get vertice and groups data from voronoi
	std::vector<std::vector<size_t>> voronoi_groups;
	voronoi.getVerticesGroups(vertices, voronoi_groups);

	// Make regions from groups, then can populate indices and faces
	make_regions(voronoi_groups);

	std::cout << "Doing elevation simulation." << std::endl;
	// Do simulations
	elevation_sim(noise_seed);

	std::cout << "Populating vertex/index vectors." << std::endl;
	// After simulation, populate indices and faces
	populate_mesh_data();
}

void Mesh::generate_vertices(unsigned num_points, int iterations)
{
	std::vector<glm::vec3> points;
	for (unsigned i = 0; i < num_points; i++)
	{
		// Generate random point on unit sphere
		glm::vec3 pt(glm::normalize(glm::ballRand(1.0f)));
		points.push_back(pt);
	}

	for (int i = 0; i < iterations; i++)
	{
		Voronoi v(points);
		std::vector<glm::vec3> new_points = v.getCenters();
		points.clear();
		for (auto& p : new_points)
			points.push_back(glm::normalize(p));
	}
	hull_points = points;
}

void Mesh::make_regions(std::vector<std::vector<size_t>> groups)
{
	for (auto& g : groups)
		if (g.size() > 0) regions.push_back(new Region(g, &vertices));
}

void Mesh::elevation_sim(unsigned noise_seed)
{
	// Populate the elevation multiplier for each regions
	SimplexNoise sn(el_frequency, el_amplitude, el_lacunarity, el_persistence, noise_seed);
	for (Region *r : regions)
		r->set_elevation(sn);

	std::vector<int> owned_regions(vertices.size(), 0);
	std::vector<float> multipliers(vertices.size(), 0.0f);

	for (Region *r : regions)
	{
		r->update_affected(owned_regions, multipliers);
	}

	// Update the elevation of each vertex based on the regions its in
	for (size_t i = 0; i < vertices.size(); i++)
	{
		vertices[i] = vertices[i] * (multipliers[i] / owned_regions[i]);
	}
}

void Mesh::populate_mesh_data()
{
	for (Region *r : regions)
		r->addMeshData(lines, faces);
}

Region::Region(std::vector<size_t> idx_data, std::vector<glm::vec3>* vertices)
{
	// Record pointer to vertices
	this->vertices = vertices;
	// Copy over indices of this region
	indices = idx_data;
	// Calculate center, add it to vertices, and record the index
	glm::vec3 center(0.0f);
	for (size_t idx : indices)
	{
		center += vertices->at(idx);
	}
	center /= (static_cast<float>(indices.size()));
	center = glm::normalize(center);
	center_idx = vertices->size();
	vertices->push_back(center);
}

void Region::set_elevation(SimplexNoise sn)
{
	glm::vec3 center(vertices->at(center_idx));
	// Between -1 and 1, so divide by 10 to have +/- 10% variation in elevation
	elevation_multiplier = 1.0f;
	float x(center.x);
	float y(center.y);
	float z(center.z);

	elevation_multiplier +=  sn.fractal(24, x, y, z) / elevation_divisor;
}
void Region::update_affected(std::vector<int>& owned_regions, std::vector<float>& multipliers)
{
	for (size_t idx : indices)
	{
		owned_regions[idx]++;
		multipliers[idx] += elevation_multiplier;
	}
	owned_regions[center_idx]++;
	multipliers[center_idx] += elevation_multiplier;
}


void Region::addMeshData(std::vector<glm::uvec2>& lines, std::vector<glm::uvec3>& faces)
{
	// Fill in indices
	for (size_t i = 0; i < indices.size() - 1; i++)
	{
		size_t idx1(indices[i]);
		size_t idx2(indices[i+1]);
		// Lines
		lines.push_back(glm::uvec2(idx1, idx2));

		// Triangle
		faces.push_back(glm::uvec3(center_idx, idx1, idx2));
	}

	// Fill in indices at ends
	size_t first_idx(indices[0]);
	size_t last_idx(indices[indices.size() - 1]);

	lines.push_back(glm::uvec2(last_idx, first_idx));
	faces.push_back(glm::uvec3(center_idx, last_idx, first_idx));
}