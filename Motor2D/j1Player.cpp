#include "j1App.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Collision.h"
#include "j1Map.h"
#include "j1SwapScene.h"
#include "j1Scene.h"
#include "p2Defs.h"
#include "j1Audio.h"
#include "p2Log.h"
#include "j1Input.h"
#include "j1Player.h"
#include "j1EntityManager.h"
#include "Brofiler/Brofiler.h"


j1Player::j1Player(EntityType type, pugi::xml_node config, fPoint position, p2SString id, int clone_number) : j1Entity(type, config, position, id)
{
	jump_fx = App->audio->LoadFx(config.child("audio").child("jump_fx").child_value());
	charged_time = config.child("charged_jump").attribute("time").as_float();
	charge_increment = config.child("charged_jump").attribute("charge_increment").as_float();
	max_charge = config.child("charged_jump").attribute("max_charge").as_float();
	starting_lives = config.child("lives").attribute("value").as_int();

	collider = App->collision->AddCollider(animation_frame, COLLIDER_PLAYER, App->entitymanager, true);

	if (clone_number > 1)
	{
		scale_X = App->entitymanager->player->scale_X;
		scale_Y = App->entitymanager->player->scale_Y;
		collider_offset = 40 * scale_Y;
		collider->rect.h = 53 * scale_Y;
		collider->rect.w = 94 * scale_X;
	}

	collider->rect.x = position.x;
	collider->rect.y = position.y + collider_offset;
	this->clone_number = clone_number;
}

j1Player::~j1Player()
{}

bool j1Player::PreUpdate() 
{
	if (!App->paused)
	{
		if (App->input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN)
		{
			if (state != GOD) state = GOD;
			else state = JUMPING;
		}

		switch (state) {
		case IDLE:
			IdleUpdate();
			break;
		case MOVING:
			MovingUpdate();
			break;
		case JUMPING:
			JumpingUpdate();
			break;
		case DEAD:
			break;
		case CHARGE:
			ChargingUpdate();
			break;
		case WIN:
			target_speed.x = 0.0F;
			break;
		case GOD:
			GodUpdate();
			break;
		default:
			break;
		}

	}
	
	return true;
}

bool j1Player::Update(float dt)
{
	CheckDeath();

	switch (state)
	{
		case JUMPING:
		{
			target_speed.y += gravity * dt;
			if (target_speed.y > fall_speed) target_speed.y = fall_speed; //limit falling speed
		}
		break;
		case DEAD:
		{
			target_speed.y += gravity * dt;
			if (target_speed.y > fall_speed) target_speed.y = fall_speed; //limit falling speed
		}
		break;
		case CHARGE:
		{
			if (charge_value < max_charge)
				charge_value += charge_increment * dt;
		}
		break;
	}
	if (charge)
	{
		if (charge_value < charged_time)
			charge_value += charge_increment * dt;
	}

	velocity = ((target_speed * acceleration + velocity * (1 - acceleration))*dt);
	velocity.x = velocity.x / (scale_X/1.2f);
	StepY();
	StepX();

	animation_frame = animations[state == GOD? (int)JUMPING:state].GetCurrentFrame(dt);

	return true;
}

void j1Player::IdleUpdate() 
{
	target_speed.x = 0.0F;
	if (App->input->GetKey(SDL_SCANCODE_D) != App->input->GetKey(SDL_SCANCODE_A)) state = MOVING;
	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT)
	{
		charge = true;
		if (charge_value >= charged_time)
		{
			state = CHARGE;
			charge = false;
		}
	}
	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		target_speed.y = -jump_speed;
		is_grounded = false;
		state = JUMPING;
		charge_value = 0.0F;
		Jump(0.0F);
	}
		
	if (!is_grounded) state = JUMPING;
}

void j1Player::MovingUpdate() 
{
	if (App->input->GetKey(SDL_SCANCODE_D) == App->input->GetKey(SDL_SCANCODE_A))
	{
		state = IDLE;
		target_speed.x = 0.0F;
	}
	else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)	
	{
		target_speed.x = movement_speed;
		flipX = true;
	}
	else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		target_speed.x = -movement_speed;
		flipX = false;
	}

	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT)
	{
		charge = true;
		if (charge_value >= charged_time)
		{
			state = CHARGE;
			charge = false;
		}
	}
	else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		target_speed.y = -jump_speed;
		is_grounded = false;
		state = JUMPING;
		charge_value = 0.0F;
		Jump(0.0F);
	}

	if (!is_grounded) state = JUMPING;
}

