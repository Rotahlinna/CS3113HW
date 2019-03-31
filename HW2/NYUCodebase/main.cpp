#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Entity {
public:
	float x;
	float y;
	float rotation;

	int textureID;

	float width;
	float height;

	float velocity;
	float direction_x;
	float direction_y;

	Entity(float myX, float myY, float myWidth, float myHeight)
	{
		x = myX;
		y = myY;
		width = myWidth;
		height = myHeight;
	}

	void Draw(ShaderProgram &p)
	{
		float vertices[] = { (float)(x + width / 2), (float)(y - height / 2), (float)(x - width / 2), (float)(y + height / 2), (float)(x - width / 2),
			(float)(y - height / 2), (float)(x + width / 2), (float)(y - height / 2), (float)(x + width / 2), (float)(y + height / 2),
			(float)(x - width / 2), (float)(y + height / 2) };//square made of 2 triangles...I think the (float) casts were unnecessary...

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
	}
};

ShaderProgram program;
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 modelMatrix2 = glm::mat4(1.0f);
glm::mat4 modelMatrix3 = glm::mat4(1.0f);
bool done = false;
SDL_Event event;
float lastFrameTicks;
float elapsed;
float ticks;
Entity paddle1(0.0f, 0.0f, 0.3f, 1.0f);
Entity paddle2(0.0f, 0.0f, 0.3f, 1.0f);
Entity ball(0.0f, 0.0f, 0.1f, 0.1f);
void Setup()
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	//modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.3f, 0.0f, 0.0f));
	paddle1.x = -1.3f;

	//modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(1.3f, 0.0f, 0.0f)); Hmmm, I feel like I'm still supposed to be using matrix transformations...
	paddle2.x = 1.3f;

	//modelMatrix3 = glm::scale(modelMatrix3, glm::vec3(0.3f, 0.3f, 1.0f));


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program.programID);

	lastFrameTicks = 0.0f;
	ball.direction_x = 1.0f;
	ball.direction_y = 1.0f;

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
}

void ProcessEvents()
{
	
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_W] && (paddle1.y + (paddle1.height/2)) < 1.0f) {
			//modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, elapsed * 2.0f, 0.0f));
			paddle1.y += elapsed * 2.0f;
		}

		if (keys[SDL_SCANCODE_S] && (paddle1.y - (paddle1.height/2)) > -1.0f) {
			paddle1.y -= elapsed * 2.0f;
		}

		if (keys[SDL_SCANCODE_UP] && (paddle2.y + (paddle2.height / 2)) < 1.0f) {
			paddle2.y += elapsed * 2.0f;
		}

		if (keys[SDL_SCANCODE_DOWN] && (paddle2.y - (paddle2.height / 2)) > -1.0f) {
			paddle2.y -= elapsed * 2.0f;
		}

		if (keys[SDL_SCANCODE_E]) {
			ball.direction_x *= -1.0f;
		}
	}
}

void Update()
{
	ball.x += ball.direction_x * elapsed * 0.3f;
	ball.y += ball.direction_y * elapsed * 0.3f;

	if (ball.y + (ball.height / 2) > 1.0f || ball.y - (ball.height / 2) < -1.0f) {
		ball.direction_y *= -1.0f;
	}

	if (((abs(paddle1.x - ball.x) - (paddle1.width + ball.width) / 2) < 0) && ((abs(paddle1.y - ball.y) - (paddle1.height + ball.height) / 2) < 0)) {
		ball.direction_x *= -1.0f;
	}

	if (((abs(paddle2.x - ball.x) - (paddle2.width + ball.width) / 2) < 0) && ((abs(paddle2.y - ball.y) - (paddle2.height + ball.height) / 2) < 0)) {
		ball.direction_x *= -1.0f;
	}//If you go at the ball after it's past the "front" of a paddle you can get the ball stuck inside the paddle

	if (abs(ball.x) > 1.8f) {
		glClearColor(0.5f, 0.7f, 0.2f, 1.0f); //Point!
	}

}

void Render()
{
	glClear(GL_COLOR_BUFFER_BIT);

	program.SetModelMatrix(modelMatrix);
	paddle1.Draw(program);

	program.SetModelMatrix(modelMatrix2);
	paddle2.Draw(program);
	program.SetModelMatrix(modelMatrix3);
	ball.Draw(program);
}

int main(int argc, char *argv[])
{
	Setup();
    while (!done) {
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		ProcessEvents();
		Update();
		Render();

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

