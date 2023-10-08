// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.

#include "Laser.h"
#include "SpriteComponent.h"
#include "MoveComponent.h"
#include "Game.h"
#include "CircleComponent.h"
#include "Asteroid.h"
#include "RepairItem.h"

Laser::Laser(Game* game)
	:Actor(game)
	, mDeathTimer(1.0f)
{
	// Create a sprite component
	SpriteComponent* sc = new SpriteComponent(this);
	sc->SetTexture(game->GetTexture("Assets/Laser.png"));

	// Create a move component, and set a forward speed
	MoveComponent* mc = new MoveComponent(this);
	mc->SetForwardSpeed(800.0f);

	// Create a circle component (for collision)
	mCircle = new CircleComponent(this);
	mCircle->SetRadius(11.0f);
}

void Laser::UpdateActor(float deltaTime)
{
	// If we run out of time, laser is dead
	Vector2 pos = GetPosition();
	//右端でレーザー消滅
	if (pos.x >= 1000)
	{
		SetState(EDead);
	}
	mDeathTimer -= deltaTime;
	if (mDeathTimer <= 0.0f)
	{
		SetState(EDead);
	}
	else
	{
		//RepairItemとの衝突判定
		for (auto rep : GetGame()->GetRepairItems())
		{
			if (Intersect(*mCircle, *(rep->GetCircle())))
			{
				// The first asteroid we intersect with,
				// set ourselves and the asteroid to dead
				rep->SetState(EDead);
				GetGame()->PlayDestroySE();
				SetState(EDead);
				
				break;
			}
		}

		// Do we intersect with an asteroid?
		for (auto ast : GetGame()->GetAsteroids())
		{
			if (Intersect(*mCircle, *(ast->GetCircle())))
			{
				// The first asteroid we intersect with,
				// set ourselves and the asteroid to dead
				ast->SetState(EDead);
				GetGame()->AddBreakAsteroidsCount();
				GetGame()->PlayDestroySE();
				SetState(EDead);
				break;
			}
		}
	}
}
