// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
#pragma once
#include "Actor.h"
#include "SDL.h"

class Ship : public Actor
{
public:
	Ship(class Game* game);
	~Ship();

	

	void UpdateActor(float deltaTime) override;
	void ActorInput(const uint8_t* keyState) override;

	void CheckCollision(float deltaTime);
	void OnCollisionAsteroids();
	void OnCollisionRepairItems();

	void SetDamageTex(float deltaTime);
	void SetShipTex();
	void SetExplosionTex();

	void EndShip(float deltaTime);

	class SpriteComponent* spriteComponent;

	SDL_Texture* mTexture=nullptr;

	int GetLaserNum() const { return laserNum; }

	int GetMaxHealth() const { return maxHealth; }
	int GetmHealth() const { return mHealth; }

	bool PermitInput() { return endShipCheck; }

private:

	void Fire();

	float mLaserCooldown;
	class CircleComponent* mCircle;

	//shipのレーン移動処理に使用する変数群
	float mDownSpeed;
	std::vector<float>  lane = { 200.0f, 380.0f, 560.0f }; //レーンのy座標一覧
	int laneNum = 0;
	bool moveCheck = false;
	float count = 0;
	//体力
	int maxHealth = 3.0f;
	int mHealth=maxHealth;
	//ダメージ後の無敵時間
	float noDamageTime = 2.0f;
	float noDamageTimecount = 2.0f;
	//レーザーの装弾数とリロード時間の管理
	int laserNum = 10;						//装弾数
	bool reload = false;						//リロード中はtrue
	float reloadTime = 0.0f;				//リロード経過時間
	float waitReloadTime = 3.0f;			//リロード時間
	bool noDamage=false;					//無敵時間適用中はTrue
	bool showImage=false;
	float interval=0.2f;
	float countInterval = 0.0f;

	class Game* mGame;

	float endCountTime = 0.0f;
	bool endShipCheck = false;
	bool exCheck = true;
};