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

	//ship�̃��[���ړ������Ɏg�p����ϐ��Q
	float mDownSpeed;
	std::vector<float>  lane = { 200.0f, 380.0f, 560.0f }; //���[����y���W�ꗗ
	int laneNum = 0;
	bool moveCheck = false;
	float count = 0;
	//�̗�
	int maxHealth = 3.0f;
	int mHealth=maxHealth;
	//�_���[�W��̖��G����
	float noDamageTime = 2.0f;
	float noDamageTimecount = 2.0f;
	//���[�U�[�̑��e���ƃ����[�h���Ԃ̊Ǘ�
	int laserNum = 10;						//���e��
	bool reload = false;						//�����[�h����true
	float reloadTime = 0.0f;				//�����[�h�o�ߎ���
	float waitReloadTime = 3.0f;			//�����[�h����
	bool noDamage=false;					//���G���ԓK�p����True
	bool showImage=false;
	float interval=0.2f;
	float countInterval = 0.0f;

	class Game* mGame;

	float endCountTime = 0.0f;
	bool endShipCheck = false;
	bool exCheck = true;
};