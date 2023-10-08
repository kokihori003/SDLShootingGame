// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.


#include "InputComponent.h"
#include "Actor.h"

InputComponent::InputComponent(class Actor* owner)
	:MoveComponent(owner)
	, mForwardKey(0)
	, mBackKey(0)
	, mClockwiseKey(0)
	, mCounterClockwiseKey(0)
{

}

//InputComponent::InputComponent(class Actor* owner)
//	:MoveComponent(owner)
//	, mForwardKey(0)
//	, mBackKey(0)
//	, mUpKey(0)
//	, mDownKey(0)
//{
//
//}

void InputComponent::ProcessInput(const uint8_t* keyState)
{
	if (keyState == nullptr)
	{
		SetForwardSpeed(0.0f);
		return;
	}
	// Calculate forward speed for MoveComponent
	float forwardSpeed = 0.0f;
	if (keyState[mForwardKey])
	{
		forwardSpeed += mMaxForwardSpeed;
	}
	if (keyState[mBackKey])
	{
		forwardSpeed -= mMaxForwardSpeed;
	}
	SetForwardSpeed(forwardSpeed);

	// Calculate angular speed for MoveComponent
	float angularSpeed = 0.0f;
	if (keyState[mClockwiseKey])
	{
		angularSpeed += mMaxAngularSpeed;
	}
	if (keyState[mCounterClockwiseKey])
	{
		angularSpeed -= mMaxAngularSpeed;
	}
	SetAngularSpeed(angularSpeed);

	//Calculate Up/Down speed
	if (keyState[mUpKey])
	{
		forwardSpeed += mMaxForwardSpeed;
	}
	if (keyState[mDownKey])
	{
		forwardSpeed -= mMaxForwardSpeed;
	}
	SetForwardSpeed(forwardSpeed);
}
