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
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <SDL_mixer.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
using namespace std;


SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath);

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing);

class SheetSprite {
public:
	SheetSprite()
	{
		size = 0.0f;
		textureID = 0;
		u = 0.0f;
		v = 0.0f;
		width = 0;
		height = 0;
	}
	SheetSprite(unsigned int mytextureID, float myu, float myv, float mywidth, float myheight, float mysize)
	{
		textureID = mytextureID;
		u = myu;
		v = myv;
		width = mywidth;
		height = myheight;
		size = mysize;
	}

	void Draw(ShaderProgram &program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};



class Entity {
public:
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;

	float rotation = 0.0f;

	SheetSprite sprite;

	Entity()
	{
		position.x = 0;
		position.y = 0;
	}
	Entity(float myX, float myY, float myWidth, float myHeight)
	{
		position.x = myX;
		position.y = myY;
		size.x = myWidth;
		size.y = myHeight;
	}

	void Draw(ShaderProgram &p)
	{
		sprite.Draw(p);
	}

	void Update(float elapsed)
	{
		position.x += velocity.x * elapsed;
		position.y += velocity.y * elapsed;
	}
};

ShaderProgram program;
glm::mat4 viewMatrix = glm::mat4(1.0f);
bool done = false;
SDL_Event event;
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float lastFrameTicks;
float elapsed;
float ticks;


enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER , STATE_WON};

GameMode mode;
glm::mat4 textMatrix = glm::mat4(1.0f);
GLuint fontSheet;
GLuint spriteSheet;
Mix_Music *menuTheme;
Mix_Music *mainTheme;
Mix_Music *gameOver;
Mix_Chunk *playerLaser;

#define MAX_BULLETS 300
Entity bullets[MAX_BULLETS];
int bulletIndex = 0;
void shootBullet(string direction);


Entity Player;
Entity badGuy[10];
Entity Goal;

Entity particles[20]; //I'm doing this in a bad / hacky way, I know
void explode(float x, float y);

Entity gameOverMan;

int currLvl = 1;
void setupLvl1();
void setupLvl2();
void setupLvl3();

float timer = 0.0f;
bool shot = false;

void Setup()
{
	mode = STATE_MAIN_MENU;
	textMatrix = glm::translate(textMatrix, glm::vec3(-0.7f, 0.0f, 0.0f));
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
	glViewport(0, 0, 1280, 720);
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float liftime = rand();

	glUseProgram(program.programID);

	GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");

	menuTheme = Mix_LoadMUS("1-05 Firelink Shrine.mp3");
	mainTheme = Mix_LoadMUS("MegaMan2.mp3");
	gameOver = Mix_LoadMUS("gameOver.mp3");

	playerLaser = Mix_LoadWAV("laser04.wav");

	Mix_OpenAudio(24100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_VolumeMusic(0);
	//Mix_VolumeChunk(playerLaser, 15);
	Mix_PlayMusic(menuTheme, -1);

	GLuint deathScreen = LoadTexture(RESOURCE_FOLDER"youdied.jpg");
	SheetSprite ded = SheetSprite(deathScreen, 1.0f, 1.0f, 1.0f, 1.0f, 3.0f);
	gameOverMan.sprite = ded;

	GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sprite_sheet.png"); // 1024 x 1024
	SheetSprite playerSprite = SheetSprite(spriteSheet, 434.0f / 1024.0f, 234.0f / 1024.0f, 91.0f / 1024.0f, 91.0f / 1024.0f, 0.2f);
	Player.sprite = playerSprite;
	Player.position.x = 0.2f;
	Player.position.y = -0.4f;
	Player.size.x, Player.size.y = 0.2f;

	SheetSprite starSprite = SheetSprite(spriteSheet, 778.0f / 1024.0f, 557.0f / 1024.0f, 31.0f / 1024.0f, 30.0f / 1024.0f, 0.45f);
	Goal.sprite = starSprite;
	Goal.position.x = 2.9f;
	Goal.position.y = 0.75f;

	SheetSprite enemySprite = SheetSprite(spriteSheet, 505.0f / 1024.0f, 898.0f / 1024.0f, 91.0f / 1024.0f, 91.0f / 1024.0f, 0.2f);
	for (int i = 0; i < 10; i++)
	{
		badGuy[i].sprite = enemySprite;
	}
	badGuy[0].position.x = Goal.position.x - 0.7f;
	badGuy[0].position.y = Goal.position.y - 0.65f;
	badGuy[1].position.x = Goal.position.x - 0.7f;
	badGuy[1].position.y = Goal.position.y + 0.65;
	badGuy[2].position.x = Goal.position.x + 0.7f;
	badGuy[2].position.y = Goal.position.y - 0.65f;
	badGuy[3].position.x = Goal.position.x + 0.7f;
	badGuy[3].position.y = Goal.position.y + 0.65f;
	for (int i = 4; i < 10; i++)
	{
		badGuy[i].position.x = 2000.0f;
	}

	SheetSprite particleSprite = SheetSprite(spriteSheet, 738.0f / 1024.0f, 650.0f / 1024.0f, 37.0f / 1024.0f, 36.0f / 1024.0f, 0.04f);
	for (int i = 0; i < 20; i++)
	{
		particles[i].sprite = particleSprite;
		particles[i].position.y = 1000.0f;
	}

	SheetSprite bulletSprite = SheetSprite(spriteSheet, 843.0f / 1024.0f, 116.0f / 1024.0f, 13.0f / 1024.0f, 57.0f / 1024.0f, 0.1f);
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].position.x = -2000.0f;
		bullets[i].size.x = 0.05f;
		bullets[i].size.y = 0.15f;
		bullets[i].sprite = bulletSprite;
	}



	lastFrameTicks = 0.0f;
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
}

