// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.


#pragma once
#include "Actor.h"
class Asteroid : public Actor
{
public:
	Asteroid(class Game* game);
	~Asteroid();

	class CircleComponent* GetCircle() { return mCircle; }

	class SpriteComponent* spriteComponent;

private:
	class CircleComponent* mCircle;

	std::vector<float>  lane = { 200.0f, 380.0f, 560.0f }; //ƒŒ[ƒ“‚ÌyÀ•Wˆê——
	int laneNum = 0;

	float endCountTime = 0.0f;
	bool endAsteroidCheck = false;
	bool exCheck = true;
};
