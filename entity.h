#pragma once

#include <glm/vec2.hpp>
#include "sprite.h"
#include "tilemap.h"
#include <memory>
#include <vector>

class Level;

enum class ItemType {
	Key
};

enum class MoveState {
	IDLE,
	LEFT,
	RIGHT
};

enum class ClimbState {
	IDLE,
	UP,
	DOWN
};

enum class Orientation {
	LEFT,
	RIGHT
};

enum class DamageType {
	Hit,
	Spikes,
	Water,
	Burn
};

struct Hitbox {
	float w;
	float h;
};

struct CollideInfo {
	bool left = false;
	bool right = false;
	bool top = false;
	bool bottom = true;
	bool bottomLeft = false;
	bool bottomRight = false;
};

enum class EntityType {
	Player,
	Enemy,
	Projectile,
	Item,
	Platform,
	Obstacle,
	Interactable,
	Controlable,
	FX
};

class Entity {
public:
	int id;
	std::string name;
	bool alive;
	int maxHitpoints;
	int hitpoints;
	DamageType diedBy;
	Orientation orientation;
	glm::vec2 position;
	SpriteRegion spriteRegion;
	Animator mAnimator;

	Entity(int id, const Spritesheet& spritesheet, const Hitbox& hitbox, int hitpoints = 1);
	virtual ~Entity() = default;
	virtual void Update(Level& level) = 0;
	void TakeDamage(int amount, DamageType type = DamageType::Hit);
	virtual EntityType GetType() const = 0;
	bool Collide(const Entity& entity) const;
	inline float getLeft() const { return position.x - mHitbox.w * 0.5f; }
	inline float getRight() const { return position.x + mHitbox.w * 0.5f; }
	inline float getTop() const { return position.y + mHitbox.h * 0.5f; }
	inline float getBottom() const { return position.y - mHitbox.h * 0.5f; }
	inline const Spritesheet& getSpritesheet() const { return mSpritesheet; }
	inline Animator& getAnimator() { return mAnimator; }
	inline const Animator& getAnimator() const { return mAnimator; }
	inline const Hitbox& getHitbox() const { return mHitbox; }
	inline bool isDead() const { return hitpoints == 0; }

protected:
	const Spritesheet& mSpritesheet;
	Hitbox mHitbox;
};

class Mover : public Entity {
public:
	MoveState moveState;
	float speed;
	float gravity;
	float maxGravity;
	bool slopesEnabled;
	glm::vec2 velocity;
	int baseSpriteID;

	Mover(const Spritesheet& spritesheet, int baseSpriteID, const Hitbox& hitbox, int hitpoints, float speed, float gravity, bool slopesEnabled = false);

protected:
	CollideInfo mCollideInfo;
	int mSlopeTileIndex;
	int mLedderTileIndex;
	struct {
		int id;
		int prevID;
	} mPlatform;

	void resetCollisionInfo();
	float getSlopeTileY(const Tilemap& tilemap, const Tile& tile, float x) const;
	void collideX(Level& level, bool resolveCollision);
	void collideY(Level& level, bool resolveCollision);
	void updateSlope(const Tilemap& tilemap);
	void updatePlatform(Level& level);
	void jump(float jumpSpeed);
	void handleLinearMovement();
	void updatePhysics(Level& level, bool resolveCollision = true);

private:
	bool isCollisionWithSlopeX(const Tilemap& tilemap, const Tile& tile, int direction) const;
};

class Item : public Entity {
public:
	ItemType type;
	std::string name;

	Item(const std::string& name, ItemType type);
	EntityType GetType() const override { return EntityType::Item; }
	void Update(Level& level) override;
};

class Inventory {
public:
	enum class Mode {
		Idle,
		Select,
		Transfer
	};

	Inventory();
	bool AddItem(const Item& item);
	int GetEmptySlotIdx() const;
	void RemoveItem(int idx);
	void StartSelect();
	void StopSelect();
	void Update(Level& level);
	int GetAnimationFrame() const;
	inline const Item* getItem(int idx) const { return mItems[idx].get(); }
	inline int getSelectedIdx() const { return mSelectedIdx; }
	inline Mode getMode() const { return mMode; }
	inline int getTransferIdx() const { return mTransferIdx; }

private:
	std::vector<std::unique_ptr<Item>> mItems;
	int mSelectedIdx;
	Mode mMode;
	int mTransferIdx;
	double mAnimCounter;

	void endTransfer(Level& level);
};

class Player : public Mover {
public:
	float acceleration;
	float decceleration;
	float climbSpeed;
	float jumpSpeed;
	ClimbState climbState;
	Inventory inventory;

