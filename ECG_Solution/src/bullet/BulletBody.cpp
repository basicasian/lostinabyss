
#include "BulletBody.h"

BulletBody::BulletBody(int tag, std::vector<Mesh> data, float mass, boolean convex, glm::vec3 position, btDiscreteDynamicsWorld* dynamics_world)
	: _mass(mass), _convex(convex), _data(data), _position(position), _tag(tag)
{
	createShapeWithVertices();
	createBodyWithMass(dynamics_world);

}

void BulletBody::createShapeWithVertices() {

	// takes different approaches to create convex and concave shapes
	if (_convex) {

		// allows you to add all points of the object 
		// and uses them to automatically create the minimum convex hull

		// simplification as a box, width, height and depth has to be added in parameter 
		//_shape = new btBoxShape(btVector3(btScalar(width), btScalar(height), btScalar(depth)));

		_shape = new btConvexHullShape();
		for (auto& mesh : _data) {
			for (int i = 0; i < mesh._vertices.size(); i++) {

				btVector3 btv = btVector3(mesh._vertices[i].Position.x, mesh._vertices[i].Position.y, mesh._vertices[i].Position.z);
				((btConvexHullShape*)_shape)->addPoint(btv);
			}
		}

	}
	else {
		// gather triangles by grouping vertices from the list of vertices
		btTriangleMesh* mesh = new btTriangleMesh();

		for (auto& modelMesh : _data) {
			for (int i = 0; i < modelMesh._vertices.size(); i += 3) {

				btVector3 bv1 = btVector3(modelMesh._vertices[i].Position.x, modelMesh._vertices[i].Position.y, modelMesh._vertices[i].Position.z);
				btVector3 bv2 = btVector3(modelMesh._vertices[i + 1].Position.x, modelMesh._vertices[i + 1].Position.y, modelMesh._vertices[i + 1].Position.z);
				btVector3 bv3 = btVector3(modelMesh._vertices[i + 2].Position.x, modelMesh._vertices[i + 2].Position.y, modelMesh._vertices[i + 2].Position.z);

				mesh->addTriangle(bv1, bv2, bv3);
			}
		}

		_shape = new btBvhTriangleMeshShape(mesh, true);
	}
}

void BulletBody::createBodyWithMass(btDiscreteDynamicsWorld* _dynamics_world) {

	// quaternions to represent object rotation
	//btQuaternion rotation;
	//rotation.setEulerZYX(_yaw, _pitch, 0);

	// specify the object’s position
	btVector3 position = btVector3(_position.x, _position.y, _position.z);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(position);

	// allows you to sync a physical body and with your drawable objects
	//btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(rotation, position));
	btDefaultMotionState* motionState = new btDefaultMotionState(transform);

	// set the mass and inertia values for the shape
	btScalar bodyMass = _mass;
	btVector3 bodyInertia(0, 0, 0);
	if (_mass != 0.0f) {
		_shape->calculateLocalInertia(bodyMass, bodyInertia);
	}

	// ConstructionInfo contains all the required properties to construct the body
	btRigidBody::btRigidBodyConstructionInfo bodyInfo = btRigidBody::btRigidBodyConstructionInfo(bodyMass, motionState, _shape, bodyInertia);

	// bodyCI.m_restitution sets an object’s bounciness
	// m=0   - sticks to the floor
	// 0>m>1 - object bounces, but with each bounce loses part of its energy
	// m>1   - object gains energy with each bounce, not realistic
	bodyInfo.m_restitution = 0.0f;
	// bodyCI.m_friction to non-zero, will make the object spin
	bodyInfo.m_friction = 0.5f;

	_body = new btRigidBody(bodyInfo);

	_body->setUserPointer(this);

	_dynamics_world->addRigidBody(_body);
}

void BulletBody::setPosition(glm::vec3 position) {

	// transform is a translation [vector] plus a rotation [quaternion]
	btTransform trans = _body->getWorldTransform();
	trans.setOrigin(btVector3(position.x, position.y, position.z));
	_body->setWorldTransform(trans);

}

glm::vec3 BulletBody::getPosition() {

	btTransform trans = _body->getWorldTransform();
	return glm::vec3(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
}

glm::vec3 getFrontVector() {

	return glm::vec3(0, 0, 0);
}

int BulletBody::getTag() {
	return _tag;
}


/*
void BulletBody::ProcessKeyboardBullet(Bullet_Movement direction)
{
	if (direction == FORWARD_B)
		_body->setLinearVelocity(btVector3(0, 0, 1));

	if (direction == BACKWARD_B)
		_body->setLinearVelocity(btVector3(0, 0, -1));

	if (direction == RIGHT_B)
		_body->setLinearVelocity(btVector3(-1, 0, 0));

	if (direction == LEFT_B)
		_body->setLinearVelocity(btVector3(1, 0, 0));

	if (direction == UP_B)
		_body->setLinearVelocity(btVector3(0, 1, 0));

}*/

void BulletBody::destroyBody(btDiscreteDynamicsWorld* dynamics_world) {
	dynamics_world->removeRigidBody(this->_body);

}