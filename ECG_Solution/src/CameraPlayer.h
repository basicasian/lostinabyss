#pragma once

#include "Utils.h"
#include <bullet/btBulletDynamicsCommon.h>
#include "bullet/BulletWorld.h"
#include "bullet/BulletBody.h"
#include <glm/glm.hpp>

// camera default values 
const float _HORDEG = -90.0f;
const float _VERTDEG = 0.0f;
const float _SPEED = 25.0f;
const float _MAX_JUMP = 125.0f;
const float _FLYSPEED = 20.0f;
const float _SENSITIVITY = 0.08f;
const float _SIZE = 1;
const float _MASS = 10;
const float _FRICTION = 1.5;

struct KeyInput
{
    bool forward;
    bool backward;
    bool left;
    bool right;
    bool jump;
};

class CameraPlayer
{

private:
    // camera Attributes
    glm::vec3 _position;
    glm::vec3 _front; // direction vector
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;
    // euler Angles
    float _horizonDegree;
    float _verticalDegree;
    // camera options
    float _flyspeed;
    float _movementSpeed;
    float _mouseSensitivity;

    bool _won = false;

    glm::vec3 _jumpImpulse;

    glm::mat4 _projMatrix;

    btRigidBody* _rigidBody;

    BulletWorld* _physicsWorld;

    btCapsuleShape* _bulletShape;

    btMotionState* _motionState;

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

    void initPhysics();

public:

    // constructor with vectors
    CameraPlayer(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float horDeg = _HORDEG, float vertDeg = _VERTDEG);

    // constructor with scalar values
    CameraPlayer(float posX, float posY, float posZ, float horDeg, float vertDeg);

    void setProjectionMatrix(GLfloat camera_fov, GLfloat camera_far, GLfloat camera_near, GLfloat camera_asp_ratio);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix();

    // returns the projection view matrix
    glm::mat4 getProjectionViewMatrix();

    // receive input from keyboard, move according the direction
    void inputKeys(KeyInput& input, double deltaTime);

    // receive input when mouse is moved
    void inputMouseMovement(double xoffset, double yoffset);

    bool getWon();

    // Inherited via Object
    virtual void addToWorld(BulletWorld& physicsWorld);
    virtual void moveTo(glm::vec3 newLocation);
    virtual glm::vec3 getPosition();
};