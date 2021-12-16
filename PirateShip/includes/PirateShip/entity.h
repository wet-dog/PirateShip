#pragma once
#ifndef ENTITY_H
#define ENTITY_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

#include <PirateShip/plane.h>
#include <PirateShip/collision_package.h>
#include <PirateShip/math.h>

class CharacterEntity 
{
public:

	glm::vec3 position;
	glm::vec3 velocity;
	bool grounded = false;
	int collisionRecursionDepth;
	CollisionPackage* collisionPackage;
	std::vector<std::vector<glm::vec3>> triangles;

	CharacterEntity() {
		collisionPackage = new CollisionPackage();
		position = glm::vec3(0, 0, 0);
		velocity = glm::vec3(0, 0, 0);
		collisionRecursionDepth = 0;
		collisionPackage->eRadius = glm::vec3(0.5f, 1.05f, 0.5f);
	}

	void collideAndSlide(const glm::vec3& vel, const glm::vec3& gravity)
	{
		// Do collision detection:
		collisionPackage->R3Position = position;
		collisionPackage->R3Velocity = vel;
		// calculate position and velocity in eSpace
		glm::vec3 eSpacePosition = collisionPackage->R3Position / collisionPackage->eRadius;
		glm::vec3 eSpaceVelocity = collisionPackage->R3Velocity / collisionPackage->eRadius;
		// Iterate until we have our final position.
		collisionRecursionDepth = 0;
		//glm::vec3 finalPosition = collideWithWorld(eSpacePosition, eSpaceVelocity);
		glm::vec3 finalPosition = collideWithWorld(eSpacePosition, eSpaceVelocity);
		// Add gravity pull:
		// To remove gravity uncomment from here .....
		// Set the new R3 position (convert back from eSpace to R3
		collisionPackage->R3Position = finalPosition * collisionPackage->eRadius;
		collisionPackage->R3Velocity = gravity;
		eSpaceVelocity = gravity / collisionPackage->eRadius;
		collisionRecursionDepth = 0;
		//finalPosition = collideWithWorld(finalPosition, eSpaceVelocity);
		finalPosition = collideWithWorld(finalPosition, eSpaceVelocity);
		// ... to here
		// Convert final result back to R3:
		finalPosition = finalPosition * collisionPackage->eRadius;
		position = finalPosition;
	}

	// Set this to match application scale..
	const float unitsPerMeter = 100.0f;
	glm::vec3 collideWithWorld(const glm::vec3& pos, const glm::vec3& vel)
	{
		// All hard-coded distances in this function is
		// scaled to fit the setting above..
		float unitScale = unitsPerMeter / 100.0f;
		float veryCloseDistance = 0.005f * unitScale;
		// do we need to worry?
		if (collisionRecursionDepth > 5)
			return pos;
		// Ok, we need to worry:
		collisionPackage->velocity = vel;
		collisionPackage->normalizedVelocity = vel;
		collisionPackage->normalizedVelocity = glm::normalize(collisionPackage->normalizedVelocity);
		collisionPackage->basePoint = pos;
		collisionPackage->foundCollision = false;
		// Check for collision (calls the collision routines)
		// Application specific!!
		checkCollision();
		// If no collision we just move along the velocity
		if (collisionPackage->foundCollision == false) {
			return pos + vel;
		}
		// *** Collision occured ***
		// The original destination point
		glm::vec3 destinationPoint = pos + vel;
		glm::vec3 newBasePoint = pos;
		// only update if we are not already very close
		// and if so we only move very close to intersection..not
		// to the exact spot.
		if (collisionPackage->nearestDistance >= veryCloseDistance)
		{
			glm::vec3 V = glm::normalize(vel) * ((float)collisionPackage->nearestDistance - veryCloseDistance);
			// Commented out:
			//newBasePoint = collisionPackage->basePoint + V;
			// Adjust polygon intersection point (so sliding
			// plane will be unaffected by the fact that we
			// move slightly less than collision tells us)
			V = glm::normalize(V);
			collisionPackage->intersectionPoint -= veryCloseDistance * V;
		}
		// Determine the sliding plane
		glm::vec3 slidePlaneOrigin = collisionPackage->intersectionPoint;
		glm::vec3 slidePlaneNormal = newBasePoint - collisionPackage->intersectionPoint;
		slidePlaneNormal = glm::normalize(slidePlaneNormal);
		Plane slidingPlane(slidePlaneOrigin, slidePlaneNormal);
		// Again, sorry about formatting.. but look carefully ;)
		glm::vec3 newDestinationPoint = destinationPoint - slidingPlane.signedDistanceTo(destinationPoint) * slidePlaneNormal;
		// Generate the slide vector, which will become our new
		// velocity vector for the next iteration
		glm::vec3 newVelocityVector = newDestinationPoint - collisionPackage->intersectionPoint;
		// Recurse:
		// dont recurse if the new velocity is very small
		if (newVelocityVector.length() < veryCloseDistance) {
			return newBasePoint;
		}
		collisionRecursionDepth++;
		return collideWithWorld(newBasePoint, newVelocityVector);

		//checkCollision();
	}

	void checkCollision()
	{
		for (std::vector<glm::vec3> triangle : triangles) {
			Math::checkTriangle(collisionPackage, triangle[0], triangle[1], triangle[2]);
		}
	}


	void update(bool useGravity)
	{
		this->grounded = false;
		glm::vec3 gravity;
		if (useGravity) {
			gravity = { 0.0f, -0.01f, 0.0f };
		}
		else {
			gravity = { 0.0f, 0.0f, 0.0f };
		}
		gravity = { 0.0f, 0.0f, 0.0f };

		collideAndSlide(gravity, velocity);
	}
};
#endif
