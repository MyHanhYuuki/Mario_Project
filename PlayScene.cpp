#include <iostream>
#include <fstream>
#include "AssetIDs.h"

#include "PlayScene.h"
#include "Utils.h"
#include "Textures.h"
#include "Sprites.h"
#include "Portal.h"
#include "Coin.h"
#include "Platform.h"
#include "tinyxml.h"

#include "SampleKeyEventHandler.h"

using namespace std;

CPlayScene::CPlayScene(int id, LPCWSTR filePath):
	CScene(id, filePath)
{
	player = NULL;
	key_handler = new CSampleKeyHandler(this);
}


#define SCENE_SECTION_UNKNOWN -1
#define SCENE_SECTION_ASSETS	1
#define SCENE_SECTION_TILEMAP	2
#define SCENE_SECTION_OBJECTS	3

#define ASSETS_SECTION_UNKNOWN -1
#define ASSETS_SECTION_SPRITES 1
#define ASSETS_SECTION_ANIMATIONS 2

#define MAX_SCENE_LINE 1024

void CPlayScene::_ParseSection_SPRITES(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 6) return; // skip invalid lines

	int ID = atoi(tokens[0].c_str());
	int l = atoi(tokens[1].c_str());
	int t = atoi(tokens[2].c_str());
	int r = atoi(tokens[3].c_str());
	int b = atoi(tokens[4].c_str());
	int texID = atoi(tokens[5].c_str());

	LPTEXTURE tex = CTextures::GetInstance()->Get(texID);
	if (tex == NULL)
	{
		DebugOut(L"[ERROR] Texture ID %d not found!\n", texID);
		return; 
	}

	CSprites::GetInstance()->Add(ID, l, t, r, b, tex);
}

void CPlayScene::_ParseSection_ASSETS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 1) return;

	wstring path = ToWSTR(tokens[0]);
	
	LoadAssets(path.c_str());
}

// Đọc section [TILEMAP] trong file config (scene0005.txt)
void CPlayScene::_ParseSection_MAP(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 1) return;

	LoadMap(tokens[0]);
}

void CPlayScene::_ParseSection_ANIMATIONS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 3) return; // skip invalid lines - an animation must at least has 1 frame and 1 frame time

	//DebugOut(L"--> %s\n",ToWSTR(line).c_str());

	LPANIMATION ani = new CAnimation();

	int ani_id = atoi(tokens[0].c_str());
	for (int i = 1; i < tokens.size(); i += 2)	// why i+=2 ?  sprite_id | frame_time  
	{
		int sprite_id = atoi(tokens[i].c_str());
		int frame_time = atoi(tokens[i+1].c_str());
		ani->Add(sprite_id, frame_time);
	}

	CAnimations::GetInstance()->Add(ani_id, ani);
}

/*
	Parse a line in section [OBJECTS] 
*/
void CPlayScene::_ParseSection_OBJECTS(string line)
{
	vector<string> tokens = split(line);

	// skip invalid lines - an object set must have at least id, x, y
	if (tokens.size() < 2) return;

	int object_type = atoi(tokens[0].c_str());
	float x = (float)atof(tokens[1].c_str());
	float y = (float)atof(tokens[2].c_str());

	CGameObject *obj = NULL;

	switch (object_type)
	{
	case OBJECT_TYPE_MARIO:
		if (player!=NULL) 
		{
			DebugOut(L"[ERROR] MARIO object was created before!\n");
			return;
		}
		obj = new CMario(x,y); 
		player = (CMario*)obj;  

		DebugOut(L"[INFO] Player object has been created!\n");
		break;
	case OBJECT_TYPE_GOOMBA: obj = new CGoomba(x,y); break;
	case OBJECT_TYPE_BRICK: obj = new CBrick(x,y); break;
	case OBJECT_TYPE_COIN: obj = new CCoin(x, y); break;

	case OBJECT_TYPE_PLATFORM:
	{

		float cell_width = (float)atof(tokens[3].c_str());
		float cell_height = (float)atof(tokens[4].c_str());
		int length = atoi(tokens[5].c_str());
		int sprite_begin = atoi(tokens[6].c_str());
		int sprite_middle = atoi(tokens[7].c_str());
		int sprite_end = atoi(tokens[8].c_str());

		obj = new CPlatform(
			x, y,
			cell_width, cell_height, length,
			sprite_begin, sprite_middle, sprite_end
		);

		break;
	}

	case OBJECT_TYPE_PORTAL:
	{
		float r = (float)atof(tokens[3].c_str());
		float b = (float)atof(tokens[4].c_str());
		int scene_id = atoi(tokens[5].c_str());
		obj = new CPortal(x, y, r, b, scene_id);
	}
	break;


	default:
		DebugOut(L"[ERROR] Invalid object type: %d\n", object_type);
		return;
	}

	// General object setup
	obj->SetPosition(x, y);


	objects.push_back(obj);
}

