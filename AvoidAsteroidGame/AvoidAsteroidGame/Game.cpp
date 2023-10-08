// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.


#include "Game.h"
#include "SDL_image.h"
#include <algorithm>
#include "Actor.h"
#include "SpriteComponent.h"
#include "Ship.h"
#include "Asteroid.h"
#include "RepairItem.h"
#include "Random.h"
#include "BGSpriteComponent.h"
#include "SDL_mixer.h"

#define SE_LASER_PATH "Audio/Laser.mp3"
#define SE_DAMAGE_PATH "Audio/Damage.mp3"
#define SE_DESTROY_PATH "Audio/Destroy.mp3"
#define SE_REPAIR_PATH "Audio/Repair.mp3"
#define SE_STARTRELOAD_PATH "Audio/StartReload.mp3"
#define SE_ENDRELOAD_PATH "Audio/EndReload.mp3"
#define MUS_TITLE_PATH "Audio/Title.ogg"
#define MUS_GAME_PATH "Audio/GameBGM.ogg"
#define MUS_RESULT_PATH "Audio/Result.ogg"
#define FONT_PATH "PixelMplus/PixelMplus12-Regular.ttf"

Game::Game()
	:mWindow(nullptr)
	, mRenderer(nullptr)
	, mIsRunning(true)
	, mUpdatingActors(false)
{

}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}
	windowWidth = 1024;
	windowHeight = 768;
	mWindow = SDL_CreateWindow("Game Programming in C++ (Chapter 3)", 100, 100, windowWidth, windowHeight, 0);
	if (!mWindow)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!mRenderer)
	{
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		return false;
	}

	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		SDL_Log("Unable to initialize SDL_image: %s", SDL_GetError());
		return false;
	}

	if (TTF_Init() < 0) 
	{
		SDL_Log("Unable to initialize SDL_TTF: %s", SDL_GetError());
		return false;
	}
	else
	{
		font = TTF_OpenFont(FONT_PATH, 40);
	}

	// Initialize SDL_mixer 
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
	{
		SDL_Log("Unable to initialize SDL_mixer: %s", SDL_GetError());
		return false;
	}

	//タイトル画面になる画像や文字などを表示し、
	// 特定の入力がなされるまで次の処理を待つコード
	// 音楽を再生
	PlayTitleMusic();
	ShowTitle(mRenderer, font, windowWidth, windowHeight);
	WaitLoop();
	font = TTF_OpenFont(FONT_PATH, 30); // 24はフォントのサイズ
	ShowExplanation(mRenderer, font, windowWidth, windowHeight);
	WaitLoop();
	// BGMの解放
	StopMusic();

	Random::Init();
	LoadData();
	mTicksCount = SDL_GetTicks();
	return true;
}

void Game::RunLoop()
{
	PlayGameMusic();
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
	StopMusic();
}

void Game::CheckShip()
{
	//Shipのデストラクタが呼び出されると呼び出し
	//ゲーム終了処理
	mIsRunning = false;
}

void Game::ProcessInput()
{
	if (mShip->PermitInput())
	{
		mUpdatingActors = true;
		for (auto actor : mActors)
		{
			actor->ProcessInput(nullptr);
		}
		return;
	}
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			mIsRunning = false;
			break;
		}
	}

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_ESCAPE])
	{
		mIsRunning = false;
	}
	
	mUpdatingActors = true;
	for (auto actor : mActors)
	{
		actor->ProcessInput(keyState);
	}
	mUpdatingActors = false;
}

