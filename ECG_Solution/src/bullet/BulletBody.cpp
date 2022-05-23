
#include "BulletBody.h"

BulletBody::BulletBody(int tag, aiMesh* data, aiMatrix4x4 transformationMatrix, float mass, boolean convex, btDiscreteDynamicsWorld* dynamics_world)
	: _mass(mass), _convex(convex), _data(data), _tag(tag), _transformationMatrix(transformationMatrix), _dynamics_world(dynamics_world)
{
	createMeshShapeWithVertices();
}

BulletBody::BulletBody(int tag, GeometryData data, float mass, boolean convex, glm::vec3 position, btDiscreteDynamicsWorld* dynamics_world)
	: _mass(mass), _convex(convex), _geoData(data), _position(position), _tag(tag), _dynamics_world(dynamics_world)
{
	createShapeWithVertices();
	createBodyWithMass();

}

BulletBody::BulletBody() {}

void BulletBody::createMeshShapeWithVertices()
{
	glm::mat4 transform = aiMatrixToMat4(_transformationMatrix);

	// Get information from transformation matrix
	glm::vec3 translation = glm::vec3(transform[3]);
	glm::mat3 cutoff = glm::mat3(transform);
	glm::vec3 scale = glm::vec3(glm::length(cutoff[0]), glm::length(cutoff[1]), glm::length(cutoff[2]));
	glm::quat rotation = glm::quat_cast(glm::mat3(cutoff[0] / scale.x, cutoff[1] / scale.y, cutoff[2] / scale.z));

	
	// takes different approaches to create convex and concave shapes
	if (_convex) {

		_shape = new btConvexHullShape();
		for (int i = 0; i < _data->mNumVertices; i++) {

			btVector3 btv = btVector3(_data->mVertices[i].x, _data->mVertices[i].y, _data->mVertices[i].z);
			((btConvexHullShape*)_shape)->addPoint(btv);
		}

	}
	else {
		// gather triangles by grouping vertices from the list of vertices

		btTriangleMesh* mesh = new btTriangleMesh();

		for (int i = 0; i < _data->mNumVertices; i += 3) {

			btVector3 bv1 = btVector3(_data->mVertices[i].x, _data->mVertices[i].y, _data->mVertices[i].z);
			btVector3 bv2 = btVector3(_data->mVertices[i + 1].x, _data->mVertices[i + 1].y, _data->mVertices[i + 1].z);
			btVector3 bv3 = btVector3(_data->mVertices[i + 2].x, _data->mVertices[i + 2].y, _data->mVertices[i + 2].z);

			mesh->addTriangle(bv1, bv2, bv3);
			mesh->setScaling({ scale.x, scale.y, scale.z });
		}
		_shape = new btBvhTriangleMeshShape(mesh, true);
	}
	createMeshBodyWithMass(rotation, translation);
}

void BulletBody::createShapeWithVertices() {
	
	if (_convex) {

		_shape = new btConvexHullShape();
		for (int i = 0; i < _geoData.positions.size(); i++) {

			btVector3 btv = btVector3(_geoData.positions[i].x, _geoData.positions[i].y, _geoData.positions[i].z);
			((btConvexHullShape*)_shape)->addPoint(btv);
		}

	}
	else {
		// gather triangles by grouping vertices from the list of vertices
		btTriangleMesh* mesh = new btTriangleMesh();

		for (int i = 0; i < _geoData.positions.size(); i += 3) {

			btVector3 bv1 = btVector3(_geoData.positions[i].x, _geoData.positions[i].y, _geoData.positions[i].z);
			btVector3 bv2 = btVector3(_geoData.positions[i + 1].x, _geoData.positions[i + 1].y, _geoData.positions[i + 1].z);
			btVector3 bv3 = btVector3(_geoData.positions[i + 2].x, _geoData.positions[i + 2].y, _geoData.positions[i + 2].z);

			mesh->addTriangle(bv1, bv2, bv3);
		}

		_shape = new btBvhTriangleMeshShape(mesh, true);
	}
}

void BulletBody::createMeshBodyWithMass(glm::quat rotation, glm::vec3 translation)
{
	
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), { translation.x, translation.y, translation.z }));

	// set the mass and inertia values for the shape
	btScalar bodyMass = _mass;
	btVector3 bodyInertia;

	if (_mass != 0.0f) {
		_shape->calculateLocalInertia(bodyMass, bodyInertia);
	}

	// ConstructionInfo contains all the required properties to construct the body
	btRigidBody::btRigidBodyConstructionInfo bodyInfo = btRigidBody::btRigidBodyConstructionInfo(bodyMass, motionState, _shape, bodyInertia);

	bodyInfo.m_restitution = 0.0f;
	bodyInfo.m_friction = 0.5f;

	_body = new btRigidBody(bodyInfo);

	_body->setUserPointer(this);

	_dynamics_world->addRigidBody(_body);
}

void BulletBody::createBodyWithMass() {

	btVector3 position = btVector3(_position.x, _position.y, _position.z);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(position);

	btDefaultMotionState* motionState = new btDefaultMotionState(transform);

	// set the mass and inertia values for the shape
	btScalar bodyMass = _mass;
	btVector3 bodyInertia(0, 0, 0);
	if (_mass != 0.0f) {
		_shape->calculateLocalInertia(bodyMass, bodyInertia);
	}

	// ConstructionInfo contains all the required properties to construct the body
	btRigidBody::btRigidBodyConstructionInfo bodyInfo = btRigidBody::btRigidBodyConstructionInfo(bodyMass, motionState, _shape, bodyInertia);

	bodyInfo.m_restitution = 0.0f;
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

int BulletBody::getTag() {
	return _tag;
}

void BulletBody::destroyBody(btDiscreteDynamicsWorld* dynamics_world) {
	dynamics_world->removeRigidBody(this->_body);

}

glm::mat4 BulletBody::aiMatrixToMat4(const aiMatrix4x4& aiMatrix)
{
	glm::mat4 result = glm::mat4();

	result[0][0] = (GLfloat)aiMatrix.a1;
	result[0][1] = (GLfloat)aiMatrix.b1;
	result[0][2] = (GLfloat)aiMatrix.c1;
	result[0][3] = (GLfloat)aiMatrix.d1;
	result[1][0] = (GLfloat)aiMatrix.a2;
	result[1][1] = (GLfloat)aiMatrix.b2;
	result[1][2] = (GLfloat)aiMatrix.c2;
	result[1][3] = (GLfloat)aiMatrix.d2;
	result[2][0] = (GLfloat)aiMatrix.a3;
	result[2][1] = (GLfloat)aiMatrix.b3;
	result[2][2] = (GLfloat)aiMatrix.c3;
	result[2][3] = (GLfloat)aiMatrix.d3;
	result[3][0] = (GLfloat)aiMatrix.a4;
	result[3][1] = (GLfloat)aiMatrix.b4;
	result[3][2] = (GLfloat)aiMatrix.c4;
	result[3][3] = (GLfloat)aiMatrix.d4;

	return result;
}

