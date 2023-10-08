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

	//�^�C�g����ʂɂȂ�摜�╶���Ȃǂ�\�����A
	// ����̓��͂��Ȃ����܂Ŏ��̏�����҂R�[�h
	// ���y���Đ�
	PlayTitleMusic();
	ShowTitle(mRenderer, font, windowWidth, windowHeight);
	WaitLoop();
	font = TTF_OpenFont(FONT_PATH, 30); // 24�̓t�H���g�̃T�C�Y
	ShowExplanation(mRenderer, font, windowWidth, windowHeight);
	WaitLoop();
	// BGM�̉��
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
	//Ship�̃f�X�g���N�^���Ăяo�����ƌĂяo��
	//�Q�[���I������
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
	// ��莞�Ԃ��Ƃ�Asteroid�𐶐�����
	if (spawnAsteroidSpeed >= spawnAsteroidInterval)
	{
		SpawnAsteroid();
		spawnAsteroidSpeed = 0.0f;
	}

	spawnRepairItemSpeed += deltaTime;
	// ��莞�Ԃ��Ƃ�Asteroid�𐶐�����
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
	// Meteor �I�u�W�F�N�g�𐶐����A�Q�[�����[���h�ɒǉ�
	new Asteroid(this); 

}

void Game::SpawnRepairItem()
{
	// Meteor �I�u�W�F�N�g�𐶐����A�Q�[�����[���h�ɒǉ�
	new RepairItem(this);

}

void Game::ShowTitle(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight)
{
	// �^�C�g����ʂ̔w�i�F
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // ��: ���F�̔w�i

	// �^�C�g����ʂɕ\������e�L�X�g
	SDL_Color textColor = { 255, 255, 255, 255 }; // ���F�̃e�L�X�g
	SDL_Color bgColor = { 0, 0, 0, 0 }; // �w�i�F�𓧖��ɐݒ�
	SDL_Surface* titleTextSurface = TTF_RenderUTF8_Blended(font, "Shooting Game", textColor);

	// �e�L�X�g�̕`��ʒu
	SDL_Rect titleTextRect;
	titleTextRect.x = (windowWidth - titleTextSurface->w) / 2;
	titleTextRect.y = (windowHeight - titleTextSurface->h) / 2;
	titleTextRect.w = titleTextSurface->w;
	titleTextRect.h = titleTextSurface->h;

	// �^�C�g����ʂ�`��
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
	// ��������̊e�s��Vector�ŕۊ�
	std::vector<std::string> explanationLines = {
		"Instructions",
		"This is a shooting game.",
		"You can move with WASD and attack with the space key.",
		"Reloading occurs after firing 10 times.",
		"Asteroid: Takes damage. Can be destroyed with attacks.",
		"Wrench: Provides healing. Can be destroyed with attacks."
	};

	SDL_RenderClear(renderer);

	// �e�s��Y���W���v�Z
	int yPos = (windowHeight - explanationLines.size() * 20) / 2; // �e�s�̍�����K�؂ɐݒ�

	// �e�s��`��
	SDL_Color textColor = { 255, 255, 255, 255 }; // ���F�̃e�L�X�g
	for (const std::string& line : explanationLines) {
		SDL_Surface* lineSurface = TTF_RenderUTF8_Blended(font, line.c_str(), textColor);

		// ��������̕`��ʒu
		SDL_Rect lineRect;
		lineRect.x = (windowWidth - lineSurface->w) / 2;
		lineRect.y = yPos;
		lineRect.w = lineSurface->w;
		lineRect.h = lineSurface->h;

		// ���������`��
		SDL_RenderCopy(renderer, SDL_CreateTextureFromSurface(renderer, lineSurface), NULL, &lineRect);

		yPos += lineSurface->h + 10; // ���̍s��Y���W���v�Z
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
	// �w�i�F
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // ��: ���F�̔w�i

	// �\������e�L�X�g
	SDL_Color textColor = { 255, 255, 255, 255 }; // ���F�̃e�L�X�g
	SDL_Color bgColor = { 0, 0, 0, 0 }; // �w�i�F�𓧖��ɐݒ�
	std::string scoreText = "Game over! Your score is : " + std::to_string(static_cast<int>(score));
	SDL_Surface* titleTextSurface = TTF_RenderUTF8_Blended(font, scoreText.c_str(), textColor);

	// �e�L�X�g�̕`��ʒu
	SDL_Rect titleTextRect;
	titleTextRect.x = (windowWidth - titleTextSurface->w) / 2;
	titleTextRect.y = (windowHeight - titleTextSurface->h) / 2;
	titleTextRect.w = titleTextSurface->w;
	titleTextRect.h = titleTextSurface->h;

	// �^�C�g����ʂ�`��
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
	// �^�C�g����ʂ̕\���ҋ@
	bool showInstructions = false;

	while (!showInstructions) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				// �Q�[�����I�����鏈��
				showInstructions = true; // ���[�v�𔲂���
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
				// �X�y�[�X�L�[�������ꂽ�玟�̏�����
				showInstructions = true; // ���[�v�𔲂���
			}
		}
	}
}