	Player(int baseSpriteID, float speed);
	EntityType GetType() const override { return EntityType::Player; }
	void Update(Level& level) override;
	virtual void HandleInput();
	void ResetInput();

protected:
	bool mClimbStarted;
	bool mIsAction;
	bool mUsingItem;
	bool mJumping;
	bool mPunching;
	bool mPunchStarted;
	bool mShooting;
	bool mShootStarted;
	bool mFloatChanged;
	bool mFloating;

private:
	void handleAnimation();
	bool findNearbyLedder(const Tilemap& tilemap, Tile& out);
	bool useItem(Level& level, const Item* item);
	glm::vec2 getHandPosition() const;
};

class Ralph : public Player {
public:
	Ralph();
	void HandleInput() override;
};

class Gregory : public Player {
public:
	Gregory();
	void HandleInput() override;
};

class Enemy : public Mover {
public:
	Enemy(int baseSpriteID, const Hitbox& hitbox, int hitpoints, float speed, float gravity, bool slopesEnabled = false);
	EntityType GetType() const override { return EntityType::Enemy; }
};

class Slime : public Enemy {
public:
	Slime();
	void Update(Level& level) override;
};

class Bat : public Enemy {
public:
	Bat();
	void Update(Level& level) override;
};

class Dragon : public Enemy {
private:
	bool mFiring;
	double mFireCounter;

	void Fire(Level& level);
public:
	Dragon();
	void Update(Level& level) override;
};

enum class ProjectileType {
	Fireball,
	PlasmaBullet
};

class Projectile : public Mover {
public:
	Projectile(int baseSpriteID, const Hitbox& hitbox, int dmg, DamageType damageType, float speed, float gravity, double lifetime, bool isPlayer = false);
	EntityType GetType() const override { return EntityType::Projectile; }
	void Update(Level& level) override;

private:
	int mDamage;
	DamageType mDamageType;
	double mLifetime;
	bool mIsPlayer;
};

class Fireball : public Projectile {
public:
	Fireball();
};

class PlasmaBullet : public Projectile {
public:
	PlasmaBullet();
};

class Controlable : public Entity {
public:
	Controlable(int id, float w, float h, bool active = false);
	virtual EntityType GetType() const override { return EntityType::Controlable; };
	virtual void Update(Level& level) override;
	void Activate();
	void Deactivate();
	inline bool IsActive() const { return mIsActive; }

private:
	bool mIsActive;
};

class Door : public Controlable {
public:
	Door(int id, float w, float h);
	void Update(Level& level) override;

private:
	bool mIsOpened;
	double mCounter;
	bool mOpenStarted;
	int mOpenProgress;
};

class Bridge : public Controlable {
public:
	Bridge(int id, float w, float h, int direction, int tileID);
	void Update(Level& level) override;

private:
	int mDirection;
	int mTileID;
	bool mIsOpened;
	double mCounter;
	bool mOpenStarted;
	int mOpenProgress;
};

class Spawner : public Controlable {
public:
	Spawner(int id, ProjectileType projectileType, int reloadTime, int direction, bool active = true);
	void Update(Level& level) override;

private:
	ProjectileType mProjectileType;
	int mReloadTime;
	double mCounter;
	int mDirection;

	void spawn(Level& level);
};

class Platform : public Controlable {
public:
	float speed;
	glm::vec2 velocity;

	Platform(int id, float speed, bool solid, bool active, const std::vector<glm::vec2>& points);
	EntityType GetType() const override { return EntityType::Platform; }
	void Update(Level& level) override;

private:
	bool mIsSolid;
	int mPointIndex;
	std::vector<glm::vec2> mPoints;
};

class Obstacle : public Mover {
public:
	Obstacle();
	EntityType GetType() const override { return EntityType::Obstacle; }
	void Update(Level& level) override;
};

class Interactable : public Entity {
public:
	struct Target {
		int id = 0;
		EntityType type = EntityType::Controlable;
	};

	bool projectileTrigger;

	Interactable(Target target, bool projectileTrigger = true);
	EntityType GetType() const override { return EntityType::Interactable; };
	virtual void Update(Level& level) override;
	virtual bool OnAction(Level& level);
	virtual bool OnItem(Level& level, const Item& item);

protected:
	void OnTarget(Level& level, bool active) const;

private:
	Target mTarget;
};

class Lever : public Interactable {
public:
	bool isUp;

	Lever(Target target, bool isUp = true);
	bool OnAction(Level& level) override;

private:
	void handleSprite();
};

class Button : public Interactable {
public:
	Button(Target target);
	void Update(Level& level) override;
	bool OnAction(Level& level) override;

private:
	double mPressedDownCounter;
};

class DoorLock : public Interactable {
public:
	std::string color;

	DoorLock(Target target, const std::string& color);
	bool OnItem(Level& level, const Item& item) override;
};

class HelpBlock : public Interactable {
public:	
	std::string text;

	HelpBlock(const std::string& text);
	bool OnAction(Level& level) override;
};

class FXEffect : public Entity {
public:
	FXEffect(int animationID);
	EntityType GetType() const override { return EntityType::FX; }
	void Update(Level& level) override;
};