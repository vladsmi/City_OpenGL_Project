#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// fog density
GLfloat fogDensity;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint fogDensityLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat carSpeed = 0.4;
GLfloat carRotationAngle = 0.9;

GLfloat cameraSpeed = 0.1f;

GLfloat cameraSensitivity = 0.05;
float pitch, yaw = -90.0f;

GLfloat fogSensitivity = 0.002;

GLboolean pressedKeys[1024];

//SkyBox
gps::SkyBox mySkyBox;

// models
struct Waveform {
    gps::Model3D object;
    glm::mat4 modelMat;
};
gps::Model3D scene;

Waveform car;
glm::vec3 carFrontDirection;
glm::vec3 carRightDirection;
glm::mat4 carCameraModel = glm::mat4(1.0f);

Waveform arrow;

GLfloat angle;

// shaders
gps::Shader myBasicShader, skyboxShader;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

bool firstMove = true;
double xPrev = 0, yPrev = 0;
void mouseCallback(GLFWwindow* window, const double xpos, const double ypos) {
    if (firstMove) {
        xPrev = xpos;
        yPrev = ypos;
        firstMove = false;
    }

    yaw += static_cast<float>((xpos - xPrev) * cameraSensitivity);
    pitch += static_cast<float>((yPrev - ypos) * cameraSensitivity);

    xPrev = xpos;
    yPrev = ypos;
}

glm::vec3 alteredCarPos;
bool carKey = false, moveCar = false, changed = false;
void processCarMovement() {
    if (pressedKeys[GLFW_KEY_E] && !carKey) {
        carKey = true;
        if (moveCar) {
            changed = true;
            moveCar = false;
            alteredCarPos = myCamera.cameraPosition;
        }
        else if (glm::distance(myCamera.cameraPosition, alteredCarPos) < 7.0f) {
            moveCar = true;
        }
    }
    else if (!pressedKeys[GLFW_KEY_E]) {
        carKey = false;
    }
    if (moveCar) {
        carCameraModel = glm::translate(glm::mat4(1.0f), glm::vec3(-0.3f, 0.45f, -0.4f));
        carCameraModel = glm::translate(carCameraModel, car.object.location);
        carCameraModel = car.modelMat * carCameraModel;
        carCameraModel = glm::translate(carCameraModel, -myCamera.cameraPosition);
        myCamera.cameraPosition = carCameraModel * glm::vec4(myCamera.cameraPosition, 1.0f);
    }
    if (changed) {
        arrow.modelMat = car.modelMat;
        changed = false;
    }
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
        if (moveCar) {
            car.modelMat = glm::translate(car.modelMat, glm::vec3(carSpeed, 0.0f, 0.0f));
        }
        else {
            if (glm::distance(myCamera.cameraPosition, alteredCarPos) > 2.5f) {
                myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
                //update view matrix
                view = myCamera.getViewMatrix();
                myBasicShader.useShaderProgram();
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                // compute normal matrix for scene
                normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            }
            
        }
	}

	if (pressedKeys[GLFW_KEY_S]) {
        if (moveCar) {
            car.modelMat = glm::translate(car.modelMat, glm::vec3(-carSpeed, 0.0f, 0.0f));
        }
        else {
            myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
            //update view matrix
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            // compute normal matrix for scene
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        }
	}

	if (pressedKeys[GLFW_KEY_A]) {
        if (moveCar) {
            if (pressedKeys[GLFW_KEY_W]) {
                car.modelMat = glm::translate(car.modelMat, car.object.location);
                car.modelMat = glm::rotate(car.modelMat, glm::radians(carRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                car.modelMat = glm::translate(car.modelMat, -car.object.location);
            }
            else if(pressedKeys[GLFW_KEY_S]) {
                car.modelMat = glm::translate(car.modelMat, car.object.location);
                car.modelMat = glm::rotate(car.modelMat, glm::radians(-carRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                car.modelMat = glm::translate(car.modelMat, -car.object.location);
            }
        }
        else {
            myCamera.move(gps::MOVE_LEFT, cameraSpeed);
            //update view matrix
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            // compute normal matrix for scene
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        }
	}

	if (pressedKeys[GLFW_KEY_D]) {
        if (moveCar) {
            if (pressedKeys[GLFW_KEY_W]) {
                car.modelMat = glm::translate(car.modelMat, car.object.location);
                car.modelMat = glm::rotate(car.modelMat, glm::radians(-carRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                car.modelMat = glm::translate(car.modelMat, -car.object.location);
            }
            else if (pressedKeys[GLFW_KEY_S]) {
                car.modelMat = glm::translate(car.modelMat, car.object.location);
                car.modelMat = glm::rotate(car.modelMat, glm::radians(carRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
                car.modelMat = glm::translate(car.modelMat, -car.object.location);
            }
        }
        else {
            myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
            //update view matrix
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            // compute normal matrix for scene
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        }
	}
    if (pressedKeys[GLFW_KEY_M]) {
        fogDensity += fogSensitivity;
        if (fogDensity > 0.3f)
            fogDensity = 0.3f;
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }
    if (pressedKeys[GLFW_KEY_N]) {
        fogDensity -= fogSensitivity;
        if (fogDensity < 0.0f)
            fogDensity = 0.0f;
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }
}

void processRotation() {
    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for scene
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

bool key_flag = false, wireframe = false;
void processPolygonMode() {
    if (pressedKeys[GLFW_KEY_L] && !key_flag) {
        key_flag = true;
        wireframe = !wireframe;
    }
    else if (!pressedKeys[GLFW_KEY_L]) {
        key_flag = false;
    }
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    scene.LoadModel("models/dust2/untitled.obj");

    car.object.LoadModel("models/car/car_ok.obj");
    carFrontDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    carRightDirection = glm::normalize(glm::cross(carFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    alteredCarPos = car.object.location;

    arrow.object.LoadModel("models/arrow/arrow.obj");
}

void initSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces);
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for scene
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    car.modelMat = glm::mat4(1.0f);
    arrow.modelMat = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for scene
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader

    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    glUniform1fv(fogDensityLoc, 1, &fogDensity);

	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderMap(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send scene model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw scene
    scene.Draw(shader);
}

void renderCar(gps::Shader shader) {
    shader.useShaderProgram();
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(car.modelMat));

    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    car.object.Draw(shader);
}

void arrowAnimation() {
    arrow.modelMat = glm::translate(arrow.modelMat, arrow.object.location);
    arrow.modelMat = glm::rotate(arrow.modelMat, glm::radians(1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    arrow.modelMat = glm::translate(arrow.modelMat, -arrow.object.location);
}

void renderArrow(gps::Shader shader) {
    arrowAnimation();
    shader.useShaderProgram();
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(arrow.modelMat));

    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    arrow.object.Draw(shader);
}

void renderSkyBox() {
    skyboxShader.useShaderProgram();
    mySkyBox.Draw(skyboxShader, view, projection);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene

	// render the scene
	renderMap(myBasicShader);
    renderCar(myBasicShader);
    if (!moveCar)
        renderArrow(myBasicShader);
    renderSkyBox();
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
    initSkyBox();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        
        processMovement();
        processCarMovement();
        processRotation();
        processPolygonMode();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