void Game::RenderUI(SDL_Renderer* renderer)
{
	// �t�H���g�ƃe�L�X�g�J���[��ݒ�
	TTF_Font* font = TTF_OpenFont(FONT_PATH, 30);
	SDL_Color textColor = { 255, 255, 255, 255 }; // ���F�̃e�L�X�g�J���[

	// �c�e���in/10�j���v�Z
	int bullets = mShip->GetLaserNum();
	int maxBullets = 10; // �ő�̒e��

	// HP���擾
	int currentHP = mShip->GetmHealth();
	int maxHP = mShip->GetMaxHealth();

	// �X�R�A���擾�i���̒l�j
	int currentScore = static_cast<int>(score); // �X�R�A���擾����R�[�h��ǉ����Ă�������

	// �o�ߎ��Ԃ��擾�i���̒l�A�b�P�ʁj
	int elapsedTimeInSeconds = static_cast<int>(gameTime); // �o�ߎ��Ԃ��擾����R�[�h��ǉ����Ă�������

	// �e�L�X�g���쐬
	std::string bulletsText = std::to_string(bullets) + "/" + std::to_string(maxBullets);
	if (bullets == 0)
	{
		bulletsText = "Reloading...";
	}

	// �e�L�X�g�̃T�[�t�F�X���쐬
	SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, bulletsText.c_str(), textColor);

	// �e�L�X�g�̕`��ʒu���v�Z�i��ʏ㕔�����j
	int textWidth = textSurface->w;
	int xPos = (windowWidth - textWidth) / 4*1;
	int yPos = 10; // ��ʏ㕔�ɕ\��

	// �e�L�X�g��`��
	SDL_Rect textRect;
	textRect.x = xPos;
	textRect.y = yPos;
	textRect.w = textWidth;
	textRect.h = textSurface->h;

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// �������̉��
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);

	// �e�L�X�g���쐬�iHP�j
	std::string hpText = "HP: " + std::to_string(currentHP) + " / " + std::to_string(maxHP);

	// �e�L�X�g�̃T�[�t�F�X���쐬�iHP�j
	SDL_Surface* hpTextSurface = TTF_RenderUTF8_Blended(font, hpText.c_str(), textColor);

	// �e�L�X�g�̕`��ʒu���v�Z�iHP�j
	int hpTextWidth = hpTextSurface->w;
	int hpXPos = 50;
	int hpYPos = 10; // �c�e���̉��ɕ\��

	// �e�L�X�g��`��iHP�j
	SDL_Rect hpTextRect;
	hpTextRect.x = hpXPos;
	hpTextRect.y = hpYPos;
	hpTextRect.w = hpTextWidth;
	hpTextRect.h = hpTextSurface->h;

	SDL_Texture* hpTextTexture = SDL_CreateTextureFromSurface(renderer, hpTextSurface);
	SDL_RenderCopy(renderer, hpTextTexture, NULL, &hpTextRect);

	// �������̉���iHP�j
	SDL_FreeSurface(hpTextSurface);
	SDL_DestroyTexture(hpTextTexture);

	// �e�L�X�g���쐬�i�X�R�A�j
	std::string scoreText = "Score : " + std::to_string(currentScore);

	// �e�L�X�g�̃T�[�t�F�X���쐬�i�X�R�A�j
	SDL_Surface* scoreTextSurface = TTF_RenderUTF8_Blended(font, scoreText.c_str(), textColor);

	// �e�L�X�g�̕`��ʒu���v�Z�i�X�R�A�j
	int scoreTextWidth = scoreTextSurface->w;
	int scoreXPos = (windowWidth - scoreTextWidth)-100;
	int scoreYPos = 10; 

	// �e�L�X�g��`��i�X�R�A�j
	SDL_Rect scoreTextRect;
	scoreTextRect.x = scoreXPos;
	scoreTextRect.y = scoreYPos;
	scoreTextRect.w = scoreTextWidth;
	scoreTextRect.h = scoreTextSurface->h;

	SDL_Texture* scoreTextTexture = SDL_CreateTextureFromSurface(renderer, scoreTextSurface);
	SDL_RenderCopy(renderer, scoreTextTexture, NULL, &scoreTextRect);

	// �������̉���i�X�R�A�j
	SDL_FreeSurface(scoreTextSurface);
	SDL_DestroyTexture(scoreTextTexture);

	// �e�L�X�g���쐬�i�o�ߎ��ԁj
	std::string timeText = "Time : " + std::to_string(elapsedTimeInSeconds) + " seconds";

	// �e�L�X�g�̃T�[�t�F�X���쐬�i�o�ߎ��ԁj
	SDL_Surface* timeTextSurface = TTF_RenderUTF8_Blended(font, timeText.c_str(), textColor);

	// �e�L�X�g�̕`��ʒu���v�Z�i�o�ߎ��ԁj
	int timeTextWidth = timeTextSurface->w;
	int timeXPos = (windowWidth - timeTextWidth) / 2 +30;
	int timeYPos = 10;

	// �e�L�X�g��`��i�o�ߎ��ԁj
	SDL_Rect timeTextRect;
	timeTextRect.x = timeXPos;
	timeTextRect.y = timeYPos;
	timeTextRect.w = timeTextWidth;
	timeTextRect.h = timeTextSurface->h;

	SDL_Texture* timeTextTexture = SDL_CreateTextureFromSurface(renderer, timeTextSurface);
	SDL_RenderCopy(renderer, timeTextTexture, NULL, &timeTextRect);

	// �������̉���i�o�ߎ��ԁj
	SDL_FreeSurface(timeTextSurface);
	SDL_DestroyTexture(timeTextTexture);

	// �������̉���i�t�H���g�j
	TTF_CloseFont(font);

}

