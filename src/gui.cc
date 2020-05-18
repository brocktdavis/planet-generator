#include "gui.h"
#include "config.h"
#include <debuggl.h>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/io.hpp>

// Constructor
GUI::GUI(GLFWwindow* window, int view_width, int view_height, int preview_height)
	:window_(window), preview_height_(preview_height)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	if (view_width < 0 || view_height < 0) {
		view_width_ = window_width_;
		view_height_ = window_height_;
	} else {
		view_width_ = view_width;
		view_height_ = view_height;
	}
	float aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

// Destructor
GUI::~GUI()
{
}

// Assigning mesh
void GUI::assignMesh(Mesh* mesh)
{
	mesh_ = mesh;
}

// Static event callback handlers
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}

// Getting/Updating matrices
void GUI::updateMatrices()
{
	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = eye_;

	aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_;
	ret.model= &model_matrix_;
	ret.view = &view_matrix_;
	return ret;
}

// Internal event callback handlers
void GUI::keyCallback(int key, int scancode, int action, int mods)
{
	// Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}

	// WASD
	if (mods == 0 && captureWASD(key, action))
		return ;
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	if (mouse_x > view_width_)
		return ;

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;

	
	if (drag_camera) {
		// Normalized eye
		glm::vec3 eyeNorm = glm::normalize(eye_);
		// Rotating up or down
		int dir = glm::sign(delta_y);
		
		// Keep from rotating to very top or bottom
		if (eyeNorm.y > 0.98f && dir < 0)
			return;
		if (eyeNorm.y < -0.98f && dir > 0)
			return;

		glm::vec3 axis(glm::cross(up_, eyeNorm));

		eye_ = glm::rotate(eye_, roll_speed_ * dir, axis);
	}
}

void GUI::mouseButtonCallback(int button, int action, int mods)
{
	if (current_x_ <= view_width_) {
		drag_state_ = (action == GLFW_PRESS);
		current_button_ = button;
	}
}

bool GUI::captureWASD(int key, int action)
{
	// If single tap, just transform once
	if (action == GLFW_RELEASE)
		return false;

	// Zoom: move eye forward or back along the view axis
	if (key == GLFW_KEY_W) {
		glm::vec3 dir(glm::normalize(eye_));
		eye_ -= zoom_speed_ * dir;
		return true;
	} else if (key == GLFW_KEY_S) {
		glm::vec3 dir(glm::normalize(eye_));
		eye_ += zoom_speed_ * dir;

	// Rotate: rotate eye and up around the Y axis
	} else if (key == GLFW_KEY_A) {
		eye_ = glm::rotateY(eye_, -roll_speed_);
		return true;
	} else if (key == GLFW_KEY_D) {
		eye_ = glm::rotateY(eye_, roll_speed_);
		return true;
	}
	return false;
}
