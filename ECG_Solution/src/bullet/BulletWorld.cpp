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

float BulletWorld::rayTestHits(glm::vec3 from, glm::vec3 to)
{
    btVector3 btFrom = { from.x, from.y, from.z };
    btVector3 btTo = { to.x, to.y, to.z };
    btCollisionWorld::ClosestRayResultCallback res(btFrom, btTo);
    _world->rayTest(btFrom, btTo, res);

    if (!res.hasHit())
        return std::numeric_limits<float>::max();
    else
        return (res.m_hitPointWorld - btFrom).length();
}

void BulletWorld::removeRigidBody(btRigidBody* body)
{
    _world->removeRigidBody(body);
}

void BulletWorld::addRigidBody(btRigidBody* body)
{
    _world->addRigidBody(body);
}

bool BulletWorld::checkWinCondition() {
    int numManifolds = _world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        // take each manifold object from the internal manifolds array by index
        btPersistentManifold* contactManifold = _world->getDispatcher()->getManifoldByIndexInternal(i);

        // get the number of contacts and check that there is at least one contact between the pair of bodies
        int numContacts = contactManifold->getNumContacts();

        if (numContacts > 0)
        {
            // get the collision objects  and then the pointer to the bullet objects
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();

            BulletBody* btA = (BulletBody*)obA->getUserPointer();
            BulletBody* btB = (BulletBody*)obB->getUserPointer();

            if (btA->getTag() == btWin || btB->getTag() == btWin) {
                return true;
            }
        }
    }
    return false;
}