void ProcessMainMenuInput()
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == SDL_SCANCODE_E)
			{
				Mix_HaltMusic();
				Mix_PlayMusic(mainTheme, -1);
				mode = STATE_GAME_LEVEL;
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				done = true;
			}
		}
	}
}

void ProcessGameLevelInput()
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				done = true;
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_UP)
			{
				shootBullet("up");
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				shootBullet("down");
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
			{
				shootBullet("left");
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
			{
				shootBullet("right");
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_A] && Player.position.x > 0.1f)
		{
			Player.velocity.x = -0.5f;
		}
		else if (keys[SDL_SCANCODE_D] && Player.position.x < 7.5f)
		{
			Player.velocity.x = 0.5f;
		}
		else if (keys[SDL_SCANCODE_S] && Player.position.y > -2.0f)
		{
			Player.velocity.y = -0.5f;
		}
		else if (keys[SDL_SCANCODE_W] && Player.position.y < 7.0f)
		{
			Player.velocity.y = 0.5f;
		}
		else
		{
			Player.velocity.x = 0.0f;
			Player.velocity.y = 0.0f;
		}

	}
}

void ProcessGameOverInput()
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				done = true;
			}
			
			if (event.key.keysym.scancode == SDL_SCANCODE_E)
			{
				Mix_PlayMusic(mainTheme, -1);
				if (currLvl == 1)
				{
					setupLvl1();
					mode = STATE_GAME_LEVEL;
				}
				else if (currLvl == 2)
				{
					setupLvl2();
					mode = STATE_GAME_LEVEL;
				}
				else
				{
					setupLvl3();
					mode = STATE_GAME_LEVEL;
				}
			}
		}
	}
}

void ProcessWonInput()
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				done = true;
			}
		}
	}
}

void ProcessEvents()
{
	switch (mode) {
	case STATE_MAIN_MENU:
		ProcessMainMenuInput();
		break;
	case STATE_GAME_LEVEL:
		ProcessGameLevelInput();
		break;
	case STATE_GAME_OVER:
		ProcessGameOverInput();
		break;
	case STATE_WON:
		ProcessWonInput();
		break;
	}
}

void UpdateMainMenu(float elapsed)
{
	
}

void UpdateGameLevel1(float elapsed)
{
	for (int i = 0; i < 10; i++)
	{
		if (glm::sqrt(glm::pow((badGuy[i].position.x - Player.position.x), 2) + glm::pow((badGuy[i].position.y - Player.position.y), 2)) < 0.85f)
		{
			if (Player.position.x < badGuy[i].position.x)
			{
				badGuy[i].velocity.x = -0.36f;
			}
			else if (Player.position.x > badGuy[i].position.x)
			{
				badGuy[i].velocity.x = 0.36f;
			}
			else
			{
				badGuy[i].velocity.x = 0.0f;
			}

			if (Player.position.y < badGuy[i].position.y)
			{
				badGuy[i].velocity.y = -0.36f;
			}
			else if (Player.position.y > badGuy[i].position.y)
			{
				badGuy[i].velocity.y = 0.36f;
			}
			else
			{
				badGuy[i].velocity.y = 0.0f;
			}
		}
		else
		{
			badGuy[i].velocity.x = 0.0f;
			badGuy[i].velocity.y = 0.0f;
		}

		if (glm::sqrt(glm::pow((badGuy[i].position.x - Player.position.x), 2) + glm::pow((badGuy[i].position.y - Player.position.y), 2)) < 0.15f)
		{
			Mix_PlayMusic(gameOver, 0);
			Player.velocity.x = 0.0f;
			Player.velocity.y = 0.0f;
			mode = STATE_GAME_OVER;
		}
		for (int j = 0; j < MAX_BULLETS; j++)
		{
			if (glm::sqrt(glm::pow((badGuy[i].position.x - bullets[j].position.x), 2) + glm::pow((badGuy[i].position.y - bullets[j].position.y), 2)) < 0.15f)
			{
				explode(badGuy[i].position.x, badGuy[i].position.y);
				badGuy[i].position.x = 2000.0f;
				bullets[j].position.x = -2000.0f;
			}
		}
		badGuy[i].Update(elapsed);
	}

	Player.Update(elapsed);
	

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		bullets[i].Update(elapsed);
	}

	for (int i = 0; i < 20; i++)
	{
		particles[i].Update(elapsed);
	}

	if (shot)
	{
		timer += elapsed;
	}

	if (timer > 0.8f) 
	{
		for (int i = 0; i < 20; i++)
		{
			particles[i].position.y = 1000.0f;
		}
		shot = false;
		timer = 0.0f;
	}

	if (glm::sqrt(glm::pow((Player.position.x - Goal.position.x), 2) + glm::pow((Player.position.y - Goal.position.y), 2)) < 0.3f)
	{
		if (currLvl == 1)
		{
			currLvl++;
			setupLvl2();
		}
		else if (currLvl == 2)
		{
			currLvl++;
			setupLvl3();
		}
		else
		{
			mode = STATE_WON;
		}
	}

}