void Game::UpdateGame()
{
	// Compute delta time
	// Wait until 16ms has elapsed since last frame
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), mTicksCount + 16))
		;

	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	if (deltaTime > 0.05f)
	{
		deltaTime = 0.05f;
	}
	mTicksCount = SDL_GetTicks();
	spawnAsteroidSpeed += deltaTime;
	// 一定時間ごとにAsteroidを生成する
	if (spawnAsteroidSpeed >= spawnAsteroidInterval)
	{
		SpawnAsteroid();
		spawnAsteroidSpeed = 0.0f;
	}

	spawnRepairItemSpeed += deltaTime;
	// 一定時間ごとにAsteroidを生成する
	if (spawnRepairItemSpeed >= spawnRepairItemInterval)
	{
		SpawnRepairItem();
		spawnRepairItemSpeed = 0.0f;
	}

	// Update all actors
	mUpdatingActors = true;
	for (auto actor : mActors)
	{
		actor->Update(deltaTime);
	}
	mUpdatingActors = false;

	// Move any pending actors to mActors
	for (auto pending : mPendingActors)
	{
		mActors.emplace_back(pending);
	}
	mPendingActors.clear();

	CalculateScore(deltaTime);

	// Add any dead actors to a temp vector
	std::vector<Actor*> deadActors;
	for (auto actor : mActors)
	{
		if (actor->GetState() == Actor::EDead)
		{
			deadActors.emplace_back(actor);
		}
	}

	// Delete dead actors (which removes them from mActors)
	for (auto actor : deadActors)
	{
		delete actor;
	}
}

void Game::GenerateOutput()
{
	SDL_SetRenderDrawColor(mRenderer, 220, 220, 220, 255);
	SDL_RenderClear(mRenderer);

	// Draw all sprite components
	for (auto sprite : mSprites)
	{
		sprite->Draw(mRenderer);
	}

	//render ui
	RenderUI(mRenderer);

	SDL_RenderPresent(mRenderer);

	
}

void Game::LoadData()
{
	// Create player's ship
	mShip = new Ship(this);
	mShip->SetPosition(Vector2(100.0f, 384.0f));
	mShip->SetScale(1.2f);

	//Create actor for the background (this dosen't need a subclass)
	Actor* temp = new Actor(this);
	temp->SetPosition(Vector2(512.0f, 384.0f));
	//Create the "far back" background
	BGSpriteComponent* bg = new BGSpriteComponent(temp);
	bg->SetScreenSize(Vector2(1024.0f, 768.0f));
	std::vector<SDL_Texture*> bgtexs = {
		GetTexture("Assets/Farback01.png"),
		GetTexture("Assets/Farback02.png")
	};
	bg->SetBGTextures(bgtexs);
	bg->SetScrollSpeed(-100.0f);

	//create the closer background
	bg = new BGSpriteComponent(temp, 50);
	bg->SetScreenSize(Vector2(1024.0f, 768.0f));
	bgtexs = {
		GetTexture("Assets/Stars.png"),
		GetTexture("Assets/Stars.png")
	};
	bg->SetBGTextures(bgtexs);
	bg->SetScrollSpeed(-200.0f);
}

void Game::UnloadData()
{
	// Delete actors
	// Because ~Actor calls RemoveActor, have to use a different style loop
	while (!mActors.empty())
	{
		delete mActors.back();
	}

	// Destroy textures
	for (auto i : mTextures)
	{
		SDL_DestroyTexture(i.second);
	}
	mTextures.clear();
}

SDL_Texture* Game::GetTexture(const std::string& fileName)
{
	SDL_Texture* tex = nullptr;
	// Is the texture already in the map?
	auto iter = mTextures.find(fileName);
	if (iter != mTextures.end())
	{
		tex = iter->second;
	}
	else
	{
		// Load from file
		SDL_Surface* surf = IMG_Load(fileName.c_str());
		if (!surf)
		{
			SDL_Log("Failed to load texture file %s", fileName.c_str());
			return nullptr;
		}

		// Create texture from surface
		tex = SDL_CreateTextureFromSurface(mRenderer, surf);
		SDL_FreeSurface(surf);
		if (!tex)
		{
			SDL_Log("Failed to convert surface to texture for %s", fileName.c_str());
			return nullptr;
		}

		mTextures.emplace(fileName.c_str(), tex);
	}
	return tex;
}

void Game::AddAsteroid(Asteroid* ast)
{
	mAsteroids.emplace_back(ast);
}

void Game::RemoveAsteroid(Asteroid* ast)
{
	auto iter = std::find(mAsteroids.begin(),
		mAsteroids.end(), ast);
	if (iter != mAsteroids.end())
	{
		mAsteroids.erase(iter);
	}
}

