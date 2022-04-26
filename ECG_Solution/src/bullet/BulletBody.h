#pragma once
#include <bullet/btBulletDynamicsCommon.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "../Mesh.h"
#include "../Camera.h"
#include "../Utils.h"


// no semicolons after #define statement!
#define btTag      1

enum Bullet_Movement {
	FORWARD_B,
	BACKWARD_B,
	LEFT_B,
	RIGHT_B,
	UP_B,
};

/*!
 * Physics Node Implementation based on Bullet
 */
class BulletBody
{
	/// BULLET
	/*!
	* Mass of the object
	* zero, immovable -> static
	* zero, movable -> kinematic
	* non-zero, movable -> dynmamic
	*/
	float _mass;

	/*!
	* Flag if object is convex or concave
	* should always try to use convex objects because physics engines work much faster with them
	*/
	bool _convex;

	/*!
	*  reference to a rigid body that you’ll create
	*/
	btRigidBody* _body;

	/*!
	*  describes the shape of the physics body
	*/
	btCollisionShape* _shape;

	/*!
	*  data of geometry shape
	*/
	std::vector<Mesh> _data;

	/*!
	*  position of this bullet object
	*/
	glm::vec3 _position;

	/*!
	*  specifiy the type of bullet body for collision detection
	*/
	int _tag;


	/*!
	*  if body is hit
	*/
	boolean _hit = false;



public:
	/*!
	* constructor
	* @param tag: to specifiy the bullet object
	* @param data: shape data from mesh
	* @param mass: mass of the body
	* @param convex: if the shape is convec
	* @param position: of the body
	* @param dynamics_world: to add the bodies to the world
	*/
	BulletBody(int tag, std::vector<Mesh> data, float mass, boolean convex, glm::vec3 position, btDiscreteDynamicsWorld* dynamics_world);

	/*!
	 * Creates Shape with vertices
	* @param width, height, depth: size of the cube simplifiation
	 */
	void createShapeWithVertices();
	//void createShapeWithVertices(float width, float height, float depth);

	/*!
	 * Creates Body with mass
	 * @param _yaw: yaw of camera //REMOVED
	 * @param _pitch: pitch of camera //REMOVED
	 * @param _dynamics_world: dynamic world to add bodies
	 */
	void BulletBody::createBodyWithMass(btDiscreteDynamicsWorld* _dynamics_world);

	/*!
	 * set position
	 * @param position: position to be set
	 */
	void setPosition(glm::vec3 position);

	/*!
	 * get position
	 */
	glm::vec3 getPosition();

	/*!
	 * get front vector
	 */
	glm::vec3 getFrontVector();

	/*!
	 * get tag
	 */
	int getTag();

	/*!
	* process the keyboard into camera movement
	* @param direction : keyboard input
	*/
	//void ProcessKeyboardBullet(Bullet_Movement direction);

	/*!
	* destroys the bullet body
	* @param dynamics_world : bullet worlds
	*/
	void BulletBody::destroyBody(btDiscreteDynamicsWorld* dynamics_world);
};