void UpdateGameOver(float elapsed)
{

}

void UpdateWon()
{

}

void Update(float elapsed)
{
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel1(elapsed);
		break;
	case STATE_GAME_OVER:
		UpdateGameOver(elapsed);
		break;
	case STATE_WON:
		UpdateWon();
		break;
	}
}

void RenderMainMenu()
{
	glClearColor(0.5f, 0.0f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	program.SetModelMatrix(textMatrix);
	string message = "PRESS E TO START";
	DrawText(program, fontSheet, message, 0.08f, 0.01f);
}

void RenderGameLevel()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 playerMatrix = glm::mat4(1.0f);
	playerMatrix = glm::translate(playerMatrix, glm::vec3(Player.position.x, Player.position.y, 0.0f));
	program.SetModelMatrix(playerMatrix);
	Player.Draw(program);

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		glm::mat4 bulletMatrix = glm::mat4(1.0f);
		bulletMatrix = glm::translate(bulletMatrix, glm::vec3(bullets[i].position.x, bullets[i].position.y, 0.0f));
		float angle = bullets[i].rotation * (3.1415926f / 180.0f);
		bulletMatrix = glm::rotate(bulletMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		program.SetModelMatrix(bulletMatrix);
		bullets[i].Draw(program);
	}

	for (int i = 0; i < 10; i++)
	{
		glm::mat4 enemyMatrix = glm::mat4(1.0f);
		enemyMatrix = glm::translate(enemyMatrix, glm::vec3(badGuy[i].position.x, badGuy[i].position.y, 0.0f));
		program.SetModelMatrix(enemyMatrix);
		badGuy[i].Draw(program);
	}

	glm::mat4 goalMatrix = glm::mat4(1.0f);
	goalMatrix = glm::translate(goalMatrix, glm::vec3(Goal.position.x, Goal.position.y, 0.0f));
	program.SetModelMatrix(goalMatrix);
	Goal.Draw(program);

	for (int i = 0; i < 20; i++)
	{
		glm::mat4 partMat = glm::mat4(1.0f);
		partMat = glm::translate(partMat, glm::vec3(particles[i].position.x, particles[i].position.y, 0.0f));
		program.SetModelMatrix(partMat);
		particles[i].Draw(program);
	}
	

	float newX = glm::min(-Player.position.x, -1.777f);
	float newY = glm::min(-Player.position.y, 1.0f);
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(newX, newY, 0.0f));
	program.SetViewMatrix(viewMatrix);
	
}


void RenderGameOver()
{
	glClearColor(0.0f, 0.25f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glm::mat4 deathMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(deathMatrix);
	viewMatrix = glm::mat4(1.0f);
	program.SetViewMatrix(viewMatrix);
	gameOverMan.Draw(program);

}

void RenderWon()
{
	glClearColor(0.25f, 0.7f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	viewMatrix = glm::mat4(1.0f);
	glm::mat4 Star = glm::mat4(1.0f);
	program.SetViewMatrix(viewMatrix);
	program.SetModelMatrix(Star);
	Goal.Draw(program);
}


void Render()
{
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	case STATE_GAME_OVER:
		RenderGameOver();
		break;
	case STATE_WON:
		RenderWon();
		break;
	}
	
}

int main(int argc, char *argv[])
{
	Setup();
	float accumulator = 0.0f;
	while (!done) {
		ProcessEvents(); 
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}

		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		Render();

		SDL_GL_SwapWindow(displayWindow);
	}

	Mix_FreeMusic(menuTheme);
	Mix_FreeMusic(mainTheme);
	Mix_FreeMusic(gameOver);
	SDL_Quit();
	return 0;
}

