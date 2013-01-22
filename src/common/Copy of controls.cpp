// Include GLFW
#include <GL/glfw.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include<iostream>

#include "controls.hpp"

using namespace std;

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix(){
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix(){
	return ProjectionMatrix;
}


// Initial position : on +Z
glm::vec3 position = glm::vec3( 0, 0, 5 ); 
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 3.0f; // 3 units / second

bool rotation = false;
int rot_x = 0.0f;
int rot_y = 0.0f;
float rot_accel = 0.005f;

bool zoom = false;
int zoom_x = 0.0f;
int zoom_y = 0.0f;
float zoom_accel = 0.01f;

bool pan = false;
int pan_x = 0.0f;
int pan_y = 0.0f;
float pan_accel = 0.01f;


void MouseButtonCallback(int button, int action)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT){
	  if (action == GLFW_PRESS){
		  rotation = true;
		  glfwGetMousePos(&rot_x, &rot_y);
	  }
	  else{
		  rotation = false;
	  }
  }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT){
	  if (action == GLFW_PRESS){
		  zoom = true;
		  glfwGetMousePos(&zoom_x, &zoom_y);
	  }
	  else{
		  zoom = false;
	  }
  }
  else if (button == GLFW_MOUSE_BUTTON_MIDDLE){
  	  if (action == GLFW_PRESS){
  		  pan = true;
  		  glfwGetMousePos(&pan_x, &pan_y);
  	  }
  	  else{
  		  pan = false;
  	  }
    }
}

void computeMatricesFromInputs(){

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	int xpos, ypos;
	glfwGetMousePos(&xpos, &ypos);

	// Reset mouse position for next frame
	//glfwSetMousePos(1024/2, 768/2);

	// Compute new orientation
	if(rotation){
		horizontalAngle += rot_accel * float(rot_x - xpos);
		verticalAngle   += rot_accel * float(rot_y - ypos);
		rot_x = xpos;
		rot_y = ypos;
	}


	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle), 
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);
	
	// Right vector
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f/2.0f), 
		0,
		cos(horizontalAngle - 3.14f/2.0f)
	);
	
	// Up vector
	glm::vec3 up = glm::cross( right, direction );

	if(zoom){
		position += direction * ((-zoom_x + zoom_y + xpos - ypos)*zoom_accel);
		zoom_x = xpos;
		zoom_y = ypos;
	}

	if(pan){
		position += right * ((pan_x-xpos)*pan_accel);
		position += up    * (-(pan_y-ypos)*pan_accel);
		pan_x = xpos;
		pan_y = ypos;
	}

	// Move forward
	if (glfwGetKey( GLFW_KEY_UP ) == GLFW_PRESS){
		position += direction * deltaTime * speed;
	}
	// Move backward
	if (glfwGetKey( GLFW_KEY_DOWN ) == GLFW_PRESS){
		position -= direction * deltaTime * speed;
	}
	// Strafe right
	if (glfwGetKey( GLFW_KEY_RIGHT ) == GLFW_PRESS){
		position += right * deltaTime * speed;
	}
	// Strafe left
	if (glfwGetKey( GLFW_KEY_LEFT ) == GLFW_PRESS){
		position -= right * deltaTime * speed;
	}

	float FoV = initialFoV - 5 * glfwGetMouseWheel();

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(FoV, 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	ViewMatrix       = glm::lookAt(
								position,           // Camera is here
								position+direction, // and looks here : at the same position, plus "direction"
								up                  // Head is up (set to 0,-1,0 to look upside-down)
						   );

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
	glfwSetMouseButtonCallback(MouseButtonCallback);
}