void Game::AddRepairItem(class RepairItem* rep)
{
	mRepairItems.emplace_back(rep);
}

void Game::RemoveRepairItem(class RepairItem* rep)
{
	auto iter = std::find(mRepairItems.begin(),
		mRepairItems.end(), rep);
	if (iter != mRepairItems.end())
	{
		mRepairItems.erase(iter);
	}
}

void Game::Shutdown()
{
	UnloadData();
	PlayResultMusic();
	font = TTF_OpenFont(FONT_PATH, 40);
	ShowResult(mRenderer, font, windowWidth, windowHeight);
	WaitLoop();
	StopMusic();

	IMG_Quit();
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	TTF_CloseFont(font);
	SDL_FreeSurface(text_surface);
	SDL_DestroyTexture(text_texture);
	TTF_Quit();
	Mix_Quit();
	SDL_Quit();
}

void Game::AddActor(Actor* actor)
{
	// If we're updating actors, need to add to pending
	if (mUpdatingActors)
	{
		mPendingActors.emplace_back(actor);
	}
	else
	{
		mActors.emplace_back(actor);
	}
}

void Game::RemoveActor(Actor* actor)
{
	// Is it in pending actors?
	auto iter = std::find(mPendingActors.begin(), mPendingActors.end(), actor);
	if (iter != mPendingActors.end())
	{
		// Swap to end of vector and pop off (avoid erase copies)
		std::iter_swap(iter, mPendingActors.end() - 1);
		mPendingActors.pop_back();
	}

	// Is it in actors?
	iter = std::find(mActors.begin(), mActors.end(), actor);
	if (iter != mActors.end())
	{
		// Swap to end of vector and pop off (avoid erase copies)
		std::iter_swap(iter, mActors.end() - 1);
		mActors.pop_back();
	}
}

void Game::AddSprite(SpriteComponent* sprite)
{
	// Find the insertion point in the sorted vector
	// (The first element with a higher draw order than me)
	int myDrawOrder = sprite->GetDrawOrder();
	auto iter = mSprites.begin();
	for (;
		iter != mSprites.end();
		++iter)
	{
		if (myDrawOrder < (*iter)->GetDrawOrder())
		{
			break;
		}
	}

	// Inserts element before position of iterator
	mSprites.insert(iter, sprite);
}

void Game::RemoveSprite(SpriteComponent* sprite)
{
	// (We can't swap because it ruins ordering)
	auto iter = std::find(mSprites.begin(), mSprites.end(), sprite);
	mSprites.erase(iter);
}

void Game::SpawnAsteroid()
{
	// Meteor オブジェクトを生成し、ゲームワールドに追加
	new Asteroid(this); 

}

void Game::SpawnRepairItem()
{
	// Meteor オブジェクトを生成し、ゲームワールドに追加
	new RepairItem(this);

}

void Game::ShowTitle(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight)
{
	// タイトル画面の背景色
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 例: 黒色の背景

	// タイトル画面に表示するテキスト
	SDL_Color textColor = { 255, 255, 255, 255 }; // 白色のテキスト
	SDL_Color bgColor = { 0, 0, 0, 0 }; // 背景色を透明に設定
	SDL_Surface* titleTextSurface = TTF_RenderUTF8_Blended(font, "Shooting Game", textColor);

	// テキストの描画位置
	SDL_Rect titleTextRect;
	titleTextRect.x = (windowWidth - titleTextSurface->w) / 2;
	titleTextRect.y = (windowHeight - titleTextSurface->h) / 2;
	titleTextRect.w = titleTextSurface->w;
	titleTextRect.h = titleTextSurface->h;

	// タイトル画面を描画
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, titleTextSurface), NULL, &titleTextRect);


	font = TTF_OpenFont(FONT_PATH, 25);
	SDL_Surface* titleTextSurface02 = TTF_RenderUTF8_Blended(font, "Press the Enter key to continue.", textColor);

	SDL_Rect titleTextRect02;
	titleTextRect02.x = (windowWidth - titleTextSurface02->w) / 2;
	titleTextRect02.y = (windowHeight - titleTextSurface02->h) * 4 / 5;
	titleTextRect02.w = titleTextSurface02->w;
	titleTextRect02.h = titleTextSurface02->h;

	SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, titleTextSurface02), NULL, &titleTextRect02);
	SDL_RenderPresent(renderer);

}

