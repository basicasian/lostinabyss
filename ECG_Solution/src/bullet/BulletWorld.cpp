#include "BulletWorld.h"

BulletWorld::BulletWorld(btVector3 gravity)
{

    // more fine and accurate collision detection
    _collisionConfiguration = new btDefaultCollisionConfiguration();
    _dispatcher = new btCollisionDispatcher(_collisionConfiguration);

    // eliminatee objects that cannot collide
    _broadphase = new btDbvtBroadphase();

    // for objects to interact properly
    _solver = new btSequentialImpulseConstraintSolver();

    // create world 
    _world = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver, _collisionConfiguration);

    // set gravity
    _world->setGravity(gravity);
}

void BulletWorld::stepSimulation(btScalar timeStep, int maxSubSteps, btScalar fixedTimeStep)
{
    _world->stepSimulation(timeStep, maxSubSteps, fixedTimeStep);
}

void BulletWorld::deleteBullet()
{  
    delete _broadphase;
    delete _collisionConfiguration;
    delete _dispatcher;
    delete _solver;
    delete _world;
}