void Game::CalculateScore(float deltaTime)
{
	//�X�R�A�ƌo�ߎ��Ԃ��v�Z
	gameTime += deltaTime;
	score = gameTime * 100 + asteroidPoint * breakAsteroidsCount + repairItemPoint * getRepairItemsCount;
}

void Game::PlayTitleMusic()
{
	// ���y�t�@�C���̃��[�h
	music = Mix_LoadMUS(MUS_TITLE_PATH);
	// ���y���Đ�
	Mix_PlayMusic(music, -1);
}

void Game::PlayGameMusic()
{
	// ���y�t�@�C���̃��[�h
	music = Mix_LoadMUS(MUS_GAME_PATH);
	// ���y���Đ�
	Mix_PlayMusic(music, -1);
}

void Game::PlayResultMusic()
{
	// ���y�t�@�C���̃��[�h
	music = Mix_LoadMUS(MUS_RESULT_PATH);
	// ���y���Đ�
	Mix_PlayMusic(music, -1);
}

void Game::StopMusic()
{
	// BGM�̉��
	Mix_FreeMusic(music);
}

void Game::PlayLaserSE()
{
	Mix_FreeChunk(soundEffectLaser);
	// ���ʉ��̃��[�h 
	soundEffectLaser = Mix_LoadWAV(SE_LASER_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectLaser, 0);
}

void Game::PlayDamageSE()
{
	Mix_FreeChunk(soundEffectDamage);
	// ���ʉ��̃��[�h 
	soundEffectDamage = Mix_LoadWAV(SE_DAMAGE_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectDamage, 0);
}

void Game::PlayDestroySE()
{
	Mix_FreeChunk(soundEffectDestroy);
	// ���ʉ��̃��[�h 
	soundEffectDestroy = Mix_LoadWAV(SE_DESTROY_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectDestroy, 0);
}

void Game::PlayRepairSE()
{
	Mix_FreeChunk(soundEffectRepair);
	// ���ʉ��̃��[�h 
	soundEffectRepair = Mix_LoadWAV(SE_REPAIR_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectRepair, 0);
}

void Game::PlayStartReload()
{
	Mix_FreeChunk(soundEffectRepair);
	// ���ʉ��̃��[�h 
	soundEffectSReload = Mix_LoadWAV(SE_STARTRELOAD_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectSReload, 0);
}

void Game::PlayEndReload()
{
	Mix_FreeChunk(soundEffectRepair);
	// ���ʉ��̃��[�h 
	soundEffectEReload = Mix_LoadWAV(SE_ENDRELOAD_PATH);
	// ���ʉ�����x�����Đ�
	Mix_PlayChannel(-1, soundEffectEReload, 0);
}

////���ʉ��̒���I�ȊJ��
//tenScecond += deltaTime;
//if (tenScecond > 4.0f)
//{
//	// ���ʉ��̉���i�Đ���ɉ������j
//	Mix_FreeChunk(soundEffectLaser);
//	Mix_FreeChunk(soundEffectDamage);
//	Mix_FreeChunk(soundEffectDestroy);
//	Mix_FreeChunk(soundEffectRepair);
//	Mix_FreeChunk(soundEffectSReload);
//	Mix_FreeChunk(soundEffectEReload);
//	tenScecond = 0.0f;
//}