void Game::ShowExplanation(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight)
{
	// 操作説明の各行をVectorで保管
	std::vector<std::string> explanationLines = {
		"Instructions",
		"This is a shooting game.",
		"You can move with WASD and attack with the space key.",
		"Reloading occurs after firing 10 times.",
		"Asteroid: Takes damage. Can be destroyed with attacks.",
		"Wrench: Provides healing. Can be destroyed with attacks."
	};

	SDL_RenderClear(renderer);

	// 各行のY座標を計算
	int yPos = (windowHeight - explanationLines.size() * 20) / 2; // 各行の高さを適切に設定

	// 各行を描画
	SDL_Color textColor = { 255, 255, 255, 255 }; // 白色のテキスト
	for (const std::string& line : explanationLines) {
		SDL_Surface* lineSurface = TTF_RenderUTF8_Blended(font, line.c_str(), textColor);

		// 操作説明の描画位置
		SDL_Rect lineRect;
		lineRect.x = (windowWidth - lineSurface->w) / 2;
		lineRect.y = yPos;
		lineRect.w = lineSurface->w;
		lineRect.h = lineSurface->h;

		// 操作説明を描画
		SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, lineSurface), NULL, &lineRect);

		yPos += lineSurface->h + 10; // 次の行のY座標を計算
		SDL_FreeSurface(lineSurface);
	}

	font = TTF_OpenFont(FONT_PATH, 25);
	SDL_Surface* titleTextSurface02 = TTF_RenderUTF8_Blended(font, "Press the Enter key to continue.", textColor);

	SDL_Rect titleTextRect02;
	titleTextRect02.x = (windowWidth - titleTextSurface02->w) / 2;
	titleTextRect02.y = (windowHeight - titleTextSurface02->h) * 4 / 5;
	titleTextRect02.w = titleTextSurface02->w;
	titleTextRect02.h = titleTextSurface02->h;

	SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, titleTextSurface02), NULL, &titleTextRect02);

	SDL_RenderPresent(renderer);
}

void Game::ShowResult(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight)
{
	// 背景色
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 例: 黒色の背景

	// 表示するテキスト
	SDL_Color textColor = { 255, 255, 255, 255 }; // 白色のテキスト
	SDL_Color bgColor = { 0, 0, 0, 0 }; // 背景色を透明に設定
	std::string scoreText = "Game over! Your score is : " + std::to_string(static_cast<int>(score));
	SDL_Surface* titleTextSurface = TTF_RenderUTF8_Blended(font, scoreText.c_str(), textColor);

	// テキストの描画位置
	SDL_Rect titleTextRect;
	titleTextRect.x = (windowWidth - titleTextSurface->w) / 2;
	titleTextRect.y = (windowHeight - titleTextSurface->h) / 2;
	titleTextRect.w = titleTextSurface->w;
	titleTextRect.h = titleTextSurface->h;

	// タイトル画面を描画
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, titleTextSurface), NULL, &titleTextRect);


	font = TTF_OpenFont(FONT_PATH, 25);
	SDL_Surface* titleTextSurface02 = TTF_RenderUTF8_Blended(font, "Press the Enter key to quit game.", textColor);

	SDL_Rect titleTextRect02;
	titleTextRect02.x = (windowWidth - titleTextSurface02->w) / 2;
	titleTextRect02.y = (windowHeight - titleTextSurface02->h) * 4 / 5;
	titleTextRect02.w = titleTextSurface02->w;
	titleTextRect02.h = titleTextSurface02->h;

	SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, titleTextSurface02), NULL, &titleTextRect02);

	SDL_RenderPresent(renderer);
}

void Game::WaitLoop()
{
	// タイトル画面の表示待機
	bool showInstructions = false;

	while (!showInstructions) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				// ゲームを終了する処理
				showInstructions = true; // ループを抜ける
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
				// スペースキーが押されたら次の処理へ
				showInstructions = true; // ループを抜ける
			}
		}
	}
}

