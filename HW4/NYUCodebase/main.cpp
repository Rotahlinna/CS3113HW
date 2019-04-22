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
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;


SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath);

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing);

class SheetSprite { //Reuse complex sprite stuff even though it's an evenly spaced spritesheet now
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


enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN};
#define FRICTION 0.2
#define GRAVITY 0.35
float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}
class Entity {
public:

	Entity() {
		position.x = 0;
		position.y = 0;
		size.x = 1.0f;
		size.y = 1.0f;
	}

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;
	glm::vec3 acceleration;

	//float rotation;

	SheetSprite sprite;

	bool isStatic;
	EntityType entityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
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

	void CollidesX()
	{

	}

	void CollidesY()
	{

	}

	bool CollidesWith(Entity &entity) //I'll set flags in the Process phase
	{
		if (collidedTop && entity.collidedBottom)
		{
			return true;
		}
		else if (collidedBottom && entity.collidedTop)
		{
			return true;
		}
		else if (collidedRight && entity.collidedLeft)
		{
			return true;
		}
		else if (collidedLeft && entity.collidedRight)
		{
			return true;
		}
		else 
		{
			return false;
		}
	}

	void Update(float elapsed)
	{
		velocity.x = lerp(velocity.x, 0.0f, elapsed * FRICTION);
		velocity.x += acceleration.x * elapsed;
		velocity.y += acceleration.y * elapsed; 
		velocity.y -= GRAVITY * elapsed;
		position.x += velocity.x * elapsed;
		position.y += velocity.y * elapsed;
	}
};


ShaderProgram program;
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
bool done = false;
SDL_Event event;
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float lastFrameTicks;
float elapsed;
float ticks;

void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY, unsigned int textureID);

#define TILE_SIZE 16.0f
#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 128
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
int mapWidth;
int mapHeight;
unsigned char** levelData;
bool readEntityData(ifstream &stream);
bool readLayerData(ifstream &stream);
bool readHeader(ifstream &stream);


GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");//Oh God the legs are a separate sprite...and feet?  256 x 128
Entity player = Entity(0, 0, 0.2f, 0.2f);
vector<Entity> Enemies;
SheetSprite enemySprite = SheetSprite(spriteSheet, 16 / 256, 96 / 128, 16 / 256, 16 / 128, 0.2f);
vector<Entity> Tiles;
Entity aTile;

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
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program.programID);

	lastFrameTicks = 0.0f;

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	SheetSprite playerSprite = SheetSprite(spriteSheet, 48.0f / 256.0f, 112.0f / 128.0f, 16.0f / 256.0f, 16.0f / 128.0f, 0.2f);
	player.sprite = playerSprite;



	SheetSprite tileSprite = SheetSprite(spriteSheet, 16.0f / 256.0f, 16.0f / 128.0f, 32.0f / 256.0f, 32.0f / 128.0f, 0.2f);
	aTile.sprite = tileSprite;
	aTile.position.x = 0;
	aTile.position.y = 0;

	//Read through Flare map data
	ifstream infile("mymap.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile);
		}
	}

	//place tiles
	int curr_Tile = 0;
	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			if (levelData[y][x] != 0)
			{
				Tiles.push_back(Entity(x / (mapWidth*1.777), y / mapHeight, 0.2f, 0.2f));
				//Tiles[curr_Tile].sprite = tileSprite;
				curr_Tile++;
			}
		}
	}
}


void ProcessEvents()
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		
	}
}



void Update(float elapsed)
{

	for (int i = 0; i < Enemies.size(); i++) {
		Enemies[i].Update(elapsed);
	}

	aTile.velocity.x = 0;
	aTile.velocity.y = 0;

}




void Render()
{
	 glClear(GL_COLOR_BUFFER_BIT);

	//modelMatrix = glm::translate(modelMatrix, glm::vec3(player.position.x, player.position.y, 0.0f));
	program.SetModelMatrix(modelMatrix);

	player.Draw(program);

/*
	for (int i = 0; i < Enemies.size(); i++) {
		glm::mat4 modelMatrix3 = glm::mat4(1.0f);
		modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(Enemies[i].position.x, Enemies[i].position.y, 0.0f));
		program.SetModelMatrix(modelMatrix3);
		Enemies[i].Draw(program);
	}

	glm::mat4 modelMatrixqq = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrixqq);
	aTile.Draw(program);

	glClearColor(0.5f, 0.25f, 0.25f, 1.0f);

	vector<float> vertexData;
	vector<float> texCoordData;
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
					});
				texCoordData.insert(texCoordData.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),
				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
					});
			}
		}
	}

	glm::mat4 modelMatrix4 = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix4);
	glBindTexture(GL_TEXTURE_2D, spriteSheet);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, (int)((vertexData.size()/2) * 6));

	glDisableVertexAttribArray(program.texCoordAttribute);
	glDisableVertexAttribArray(program.positionAttribute);

	*/
}



int main(int argc, char *argv[])
{
	Setup();
	float accumulator = 0.0f;
    while (!done) {
		ProcessEvents(); //slides don't show where to put the process phase with fixed timestep but I assume it needs to be before Update
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
    
    SDL_Quit();
    return 0;
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

void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY, unsigned int textureID) 
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	float texCoords[] = {
	u, v + spriteHeight,
	u + spriteWidth, v,
	u, v,
	u + spriteWidth, v,
	u, v + spriteHeight,
	u + spriteWidth, v + spriteHeight
	};
	float vertices[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
	-0.5f, 0.5f, -0.5f };
	
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.texCoordAttribute);
	glDisableVertexAttribArray(program.positionAttribute);
}

bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; } 

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);

		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}

	if(mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else {
		levelData = new unsigned char*[mapHeight];
		for (int i = 0; i < mapHeight; i++) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}

bool readLayerData(ifstream &stream)
{
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;

				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

int curr_enemy = 0;
void placeEntity(string type, float myX, float myY)
{
	if (type == "Player") {
		player.position.x = myX;
		player.position.y = myY;
	}
	else if (type == "Enemy") {
		Enemies.push_back(Entity());
		Enemies[curr_enemy].position.x = myX;
		Enemies[curr_enemy].position.y = myY;
		Enemies[curr_enemy].sprite = enemySprite;
		curr_enemy++;
	}
}

bool readEntityData(ifstream &stream) {
	string line;
	string type;

	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);

		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');

			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;


			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