void shootBullet(string direction) 
{
	Mix_PlayChannel(-1, playerLaser, 0);

	bullets[bulletIndex].position.x = Player.position.x;
	bullets[bulletIndex].position.y = Player.position.y;
	if (direction == "up")
	{
		bullets[bulletIndex].velocity.x = 0.0f;
		bullets[bulletIndex].velocity.y = 6.5f;
	}
	else if (direction == "down")
	{
		bullets[bulletIndex].velocity.x = 0.0f;
		bullets[bulletIndex].velocity.y = -6.5f;
	}
	else if (direction == "left")
	{
		bullets[bulletIndex].velocity.y = 0.0f;
		bullets[bulletIndex].velocity.x = -6.5f;
		if(bullets[bulletIndex].rotation == 0.0f)
			bullets[bulletIndex].rotation = 90.0f;
	}
	else
	{
		bullets[bulletIndex].velocity.y = 0.0f;
		bullets[bulletIndex].velocity.x = 6.5f;
		if(bullets[bulletIndex].rotation == 0.0f)
			bullets[bulletIndex].rotation = 90.0f;
	}
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS - 1) {
		bulletIndex = 0;
	}
}

void explode(float x, float y)
{
	shot = true;
	for (int i = 0; i < 20; i++)
	{
		int randNum = (rand() % 6) - (rand() % 6);
		particles[i].position.x = x + randNum * 0.02f;
		particles[i].position.y = y + randNum * 0.02f;
		particles[i].velocity.x = randNum;
		particles[i].velocity.y = randNum;
	}
}

void setupLvl1()
{
	Player.position.x = 0.2f;
	Player.position.y = -0.4f;
	badGuy[0].position.x = Goal.position.x - 0.7f;
	badGuy[0].position.y = Goal.position.y - 0.65f;
	badGuy[1].position.x = Goal.position.x - 0.7f;
	badGuy[1].position.y = Goal.position.y + 0.65;
	badGuy[2].position.x = Goal.position.x + 0.7f;
	badGuy[2].position.y = Goal.position.y - 0.65f;
	badGuy[3].position.x = Goal.position.x + 0.7f;
	badGuy[3].position.y = Goal.position.y + 0.65f;
	Goal.position.x = 2.9f;
	Goal.position.y = 0.75f;
}

void setupLvl2()
{
	Player.position.x = 0.2f;
	Player.position.y = -0.4f;
	Goal.position.x = 3.2f;
	Goal.position.y = 0.75f;
	badGuy[0].position.x = 3.3f;
	badGuy[0].position.y = 1.15f;
	badGuy[1].position.x = 2.0f;
	badGuy[1].position.y = 0.6f;
	badGuy[2].position.x = 2.7f;
	badGuy[2].position.y = 1.15f;
	badGuy[3].position.x = 2.2f;
	badGuy[3].position.y = 0.35f;
	badGuy[4].position.x = 1.8f;
	badGuy[4].position.y = 0.4f;
	badGuy[5].position.x = 2.8f;
	badGuy[5].position.y = 0.2f;
	badGuy[6].position.x = 4.2f;
	badGuy[6].position.y = 0.8f;
	badGuy[7].position.x = 4.15;
	badGuy[7].position.y = 0.3f;
}

void setupLvl3()
{
	Player.position.x = 2.5f;
	Player.position.y = -0.4f;
	Goal.position.x = 2.5f;
	Goal.position.y = 3.0f;
	badGuy[0].position.x = 2.0f;
	badGuy[0].position.y = 2.7f;
	badGuy[1].position.x = 2.8f;
	badGuy[1].position.y = 2.7f;
	badGuy[2].position.x = 1.8f;
	badGuy[2].position.y = 2.3f;
	badGuy[3].position.x = 3.2f;
	badGuy[3].position.y = 2.3f;
	badGuy[4].position.x = 1.5f;
	badGuy[4].position.y = 2.0f;
	badGuy[5].position.x = 3.5f;
	badGuy[5].position.y = 2.0f;
	badGuy[6].position.x = 1.2f;
	badGuy[6].position.y = 1.5f;
	badGuy[7].position.x = 3.8f;
	badGuy[8].position.x = 1.0f;
	badGuy[8].position.y = 1.0f;
	badGuy[9].position.x = 3.0f;
	badGuy[9].position.y = 1.0f;
}

void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};

	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.texCoordAttribute);
	glDisableVertexAttribArray(program.positionAttribute);

}

void DrawText(ShaderProgram &program, int fontTexture, string text, float size, float spacing)
{
	float character_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
		texture_x, texture_y,
		texture_x, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x + character_size, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x, texture_y + character_size,
			});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);


	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.length() * 6); //6 vertices per character
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}
