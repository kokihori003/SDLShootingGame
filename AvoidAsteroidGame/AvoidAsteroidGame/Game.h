// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.


#pragma once
#include "SDL.h"
#include <unordered_map>
#include <string>
#include <vector>
#include "SDL_ttf.h"
#include "SDL_mixer.h"

class Game
{
public:
	Game();
	bool Initialize();
	void RunLoop();
	void Shutdown();

	void AddActor(class Actor* actor);
	void RemoveActor(class Actor* actor);

	void AddSprite(class SpriteComponent* sprite);
	void RemoveSprite(class SpriteComponent* sprite);

	SDL_Texture* GetTexture(const std::string& fileName);

	// Game-specific (add/remove asteroid)
	void AddAsteroid(class Asteroid* ast);
	void RemoveAsteroid(class Asteroid* ast);
	std::vector<class Asteroid*>& GetAsteroids() { return mAsteroids; }

	// Game-specific (add/remove RepairItem)
	void AddRepairItem(class RepairItem* rep);
	void RemoveRepairItem(class RepairItem* rep);
	std::vector<class RepairItem*>& GetRepairItems() { return mRepairItems; }

	void SpawnAsteroid();
	void SpawnRepairItem();

	//テキスト表示用の変数
	SDL_Color text_color = { 255, 255, 255, 255 }; // テキストの色 (RGBA)
	SDL_Surface* text_surface;
	SDL_Texture* text_texture; 
	TTF_Font* font;

	void RenderUI(SDL_Renderer* renderer);

	void CheckShip();

	void AddBreakAsteroidsCount() { breakAsteroidsCount++; }
	void AddGetRepairItemsCount() { getRepairItemsCount++; }

	//BGM,SE
	void PlayTitleMusic();
	void PlayGameMusic();
	void PlayResultMusic();
	void StopMusic();
	void PlayLaserSE();
	void PlayDamageSE();
	void PlayDestroySE();
	void PlayRepairSE();
	void PlayStartReload();
	void PlayEndReload();

private:
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void LoadData();
	void UnloadData();
	void ShowTitle(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight);
	void ShowExplanation(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight);
	void ShowResult(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight);
	void WaitLoop();
	void CalculateScore(float deltaTime);
	
	

	// Map of textures loaded
	std::unordered_map<std::string, SDL_Texture*> mTextures;

	// All the actors in the game
	std::vector<class Actor*> mActors;
	// Any pending actors
	std::vector<class Actor*> mPendingActors;

	// All the sprite components drawn
	std::vector<class SpriteComponent*> mSprites;

	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	Uint32 mTicksCount;
	bool mIsRunning;
	// Track if we're updating actors right now
	bool mUpdatingActors;

	// Game-specific
	class Ship* mShip; // Player's ship
	std::vector<class Asteroid*> mAsteroids;
	std::vector<class RepairItem*> mRepairItems;

	//オブジェクト生成間隔
	float spawnAsteroidInterval = 0.5f;
	float spawnRepairItemInterval = 5.0f;
	//カウント変数
	float spawnAsteroidSpeed = 0.0f;
	float spawnRepairItemSpeed = 0.0f;

	int windowWidth=0;
	int windowHeight=0;

	float score = 0.0f;
	float gameTime = 0.0f;

	float breakAsteroidsCount = 0.0f;
	float getRepairItemsCount = 0.0f;
	float asteroidPoint = 1500.0f;
	float repairItemPoint = 3000.0f;

	float tenScecond = 0.0f;
	Mix_Chunk* soundEffectLaser = nullptr;
	Mix_Chunk* soundEffectDestroy = nullptr;
	Mix_Chunk* soundEffectDamage = nullptr;
	Mix_Chunk* soundEffectRepair = nullptr;
	Mix_Chunk* soundEffectSReload = nullptr;
	Mix_Chunk* soundEffectEReload = nullptr;
	Mix_Music* music = nullptr;
};