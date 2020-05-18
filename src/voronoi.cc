#include "voronoi.h"

#include <iostream>
#include <algorithm>

#include <glm/gtx/io.hpp>

#include <QuickHull.hpp>


template <typename T>
inline void printVector(std::vector<T> v) { 
	for (auto &e : v)
	{
		std::cout << e << std::endl;
	}
}

inline bool compareUvec(glm::uvec2 v1, glm::uvec2 v2)
{
	if (v1[0] < v2[0]) {
		return true;
	} else if (v1[0] > v2[0]) {
		return false;
	} else {
		return v1[1] < v2[1];
	}
}

// Constructor
Voronoi::Voronoi(std::vector<glm::vec3> points)
{
	// Get the indices of points of triangles that make up the convex hull
	// Need this in two forms
	std::vector<size_t> point_indices(generateConvexHull(points));
	getTriSimplices(point_indices);

	// Get vertices from convex hull triangles (tetrahedrons w/ origin)
	// Project circumcenter of each tetra onto surface of unit sphere
	generateVertices(points);

	std::vector<size_t> tri_indices;
	for (size_t i = 0; i < tri_simplices.size(); i++)
	{
		tri_indices.push_back(i);
		tri_indices.push_back(i);
		tri_indices.push_back(i);
	}

	// Generate array associations
	std::vector<glm::uvec2> array_associations;
	for (size_t i = 0; i < tri_indices.size(); i++)
	{
		array_associations.push_back(glm::uvec2(point_indices[i], tri_indices[i]));
	}
	// Sort first by point index then tri index
	std::sort(array_associations.begin(), array_associations.end(), compareUvec);

	// Generate groups of vertices
	generateGroups(array_associations);
	sortGroups();
}

std::vector<glm::vec3> Voronoi::getCenters()
{
	std::vector<glm::vec3> centers;
	for (auto& group : groups)
	{
		std::vector<glm::vec3> polygonVertices;
		for (size_t idx : group)
		{
			polygonVertices.push_back(vertices[idx]);
		}
		centers.push_back(getCenter(polygonVertices));
	}
	return centers;
}

// Updating vectors for the mesh
void Voronoi::getHullIndices(std::vector<glm::uvec2>& indices)
{
	for (glm::uvec3 triSimplex : tri_simplices)
	{
		indices.push_back(glm::uvec2(triSimplex[0], triSimplex[1]));
		indices.push_back(glm::uvec2(triSimplex[1], triSimplex[2]));
		indices.push_back(glm::uvec2(triSimplex[2], triSimplex[0]));
	}
}

void Voronoi::getHullFaces(std::vector<glm::uvec3>& faces)
{
	faces = this->tri_simplices;
}

void Voronoi::getVerticesGroups(std::vector<glm::vec3>& vertices, std::vector<std::vector<size_t>>& groups)
{
	vertices = this->vertices;
	groups = this->groups;
}

std::vector<size_t> Voronoi::generateConvexHull(std::vector<glm::vec3> points)
{
	using namespace quickhull;
	QuickHull<float> qh;

	auto hull = qh.getConvexHull(&points[0].x, points.size(), false, true, 0.000001f);
	return hull.getIndexBuffer();
}

void Voronoi::getTriSimplices(std::vector<size_t> point_indices)
{
	for (size_t i = 0; i < point_indices.size(); i += 3)
	{
		tri_simplices.push_back(glm::uvec3(point_indices[i], point_indices[i+1], point_indices[i+2]));
	}
}

void Voronoi::generateVertices(std::vector<glm::vec3> points)
{
	for (glm::uvec3 tri_simplex : tri_simplices)
	{
		// From https://en.wikipedia.org/wiki/Tetrahedron#Circumcenter
		// Can be simplified because one of the points is at the origin
		glm::vec3 x1(points[tri_simplex[0]]);
		glm::vec3 x2(points[tri_simplex[1]]);
		glm::vec3 x3(points[tri_simplex[2]]);

		glm::mat3 A(x1, x2, x3);
		A = glm::transpose(A);
		
		// If three points and origin are coplanar
		if (glm::determinant(A) == 0.0f) {

			// If x1 and x2 are colinear, use x3 instead
			if (glm::dot(x1, x2) == -1.0f) {
				x2 = x3;
			}

			// From https://gamedev.stackexchange.com/questions/60630/how-do-i-find-the-circumcenter-of-a-triangle-in-3d
			glm::vec3 x1Xx2(glm::cross(x1, x2));
			glm::vec3 x2Xx1(-x1Xx2);
			glm::vec3 term1(glm::cross(x1Xx2, x1));
			glm::vec3 term2(glm::cross(x2Xx1, x2));
			float denom(2 * glm::dot(x1Xx2, x1Xx2));

			glm::vec3 circumcenter((term1 + term2) / denom);
			vertices.push_back(glm::normalize(circumcenter));

		// Usual case
		} else {

			glm::vec3 B(glm::dot(x1, x1), glm::dot(x2, x2), glm::dot(x3, x3));
			B = 0.5f * B;

			glm::vec3 circumcenter(glm::inverse(A) * B);
			vertices.push_back(glm::normalize(circumcenter));
		}
	}
}

void Voronoi::generateGroups(std::vector<glm::uvec2> array_associations)
{
	size_t cur_group_idx = 0;
	std::vector<size_t> cur_group;
	for (glm::vec2 assoc : array_associations)
	{
		if (assoc[0] != cur_group_idx)
		{
			groups.push_back(cur_group);
			cur_group.clear();
			cur_group_idx++;
		}

		cur_group.push_back(assoc[1]);
	}
	groups.push_back(cur_group);
}

void Voronoi::sortGroups()
{
	std::vector<std::vector<size_t>> newGroups;
	for (auto& group : groups)
	{
		// Get the vertices in the polygon and calculate center
		std::vector<glm::vec3> polygonVertices;
		for (size_t idx : group)
		{
			polygonVertices.push_back(vertices[idx]);
		}
		glm::vec3 center(getCenter(polygonVertices));

		// Put in a struct so they can be sorted by rotation around first point
		std::vector<polyVec> polyVecs;
		for (size_t idx : group)
		{
			polyVec v(vertices[idx], center, polygonVertices[0], idx);
			polyVecs.push_back(v);
		}
		std::sort(polyVecs.begin(), polyVecs.end(), polyVec::compare);

		// Once sorted, add to list of new groups
		std::vector<size_t> sorted_indices;
		for (auto &v : polyVecs)
			sorted_indices.push_back(v.idx);
		newGroups.push_back(sorted_indices);
	}
	// Reassign groups to sorted one
	groups = newGroups;
}

glm::vec3 Voronoi::getCenter(std::vector<glm::vec3> vertices)
{
	glm::vec3 sum(0.0f);
	for (auto &v : vertices)
	{
		sum += v;
	}
	return sum / static_cast<float>(vertices.size());
}