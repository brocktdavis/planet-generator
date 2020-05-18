#ifndef VORONOI_H
#define VORONOI_H

#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

// Assumes center is inside the unit sphere (i.e. not on the surface of it)
struct polyVec {

	polyVec(glm::vec3 pt, glm::vec3 cen, glm::vec3 tp, size_t index) : point(pt), center(cen), top(tp), idx(index) {}

	glm::vec3 point;
	glm::vec3 center;
	glm::vec3 top;
	size_t idx;

	bool CCW;
	float dot;

	// Sort vertices in order either clockwise or counter-clockwise
	static bool compare(polyVec v1, polyVec v2) {

		// It's the first vertex, it should stay first
		if (v1.point == v1.top) {
			return true;
		} else if (v2.point == v2.top) {
			return false;
		}

		v1.setCCW_dot();
		v2.setCCW_dot();
		// If both rotating CCW, return v with smallest angle to up
		if (v1.CCW && v2.CCW) {
			return v1.dot > v2.dot;
		// Return the one that rotates CCW
		} else if (v1.CCW) {
			return true;
		} else if (v2.CCW) {
			return false;

		// If both CW, return v with largest angle to up
		} else {
			return v1.dot < v2.dot;
		}
	}

	void setCCW_dot() {
		glm::vec3 outCenter(glm::normalize(center));
		glm::vec3 upNorm(glm::normalize(top - center));
		glm::vec3 dirNorm(glm::normalize(point - center));

		glm::vec3 normal(glm::normalize(glm::cross(upNorm, dirNorm)));

		float normDot(glm::dot(normal, glm::normalize(center - outCenter)));

		CCW = normDot < 0.0f;
		dot = glm::dot(upNorm, dirNorm);
	}
};

class Voronoi {
public:
	Voronoi(std::vector<glm::vec3> points);

	std::vector<glm::vec3> getCenters();
	void getHullIndices(std::vector<glm::uvec2>& indices);
	void getHullFaces(std::vector<glm::uvec3>& faces);

	void getVerticesGroups(std::vector<glm::vec3>& vertices, std::vector<std::vector<size_t>>& groups);

private:
	// Voronoi vertices
	std::vector<glm::vec3> vertices;
	// Groups of vertices that represent a Voronoi cell
	std::vector<std::vector<size_t>> groups;

	std::vector<glm::uvec3> tri_simplices;

	std::vector<size_t> generateConvexHull(std::vector<glm::vec3> points);
	void getTriSimplices(std::vector<size_t> point_indices);
	void generateVertices(std::vector<glm::vec3> points);
	void generateGroups(std::vector<glm::uvec2> array_associations);
	void sortGroups();

	static glm::vec3 getCenter(std::vector<glm::vec3> vertices);
};

#endif