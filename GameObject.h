#pragma once

#include <Windows.h>
#include <d3dx10.h>
#include <vector>

#include "Animation.h"
#include "Animations.h"
#include "Sprites.h"
#include "Collision.h"
#include "AssetIDs.h"

using namespace std;

#define ID_TEX_BBOX -100		// special texture to draw object bounding box
#define BBOX_ALPHA 0.25f		// Bounding box transparency

class CGameObject
{
protected:
	string name;

	float x; 
	float y;

	float vx;
	float vy;

	int width;
	int height;

	int nx;	 

	int state;

	bool isDeleted;
	int blockDirection;

public: 
	void SetPosition(float x, float y) { this->x = x, this->y = y; }
	void SetSpeed(float vx, float vy) { this->vx = vx, this->vy = vy; }
	void GetPosition(float &x, float &y) { x = this->x; y = this->y; }
	void GetSpeed(float &vx, float &vy) { vx = this->vx; vy = this->vy; }

	int GetState() { return this->state; }
	virtual void Delete() { isDeleted = true;  }
	bool IsDeleted() { return isDeleted; }

	// Thiết lập giá trị thuộc tính
	void SetName(string name) { this->name = name; }

	void RenderBoundingBox();
	void RenderBoundingBox(float left, float top);

	CGameObject();
	CGameObject(float x, float y) :CGameObject() { this->x = x; this->y = y; }
	CGameObject(string name, float x, float y) : CGameObject(x, y) {
		this->name = name;
	}
	CGameObject(float x, float y, int width, int height) : CGameObject(x, y)
	{
		this->width = width;
		this->height = height;
	}
	CGameObject(string name, float x, float y, int width, int height) : CGameObject(name, x, y) {
		this->width = width;
		this->height = height;
	}

	virtual void GetBoundingBox(float &left, float &top, float &right, float &bottom) = 0;
	virtual void Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects = NULL) {};
	virtual void Render() = 0;
	virtual void SetState(int state) { this->state = state; }

	//
	// Collision ON or OFF ? This can change depending on object's state. For example: die
	//
	virtual int IsCollidable() { return 0; };

	// When no collision has been detected (triggered by CCollision::Process)
	virtual void OnNoCollision(DWORD dt) {};

	// When collision with an object has been detected (triggered by CCollision::Process)
	virtual void OnCollisionWith(LPCOLLISIONEVENT e) {};
	
	// Is this object blocking other object? If YES, collision framework will automatically push the other object
	virtual int IsBlocking() { return 1; }

	~CGameObject();

	static bool IsDeleted(const LPGAMEOBJECT &o) { return o->isDeleted; }
};