void j1Player::JumpingUpdate() 
{
	if (App->input->GetKey(SDL_SCANCODE_D) == App->input->GetKey(SDL_SCANCODE_A))
	{
		target_speed.x = 0.0F;
		boost_x = 0.0F;
	}
	else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
	{
		if (target_speed.x < 0.0F) boost_x = 0.0F;
		target_speed.x = movement_speed + boost_x;
		flipX = true;
	}
	else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		if (target_speed.x > 0.0F) boost_x = 0.0F;
		target_speed.x = -movement_speed - boost_x;
		flipX = false;
	}

	if (is_grounded) 
	{
		if (App->input->GetKey(SDL_SCANCODE_D) == App->input->GetKey(SDL_SCANCODE_A)) state = IDLE;
		else state = MOVING;

		target_speed.y = 0.0F;
		velocity.y = 0.0F;
		boost_x = 0.0F;
	}
}

void j1Player::ChargingUpdate()
{
	target_speed.x = 0.0F;
	
	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_UP)
	{
		if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) 
		{
			boost_x = charge_value;
			Jump(charge_value/2.0F); //since we jump diagonal the y jump force is halved
		}
		else Jump(charge_value);
	}
	else if (!is_grounded)
	{
		state = JUMPING;
		charge_value = 0.0F;
	}
}

void j1Player::Jump(float boost_y)
{
	target_speed.y = (-jump_speed - boost_y)/ (scale_X / 1.2f);
	is_grounded = false;
	state = JUMPING;
	charge_value = 0;
	App->audio->PlayFx(jump_fx);
}

void j1Player::CheckDeath()
{
	if(App->map->map_loaded && position.y > App->map->data.height * App->map->data.tile_height && state != DEAD && state != GOD)
		Die();
}


void j1Player::GodUpdate()
{
	if (App->input->GetKey(SDL_SCANCODE_D) == App->input->GetKey(SDL_SCANCODE_A)) target_speed.x = 0.0F;
	else if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
	{
		target_speed.x = movement_speed;
		flipX = true;
	}
	else if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		target_speed.x = -movement_speed;
		flipX = false;
	}
	if (App->input->GetKey(SDL_SCANCODE_W) == App->input->GetKey(SDL_SCANCODE_S))
		target_speed.y = 0.0F;
	if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) 
		target_speed.y = -movement_speed;
	else if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
		target_speed.y = movement_speed;
}


void j1Player::OnCollision(Collider* c1, Collider* c2)
{
	if (c2->type == COLLIDER_ENEMY && state != GOD)
	{
		if (c2->rect.h < this->animation_frame.h && c2->rect.w < this->animation_frame.w)
		{
			if (c2->enabled)
			{
				c2->enabled = false;
				score++;
				ScaleEntity(0.2F, 0.2F);
			}			
		}
		else
			Die();
	}
}

void j1Player::Die()
{
	App->entitymanager->Reagroup();
	j1Entity::Die();
	lives--;
	ResetScale();
	if (lives > 0)
		App->swap_scene->FadeToBlack(1.0F);
	else
		App->scene->GameOver();
}

void j1Player::ResetLives()
{
	lives = starting_lives;
	score = 0;
}

void j1Player::ResetScale()
{
	scale_X = 1.0F;
	scale_Y = 1.0F;

	collider_offset = 40 * scale_Y;
	collider->rect.h = 53 * scale_Y;
	collider->rect.w = 94 * scale_X;
}

bool j1Player::Save(pugi::xml_node &conf) const
{
	j1Entity::Save(conf);
	pugi::xml_node lives = conf.append_child("lives");
	lives.append_attribute("value") = this->lives;
	pugi::xml_node score = conf.append_child("score");
	score.append_attribute("value") = this->score;

	pugi::xml_node scale = conf.append_child("scale");
	scale.append_attribute("value") = this->scale_X;

	return true;
}

bool j1Player::Load(pugi::xml_node &conf)
{
	j1Entity::Load(conf);
	lives = conf.child("lives").attribute("value").as_int();
	score = conf.child("score").attribute("value").as_int();

	float scale_value = conf.child("scale").attribute("value").as_float()-1;
	ResetScale();
	ScaleEntity(scale_value, scale_value);

	return true;
}