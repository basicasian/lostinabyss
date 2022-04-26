#pragma once

#include <bullet/btBulletDynamicsCommon.h>

class BulletWorld
{
private:
	btBroadphaseInterface* _broadphase; 
	btDefaultCollisionConfiguration* _collisionConfiguration;
	btCollisionDispatcher* _dispatcher;
	btSequentialImpulseConstraintSolver* _solver;
	


public:
	/*!
	* initialize a new bullet world
	* @param gravity: gravity of the bullet world
	*/
	BulletWorld(btVector3 gravity);

	btDiscreteDynamicsWorld* _world;

	void stepSimulation(btScalar timeStep, int maxSubSteps = 1, btScalar fixedTimeStep = btScalar(1.) / btScalar(60.));

	void deleteBullet();
};