void Game::RenderUI(SDL_Renderer* renderer)
{
	// フォントとテキストカラーを設定
	TTF_Font* font = TTF_OpenFont(FONT_PATH, 30);
	SDL_Color textColor = { 255, 255, 255, 255 }; // 白色のテキストカラー

	// 残弾数（n/10）を計算
	int bullets = mShip->GetLaserNum();
	int maxBullets = 10; // 最大の弾数

	// HPを取得
	int currentHP = mShip->GetmHealth();
	int maxHP = mShip->GetMaxHealth();

	// スコアを取得（仮の値）
	int currentScore = static_cast<int>(score); // スコアを取得するコードを追加してください

	// 経過時間を取得（仮の値、秒単位）
	int elapsedTimeInSeconds = static_cast<int>(gameTime); // 経過時間を取得するコードを追加してください

	// テキストを作成
	std::string bulletsText = std::to_string(bullets) + "/" + std::to_string(maxBullets);
	if (bullets == 0)
	{
		bulletsText = "Reloading...";
	}

	// テキストのサーフェスを作成
	SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, bulletsText.c_str(), textColor);

	// テキストの描画位置を計算（画面上部中央）
	int textWidth = textSurface->w;
	int xPos = (windowWidth - textWidth) / 4*1;
	int yPos = 10; // 画面上部に表示

	// テキストを描画
	SDL_Rect textRect;
	textRect.x = xPos;
	textRect.y = yPos;
	textRect.w = textWidth;
	textRect.h = textSurface->h;

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// メモリの解放
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);

	// テキストを作成（HP）
	std::string hpText = "HP: " + std::to_string(currentHP) + " / " + std::to_string(maxHP);

	// テキストのサーフェスを作成（HP）
	SDL_Surface* hpTextSurface = TTF_RenderUTF8_Blended(font, hpText.c_str(), textColor);

	// テキストの描画位置を計算（HP）
	int hpTextWidth = hpTextSurface->w;
	int hpXPos = 50;
	int hpYPos = 10; // 残弾数の下に表示

	// テキストを描画（HP）
	SDL_Rect hpTextRect;
	hpTextRect.x = hpXPos;
	hpTextRect.y = hpYPos;
	hpTextRect.w = hpTextWidth;
	hpTextRect.h = hpTextSurface->h;

	SDL_Texture* hpTextTexture = SDL_CreateTextureFromSurface(renderer, hpTextSurface);
	SDL_RenderCopy(renderer, hpTextTexture, NULL, &hpTextRect);

	// メモリの解放（HP）
	SDL_FreeSurface(hpTextSurface);
	SDL_DestroyTexture(hpTextTexture);

	// テキストを作成（スコア）
	std::string scoreText = "Score : " + std::to_string(currentScore);

	// テキストのサーフェスを作成（スコア）
	SDL_Surface* scoreTextSurface = TTF_RenderUTF8_Blended(font, scoreText.c_str(), textColor);

	// テキストの描画位置を計算（スコア）
	int scoreTextWidth = scoreTextSurface->w;
	int scoreXPos = (windowWidth - scoreTextWidth)-100;
	int scoreYPos = 10; 

	// テキストを描画（スコア）
	SDL_Rect scoreTextRect;
	scoreTextRect.x = scoreXPos;
	scoreTextRect.y = scoreYPos;
	scoreTextRect.w = scoreTextWidth;
	scoreTextRect.h = scoreTextSurface->h;

	SDL_Texture* scoreTextTexture = SDL_CreateTextureFromSurface(renderer, scoreTextSurface);
	SDL_RenderCopy(renderer, scoreTextTexture, NULL, &scoreTextRect);

	// メモリの解放（スコア）
	SDL_FreeSurface(scoreTextSurface);
	SDL_DestroyTexture(scoreTextTexture);

	// テキストを作成（経過時間）
	std::string timeText = "Time : " + std::to_string(elapsedTimeInSeconds) + " seconds";

	// テキストのサーフェスを作成（経過時間）
	SDL_Surface* timeTextSurface = TTF_RenderUTF8_Blended(font, timeText.c_str(), textColor);

	// テキストの描画位置を計算（経過時間）
	int timeTextWidth = timeTextSurface->w;
	int timeXPos = (windowWidth - timeTextWidth) / 2 +30;
	int timeYPos = 10;

	// テキストを描画（経過時間）
	SDL_Rect timeTextRect;
	timeTextRect.x = timeXPos;
	timeTextRect.y = timeYPos;
	timeTextRect.w = timeTextWidth;
	timeTextRect.h = timeTextSurface->h;

	SDL_Texture* timeTextTexture = SDL_CreateTextureFromSurface(renderer, timeTextSurface);
	SDL_RenderCopy(renderer, timeTextTexture, NULL, &timeTextRect);

	// メモリの解放（経過時間）
	SDL_FreeSurface(timeTextSurface);
	SDL_DestroyTexture(timeTextTexture);

	// メモリの解放（フォント）
	TTF_CloseFont(font);

}