void CPlayScene::LoadAssets(LPCWSTR assetFile)
{
	DebugOut(L"[INFO] Start loading assets from : %s \n", assetFile);

	ifstream f;
	f.open(assetFile);

	int section = ASSETS_SECTION_UNKNOWN;

	char str[MAX_SCENE_LINE];
	while (f.getline(str, MAX_SCENE_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines	

		if (line == "[SPRITES]") { section = ASSETS_SECTION_SPRITES; continue; };
		if (line == "[ANIMATIONS]") { section = ASSETS_SECTION_ANIMATIONS; continue; };
		if (line[0] == '[') { section = SCENE_SECTION_UNKNOWN; continue; }

		//
		// data section
		//
		switch (section)
		{
		case ASSETS_SECTION_SPRITES: _ParseSection_SPRITES(line); break;
		case ASSETS_SECTION_ANIMATIONS: _ParseSection_ANIMATIONS(line); break;
		}
	}

	f.close();

	DebugOut(L"[INFO] Done loading assets from %s\n", assetFile);
}

//Đọc file tmx (world-1-1.tmx) - load file Texture dùng trong TileMap

TileSet* CPlayScene::LoadTileSet(TiXmlElement* root)
{
	// Lấy tên file tileset để mở file đó và lấy các thông tin cần thiết
	TiXmlElement* element = root->FirstChildElement("tileset");
	string imageSourcePath = element->Attribute("source");

	TiXmlDocument imageRootNode(("scenes//" + imageSourcePath).c_str());
	if (imageRootNode.LoadFile())
	{
		TiXmlElement* tileSetElement = imageRootNode.RootElement();

		TileSet* tileSet = new TileSet();
		tileSetElement->QueryIntAttribute("spacing", &tileSet->spacing);
		tileSetElement->QueryIntAttribute("margin", &tileSet->margin);
		tileSetElement->QueryIntAttribute("tilewidth", &tileSet->width);
		tileSetElement->QueryIntAttribute("tileheight", &tileSet->height);
		tileSetElement->QueryIntAttribute("columns", &tileSet->columns);

		return tileSet;
	}
	return NULL;
}

// Load tag của layer vào grid 2D [width, heigth]
void CPlayScene::ParseTile(TiXmlElement* root)
{
	TiXmlElement* layerElement = root->FirstChildElement("layer");
	if (layerElement == nullptr) {
		return;
	}

	int visible;
	layerElement->QueryIntAttribute("visible", &visible);
	if (visible == 0) {
		return;
	}

	int colCount, rowCount;
	layerElement->QueryIntAttribute("width", &colCount);
	layerElement->QueryIntAttribute("height", &rowCount);

	string strTile = layerElement->FirstChildElement("data")->GetText();
	vector<string> tileArr = split(strTile, ",");

	// Điền vào grid theo trình tự: trái -> phải, trên -> dưới
	vector<vector<int>> tileIDs(colCount, vector<int>(rowCount));
	for (int col = 0; col < colCount; col++)
	{
		for (int row = 0; row < rowCount; row++)
		{
			tileIDs[col][row] = stoi(tileArr[int(col + (row * colCount))]);
		}
	}

	// Clear data
	tileArr.clear();

	// Load tileset
	TileSet* tileSet = LoadTileSet(root);
	if (tileSet == NULL) {
		DebugOut(L"[ERROR] fail to load tile set");
		return;
	}

	// Add tile Object
	int spacing = tileSet->spacing,
		margin = tileSet->margin,
		tileWidth = tileSet->width,
		tileHeight = tileSet->height,
		column = tileSet->columns;

	CSprites* sprites = CSprites::GetInstance();
	LPTEXTURE texMap = CTextures::GetInstance()->Get(ID_TEX_IMAGEMAP); //AssetID.h

	int fistSpriteId = SPRITE_ID_START_TILEMAP;
	//  Khởi tạo tile Object (vị trí, dài, rộng, thứ tự tile trong texture) theo vị trí tương ứng trong grid
	for (int j = 0; j < tileIDs[0].size(); j++)
	{
		for (int i = 0; i < tileIDs.size(); i++)
		{
			// Vị trí của sprite (ith) trong ảnh texture bắt đầu từ số 1 (không phải từ số 0)
			int sprite_ith = tileIDs[i][j];

			// Khác 0 => Vị trí này có sprite
			if (sprite_ith != 0)
			{
				int x_th, y_th; // Bắt đầu từ số 0
				if (sprite_ith % column > 0) // Chia dư, tức là sprite này không nằm ở cột cuối cùng của tileset
				{
					x_th = sprite_ith % column - 1; // Vị trí của sprite trong tileset (sprite_ith) thì bắt đầu từ só 1, cần trừ 1 để bắt đầu từ 0 
					y_th = sprite_ith / column;
				}
				else // Nếu chia không dư thì vị trí x_th của sprite nằm ở cột cuối cùng
				{
					x_th = column;
					y_th = sprite_ith / column - 1;
				}

				int left = margin + x_th * (tileWidth + spacing);
				int top = margin + y_th * (tileHeight + spacing);
				int right = left + tileWidth - 1;
				int bottom = top + tileHeight - 1;

				// Thêm sprite vào Database
				sprites->Add(fistSpriteId, left, top, right, bottom, texMap);

				LPTILE tile = new CTile((float)i, (float)j, tileWidth, tileHeight, fistSpriteId++);
				tiles.push_back(tile);
			}
		}
			
	}
}


void CPlayScene::LoadMap(string filePath)
{
	// Đọc file tml và tải vào bộ nhớ
	TiXmlDocument doc(filePath.c_str()); 
	if (!doc.LoadFile())
	{
		printf("%s", doc.ErrorDesc());
		return;
	}

	// Đọc và khởi tạo tile object (backgroud, mây, cây,...)
	auto root = doc.RootElement();
	ParseTile(root);

	// Đọc thuộc tính Map (width)
	root->QueryIntAttribute("width", &worldWidth);

	// Chuyển đổi Object trong Object Layer
	for (auto element = root->FirstChildElement("objectgroup"); element != NULL; element = element->NextSiblingElement("objectgroup"))
	{
		for (TiXmlElement* object = element->FirstChildElement("object"); object != nullptr; object = object->NextSiblingElement("object"))
		{
			CGameObject* gameObject = NULL;

			float x, y;
			int width, height;
			string name;

			name = object->Attribute("name");
			object->QueryFloatAttribute("x", &x);
			object->QueryFloatAttribute("y", &y);
			object->QueryIntAttribute("width", &width);
			object->QueryIntAttribute("height", &height);

			// Không chuyển đổi Object bị ẩn
			if (object->Attribute("visible"))
			{
				continue;
			}

			// Khởi tạo Object theo "Tên" (Ở TileMap)

			if ((int)name.rfind("Mario") >= 0) // Object có phải tên Mario không?
			{
				if (player != NULL)
				{
					DebugOut(L"[ERROR] MARIO object was created before!\n");
					return;
				}
				gameObject = new CMario(x, y);
				player = (CMario*)gameObject;

				DebugOut(L"[INFO] Player object has been created!\n");
				MakeCameraFollowMario();
			}

			if (gameObject)
			{
				gameObject->SetName(name);
				this->objects.push_back(gameObject);
			}
		}
	}
}

void CPlayScene::MakeCameraFollowMario()
{
	//Thời điểm khởi tạo không cân Update
	if (player == NULL) return;

	// Update camera to follow mario
	float cx, cy;

	//Lấy vị trí Camera
	CGame* game = CGame::GetInstance();
	game->GetCamPos(cx, cy);

	//Điều chỉnh vị trí camera dựa trên Mario
	if (player->GetState() != MARIO_STATE_DIE) {
		player->GetPosition(cx, cy);
		cx -= game->GetBackBufferWidth() / 2;
		cy -= game->GetBackBufferHeight() / 2;
	}

	if (cx < 0) {
		cx = 0;
	}
	else if ((cx + game->GetBackBufferWidth() / 2) > worldWidth) {
		cx = worldWidth - game->GetBackBufferWidth() / 2.0f;
	}

	CGame::GetInstance()->SetCamPos(cx, /*0.0f*/ cy);
}


void CPlayScene::Load()
{
	DebugOut(L"[INFO] Start loading scene from : %s \n", sceneFilePath);

	ifstream f;
	f.open(sceneFilePath);

	// current resource section flag
	int section = SCENE_SECTION_UNKNOWN;					

	char str[MAX_SCENE_LINE];
	while (f.getline(str, MAX_SCENE_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines	
		if (line == "[ASSETS]") { section = SCENE_SECTION_ASSETS; continue; };
		if (line == "[TITLEMAP]") { section = SCENE_SECTION_TILEMAP; continue; };
		if (line == "[OBJECTS]") { section = SCENE_SECTION_OBJECTS; continue; };
		if (line[0] == '[') { section = SCENE_SECTION_UNKNOWN; continue; }	

		//
		// data section
		//
		switch (section)
		{ 
			case SCENE_SECTION_ASSETS: _ParseSection_ASSETS(line); break;
			case SCENE_SECTION_TILEMAP: _ParseSection_MAP(line); break;
			case SCENE_SECTION_OBJECTS: _ParseSection_OBJECTS(line); break;
		}
	}

	f.close();

	DebugOut(L"[INFO] Done loading scene  %s\n", sceneFilePath);
}

void CPlayScene::Update(DWORD dt)
{
	// We know that Mario is the first object in the list hence we won't add him into the colliable object list
	// TO-DO: This is a "dirty" way, need a more organized way 

	vector<LPGAMEOBJECT> coObjects;
	for (size_t i = 1; i < objects.size(); i++)
	{
		coObjects.push_back(objects[i]);
	}

	for (size_t i = 0; i < objects.size(); i++)
	{
		objects[i]->Update(dt, &coObjects);
	}

	// skip the rest if scene was already unloaded (Mario::Update might trigger PlayScene::Unload)
	if (player == NULL) return; 

	// Update camera to follow Mario
    MakeCameraFollowMario();

	PurgeDeletedObjects();
}

void CPlayScene::Render()
{
	for (int i = 0; i < objects.size(); i++)
		objects[i]->Render();
}

/*
*	Clear all objects from this scene
*/
void CPlayScene::Clear()
{
	vector<LPGAMEOBJECT>::iterator it;
	for (it = objects.begin(); it != objects.end(); it++)
	{
		delete (*it);
	}
	objects.clear();
}

/*
	Unload scene

	TODO: Beside objects, we need to clean up sprites, animations and textures as well 

*/
void CPlayScene::Unload()
{
	for (int i = 0; i < objects.size(); i++)
		delete objects[i];

	objects.clear();
	player = NULL;

	DebugOut(L"[INFO] Scene %d unloaded! \n", id);
}

bool CPlayScene::IsGameObjectDeleted(const LPGAMEOBJECT& o) { return o == NULL; }

void CPlayScene::PurgeDeletedObjects()
{
	vector<LPGAMEOBJECT>::iterator it;
	for (it = objects.begin(); it != objects.end(); it++)
	{
		LPGAMEOBJECT o = *it;
		if (o->IsDeleted())
		{
			delete o;
			*it = NULL;
		}
	}

	// NOTE: remove_if will swap all deleted items to the end of the vector
	// then simply trim the vector, this is much more efficient than deleting individual items
	objects.erase(
		std::remove_if(objects.begin(), objects.end(), CPlayScene::IsGameObjectDeleted),
		objects.end());
}