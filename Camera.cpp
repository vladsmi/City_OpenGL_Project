#include "Camera.hpp"

namespace gps {

	//Camera constructor
	Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
		//TODO
		this->cameraPosition = cameraPosition;
		this->cameraTarget = cameraTarget;

		this->cameraUpDirection = cameraUp;
		this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
		this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
	}

	glm::mat4 Camera::getViewMatrix() {
		return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);;
	}

	void Camera::move(MOVE_DIRECTION direction, float speed) {
		if (direction == MOVE_FORWARD) {
			cameraPosition += cameraFrontDirection * speed;
		}
		if (direction == MOVE_BACKWARD) {
			cameraPosition -= cameraFrontDirection * speed;
		}
		if (direction == MOVE_LEFT) {
			cameraPosition -= cameraRightDirection * speed;
		}
		if (direction == MOVE_RIGHT) {
			cameraPosition += cameraRightDirection * speed;
		}
		cameraTarget = cameraPosition + cameraFrontDirection;
	}

	void Camera::rotate(float pitch, float yaw) {
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		this->cameraFrontDirection = glm::normalize(direction);
		this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
		this->cameraTarget = cameraPosition + cameraFrontDirection;
	}

}