void Game::CalculateScore(float deltaTime)
{
	//スコアと経過時間を計算
	gameTime += deltaTime;
	score = gameTime * 100 + asteroidPoint * breakAsteroidsCount + repairItemPoint * getRepairItemsCount;
}

void Game::PlayTitleMusic()
{
	// 音楽ファイルのロード
	music = Mix_LoadMUS(MUS_TITLE_PATH);
	// 音楽を再生
	Mix_PlayMusic(music, -1);
}

void Game::PlayGameMusic()
{
	// 音楽ファイルのロード
	music = Mix_LoadMUS(MUS_GAME_PATH);
	// 音楽を再生
	Mix_PlayMusic(music, -1);
}

void Game::PlayResultMusic()
{
	// 音楽ファイルのロード
	music = Mix_LoadMUS(MUS_RESULT_PATH);
	// 音楽を再生
	Mix_PlayMusic(music, -1);
}

void Game::StopMusic()
{
	// BGMの解放
	Mix_FreeMusic(music);
}

void Game::PlayLaserSE()
{
	Mix_FreeChunk(soundEffectLaser);
	// 効果音のロード 
	soundEffectLaser = Mix_LoadWAV(SE_LASER_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectLaser, 0);
}

void Game::PlayDamageSE()
{
	Mix_FreeChunk(soundEffectDamage);
	// 効果音のロード 
	soundEffectDamage = Mix_LoadWAV(SE_DAMAGE_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectDamage, 0);
}

void Game::PlayDestroySE()
{
	Mix_FreeChunk(soundEffectDestroy);
	// 効果音のロード 
	soundEffectDestroy = Mix_LoadWAV(SE_DESTROY_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectDestroy, 0);
}

void Game::PlayRepairSE()
{
	Mix_FreeChunk(soundEffectRepair);
	// 効果音のロード 
	soundEffectRepair = Mix_LoadWAV(SE_REPAIR_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectRepair, 0);
}

void Game::PlayStartReload()
{
	Mix_FreeChunk(soundEffectRepair);
	// 効果音のロード 
	soundEffectSReload = Mix_LoadWAV(SE_STARTRELOAD_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectSReload, 0);
}

void Game::PlayEndReload()
{
	Mix_FreeChunk(soundEffectRepair);
	// 効果音のロード 
	soundEffectEReload = Mix_LoadWAV(SE_ENDRELOAD_PATH);
	// 効果音を一度だけ再生
	Mix_PlayChannel(-1, soundEffectEReload, 0);
}

////効果音の定期的な開放
//tenScecond += deltaTime;
//if (tenScecond > 4.0f)
//{
//	// 効果音の解放（再生後に解放する）
//	Mix_FreeChunk(soundEffectLaser);
//	Mix_FreeChunk(soundEffectDamage);
//	Mix_FreeChunk(soundEffectDestroy);
//	Mix_FreeChunk(soundEffectRepair);
//	Mix_FreeChunk(soundEffectSReload);
//	Mix_FreeChunk(soundEffectEReload);
//	tenScecond = 0.0f;
//}