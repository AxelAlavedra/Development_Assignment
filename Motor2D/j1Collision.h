#ifndef __J1COLLISION_H__
#define __J1COLLISION_H__

#include "j1Module.h"

enum COLLIDER_TYPE
{
	COLLIDER_NONE = -1,
	COLLIDER_FLOOR,
	COLLIDER_PLATFORM,
	COLLIDER_TRIGGER,
	COLLIDER_PLAYER,
	COLLIDER_ENEMY,
	COLLIDER_MAX,
};

struct Collider
{
	SDL_Rect rect;
	bool to_delete = false;
	bool enabled = true;
	COLLIDER_TYPE type;
	j1Module* callback = nullptr;

	Collider(){}
	Collider(const SDL_Rect& rectangle, const COLLIDER_TYPE &type, j1Module* callback = nullptr) :
		rect(rectangle),
		type(type),
		callback(callback)
	{}
	~Collider()
	{
		callback = nullptr;
	}

	void SetPos(int x, int y)
	{
		rect.x = x;
		rect.y = y;
	}

	void SetShape(int w, int h)
	{
		rect.w = w;
		rect.h = h;
	}

	void SetType(COLLIDER_TYPE type)
	{
		this->type = type;
	}
	bool CheckCollision(const SDL_Rect& r) const;
};

class j1Collision : public j1Module
{
public:

	j1Collision();
	~j1Collision();

	bool PostUpdate() override;
	bool CleanUp() override;
	bool PreUpdate();
	bool Awake(pugi::xml_node&);


	float DistanceToRightCollider(Collider* coll) const;
	float DistanceToLeftCollider(Collider* coll) const;
	float DistanceToTopCollider(Collider* coll) const;
	float DistanceToBottomCollider(Collider* coll, bool ignore_platforms = false) const;
	Collider* AddCollider(const SDL_Rect &rect, const COLLIDER_TYPE &type, j1Module* callback = nullptr, const bool &player = false);

	void DebugDraw();

private:
	Collider** colliders = nullptr;
	Collider* player_collider = nullptr;
	bool debug = false;
	uint max_colliders = 0;
};

#endif
