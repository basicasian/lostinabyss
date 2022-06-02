#include "CameraPlayer.h"

CameraPlayer::CameraPlayer(glm::vec3 position, float horDeg, float vertDeg) :
    _front(glm::vec3(0.0f, 0.0f, -1.0f)),
    _worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    _movementSpeed(_SPEED),
    _mouseSensitivity(_SENSITIVITY)
{
    _position = position;
    _horizonDegree = horDeg;
    _verticalDegree = vertDeg;
    updateCameraVectors();
    initPhysics();
}

CameraPlayer::CameraPlayer(float posX, float posY, float posZ, float horDeg, float vertDeg) :
    _front(glm::vec3(0.0f, 0.0f, -1.0f)),
    _worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    _movementSpeed(_SPEED),
    _mouseSensitivity(_SENSITIVITY)
{
    _physicsWorld = nullptr;
    _position = glm::vec3(posX, posY, posZ);
    _horizonDegree = horDeg;
    _verticalDegree = vertDeg;
    updateCameraVectors();
    initPhysics();
}

void CameraPlayer::updateCameraVectors()
{
    // update front, right and up vector
    glm::vec3 front;
    front.x = cos(glm::radians(_horizonDegree)) * cos(glm::radians(_verticalDegree));
    front.y = sin(glm::radians(_verticalDegree));
    front.z = sin(glm::radians(_horizonDegree)) * cos(glm::radians(_verticalDegree));
    _front = glm::normalize(front);
    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}

void CameraPlayer::initPhysics()
{
    _physicsWorld = nullptr;
    _bulletShape = new btCapsuleShape(_SIZE / 2, _SIZE);
    _motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), { _position.x, _position.y, _position.z }));
    btVector3 fallInertia;
    _bulletShape->calculateLocalInertia(_MASS, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo info = btRigidBody::btRigidBodyConstructionInfo(_MASS, _motionState, _bulletShape, fallInertia);
    info.m_linearDamping = btScalar(0);
    info.m_angularDamping = btScalar(0);
    info.m_friction = btScalar(_FRICTION);
    info.m_rollingFriction = btScalar(0);
    info.m_spinningFriction = btScalar(0);

    _rigidBody = new btRigidBody(info);
    _rigidBody->setActivationState(DISABLE_DEACTIVATION);
    _rigidBody->setAngularFactor(0.0);
}

void CameraPlayer::setProjectionMatrix(GLfloat camera_fov, GLfloat camera_far, GLfloat camera_near, GLfloat camera_asp_ratio)
{
    _projMatrix = glm::perspective(glm::radians(camera_fov), camera_asp_ratio, camera_near, camera_far);
}

glm::mat4 CameraPlayer::getViewMatrix()
{
    glm::vec3 position = getPosition() + glm::vec3(0, _SIZE / 2, 0);;

    return glm::lookAt(position, position + _front, _up);
}

glm::mat4 CameraPlayer::getProjectionViewMatrix()
{
    return _projMatrix * getViewMatrix();
}

glm::vec3 CameraPlayer::getPosition()
{
    btTransform transform;
    _motionState->getWorldTransform(transform);
    return glm::vec3(transform.getOrigin().x(), transform.getOrigin().y(), transform.getOrigin().z());
}

void CameraPlayer::inputKeys(KeyInput& input, double deltaTime)
{
    glm::vec3 dir = glm::vec3();

    if (input.forward)
        dir += _front;
    if (input.backward)
        dir -= _front;
    if (input.right)
        dir += _right;
    if (input.left)
        dir -= _right;

    // Remove vertical component
    dir = dir * (glm::vec3(1, 1, 1) - _worldUp);
    if (input.jump) {
        dir += _worldUp * 1.5f;
    }

    glm::vec3 force = glm::normalize(dir) * _movementSpeed * _MASS;

    if (!glm::any(glm::isnan(force))) {
        if (!input.jump)
            _rigidBody->applyCentralForce({ force.x, force.y, force.z });
    }

    bool groundTouched = false;
    float distanceFromPlayerToGround;
    glm::vec3 currPosition(getPosition());
    glm::vec3 direction = glm::vec3(0, -1000, 0);
    distanceFromPlayerToGround = _physicsWorld->rayTestHits(currPosition, direction);

    // ground level = 1, but adding 0.2 for buffer
    if (distanceFromPlayerToGround <= 1.2) {
        groundTouched = true;
    } 

    // only jump if the ground is touched
    if (input.jump && groundTouched)
    {
        _rigidBody->applyCentralImpulse({ 0, 2, 0 });
    }
}

void CameraPlayer::inputMouseMovement(double xoffset, double yoffset)
{
    xoffset *= _mouseSensitivity;
    yoffset *= _mouseSensitivity;

    _horizonDegree += xoffset;
    _verticalDegree += yoffset;

    // to avoid camera from flipping over 
    if (_verticalDegree > 89.0f)
        _verticalDegree = 89.0f;
    if (_verticalDegree < -89.0f)
        _verticalDegree = -89.0f;

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void CameraPlayer::addToWorld(BulletWorld& physicsWorld)
{
    if (_physicsWorld)
        _physicsWorld->removeRigidBody(_rigidBody);

    physicsWorld.addRigidBody(_rigidBody);
    _physicsWorld = &physicsWorld;
    _rigidBody->setUserPointer(this);
}

void CameraPlayer::moveTo(glm::vec3 newLocation)
{
    btTransform transform;
    _motionState->getWorldTransform(transform);

    transform.setOrigin({ newLocation.x, newLocation.y, newLocation.z });

    _motionState->setWorldTransform(transform);
    _rigidBody->setWorldTransform(transform);
    _rigidBody->activate();
    
}

