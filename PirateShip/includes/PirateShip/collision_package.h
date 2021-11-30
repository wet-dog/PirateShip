#ifndef COLLISION_PACKAGE_H
#define COLLISION_PACKAGE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

class CollisionPackage {
public:
	glm::vec3 eRadius; // ellipsoid radius
	// Information about the move being requested: (in R3)
	glm::vec3 R3Velocity;
	glm::vec3 R3Position;
	// Information about the move being requested: (in eSpace)
	glm::vec3 velocity;
	glm::vec3 normalizedVelocity;
	glm::vec3 basePoint;
	// Hit information
	bool foundCollision;
	double nearestDistance;
	glm::vec3 intersectionPoint;

	// iteration depth
	int collisionRecursionDepth;
};
#endif
