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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


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

	//void Update(float elapsed);

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;

	float rotation;

	SheetSprite sprite;

	float health = 0;
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

#define MAX_BULLETS 30
//Entity bullets[MAX_BULLETS];
int bulletIndex = 0;
void shootBullet();

ShaderProgram program;
glm::mat4 modelMatrix = glm::mat4(1.0f);
bool done = false;
SDL_Event event;
float lastFrameTicks;
float elapsed;
float ticks;

#define NUM_BADDIES 84
//Entity Enemies[NUM_BADDIES];

//Entity Player(0.0f, -0.8f, 0.3f, 0.3f);
int sploded = 0;

GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
glm::mat4 textMatrix = glm::mat4(1.0f);

struct GameState{

	Entity player;
	Entity Enemies[NUM_BADDIES];
	Entity bullets[MAX_BULLETS];
	int sploded = 0;
};

enum GameMode {STATE_MAIN_MENU, STATE_GAME_LEVEL};

GameMode mode;
GameState state;
//I HAD something that might be called Space Invaders working
//Then I added the state and mode
//And it all broke
void Setup()
{
	mode = STATE_MAIN_MENU;
	textMatrix = glm::translate(textMatrix, glm::vec3(-0.2f, 0.0f, 0.0f));
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sprite_sheet.png");
	//playerShip1_blue;
	SheetSprite playerSprite = SheetSprite(spriteSheet, 211.0f / 1024.0f, 941.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 0.3f);
	state.player.sprite = playerSprite;
	state.player.position.x = 0.0f;
	state.player.position.y = -0.8f;
	state.player.size.x = 0.3f;
	state.player.size.y = 0.3f;
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.8f, 0.0f));

	//laserGreen02
	SheetSprite bulletSprite = SheetSprite(spriteSheet, 843.0f / 1024.0f, 116.0f / 1024.0f, 13.0f / 1024.0f, 57.0f / 1024.0f, 0.1f);
	for (int i = 0; i < MAX_BULLETS; i++) {
		state.bullets[i].position.x = -2000.0f ;
		state.bullets[i].size.x = 0.05f;
		state.bullets[i].size.y = 0.15f;
		state.bullets[i].sprite = bulletSprite;
		state.bullets[i].velocity.y = 6.5f;
	}

	//enemyRed1
	SheetSprite enemySprite = SheetSprite(spriteSheet, 425.0f / 1024.0f, 384.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
	for (int i = 0; i < NUM_BADDIES; i++)
	{
		float x_offset = 0.465f * (i % 7);
		state.Enemies[i].position.x = -1.39f + x_offset;
		float y_offset = 0.5f * (i % 12);
		state.Enemies[i].position.y = 6.6f - y_offset;
		state.Enemies[i].size.x = 0.2f;
		state.Enemies[i].size.y = 0.12f;
		state.Enemies[i].sprite = enemySprite;
		state.Enemies[i].velocity.y = -0.07f;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program.programID);


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
				mode = STATE_GAME_LEVEL;
			}
		}
	}
}

void ProcessGameLevelInput(GameState state)
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
			{
				shootBullet();
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_LEFT] && ((state.player.position.x - state.player.size.x / 2) > -1.7f)) {// 1.7 not 1.77 so we don't edge off the screen
			state.player.position.x -= elapsed * 5.0f;							//I KNOW I could do proper collision correction, but uh, I don't wanaa...
			modelMatrix = glm::translate(modelMatrix, glm::vec3(elapsed * -5.0f, 0.0f, 0.0f));

		}

		if (keys[SDL_SCANCODE_RIGHT] && ((state.player.position.x + state.player.size.x / 2) < 1.7f)) {
			state.player.position.x += elapsed * 5.0f;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(elapsed * 5.0f, 0.0f, 0.0f));
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
		ProcessGameLevelInput(state);
		break;
	}
}

void UpdateMainMenu(float elapsed)
{

}

void UpdateGameLevel(GameState state, float elapsed)
{
	for (int i = 0; i < MAX_BULLETS; i++) {
		state.bullets[i].Update(elapsed);

		for (int j = 0; j < NUM_BADDIES; j++)
		{
			if ((fabs(state.bullets[i].position.x - state.Enemies[j].position.x) - ((state.bullets[i].size.x + state.Enemies[j].size.x) / 2) < 0) &&
				(fabs(state.bullets[i].position.y - state.Enemies[j].position.y) - ((state.bullets[i].size.y + state.Enemies[j].size.y) / 2) < 0) &&
				state.Enemies[j].position.y < 1.0f)
			{
				state.bullets[i].position.x = -2000.0f;
				state.Enemies[j].position.x = 2000.0f;
				state.sploded++;
			}
		}
	}

	for (int i = 0; i < NUM_BADDIES; i++) {
		state.Enemies[i].Update(elapsed);
	}
}

void Update(float elapsed)
{
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(state, elapsed);
		break;
	}
}

void RenderMainMenu()
{
	glClearColor(0.5f, 0.0f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	program.SetModelMatrix(textMatrix);
	//"PRESS E TO START"
	std::string message = "PRESS E TO START";
	DrawText(program, fontSheet, message, 1.0f, 0.2f);
}

void RenderGameLevel(GameState state)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	program.SetModelMatrix(modelMatrix);

	state.player.Draw(program);

	for (int i = 0; i < MAX_BULLETS; i++) {
		glm::mat4 modelMatrix2 = glm::mat4(1.0f);
		modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(state.bullets[i].position.x, state.bullets[i].position.y, 0.0f));
		program.SetModelMatrix(modelMatrix2);
		state.bullets[i].Draw(program);
	}

	for (int i = 0; i < NUM_BADDIES; i++) {
		glm::mat4 modelMatrix3 = glm::mat4(1.0f);
		modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(state.Enemies[i].position.x, state.Enemies[i].position.y, 0.0f));
		program.SetModelMatrix(modelMatrix3);
		state.Enemies[i].Draw(program);
	}
}

void Render()
{
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(state);
		break;
	}
	
}


int main(int argc, char *argv[])
{
	Setup();
    while (!done) {
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		ProcessEvents();
		Update(elapsed);
		Render();

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}


void shootBullet() {
	
	state.bullets[bulletIndex].position.x = state.player.position.x;
	state.bullets[bulletIndex].position.y = state.player.position.y;
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS - 1) {
		bulletIndex = 0;
	}
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

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing)
{
	float character_size = 1.0 / 16.0f;

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

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

		glBindTexture(GL_TEXTURE_2D, fontTexture);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


	}
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

