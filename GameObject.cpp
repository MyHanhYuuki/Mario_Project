#include <d3dx9.h>
#include <algorithm>


#include "debug.h"
#include "Textures.h"
#include "Game.h"
#include "GameObject.h"
#include "Sprites.h"

CGameObject::CGameObject()
{
	x = y = 0;
	vx = vy = 0;
	nx = 1;	
	state = -1;
	isDeleted = false;
}

void CGameObject::RenderBoundingBox(float left, float top)
{
	LPTEXTURE bbox = CTextures::GetInstance()->Get(ID_TEX_BBOX);

	float l,t,r,b; 
	GetBoundingBox(l, t, r, b);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = (int)r - (int)l;
	rect.bottom = (int)b - (int)t;

	float cx, cy; 
	CGame::GetInstance()->GetCamPos(cx, cy);

	CGame::GetInstance()->Draw(left - cx, top - cy, bbox, &rect, BBOX_ALPHA);
}

void CGameObject::RenderBoundingBox()
{
	RenderBoundingBox(x, y);
}

CGameObject::~CGameObject()
{

}