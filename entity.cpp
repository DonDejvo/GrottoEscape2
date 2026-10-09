#include "entity.h"
#include <iostream>
#include "game_state.h"
#include "engine.h"
#include "constants.h"
#include "animation.h"

const float GRAVITY = 0.05f;
const float MAX_VELOCITY = 2.8f;
const float EPSILON = 0.01f;
const int INVENTORY_ANIM_FRAME_DURATION = 250;

Entity::Entity(int id, const Spritesheet& spritesheet, const Hitbox& hitbox, int hitpoints) :
	id(id),
	mSpritesheet(spritesheet),
	alive(true),
	mHitbox(hitbox),
	hitpoints(hitpoints),
	maxHitpoints(hitpoints),
	diedBy(DamageType::Hit),
	position(0, 0),
	orientation(Orientation::RIGHT)
{ }

void Entity::TakeDamage(int amount, DamageType type)
{
	hitpoints = std::max(0, hitpoints - amount);
	if (hitpoints == 0) {
		diedBy = type;
	}
}

bool Entity::Collide(const Entity & entity) const
{
	return (getRight() - entity.getLeft()) * (getLeft() - entity.getRight()) <= 0 &&
		(getTop() - entity.getBottom()) * (getBottom() - entity.getTop()) <= 0;
}

Mover::Mover(const Spritesheet& spritesheet, int baseSpriteID, const Hitbox& hitbox, int hitpoints, float speed, float gravity, bool slopesEnabled)
	: Entity(0, spritesheet, hitbox, hitpoints),
	speed(speed),
	gravity(gravity),
	maxGravity(MAX_VELOCITY),
	slopesEnabled(slopesEnabled),
	mSlopeTileIndex(-1),
	mLedderTileIndex(-1),
	mPlatform(0, 0),
	moveState(MoveState::IDLE),
	velocity(0, 0),
	baseSpriteID(baseSpriteID)
{ 
	spriteRegion = mSpritesheet.GetRegion(baseSpriteID);
	mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID), true, false);
}

void Mover::resetCollisionInfo()
{
	mCollideInfo.left = false;
	mCollideInfo.right = false;
	mCollideInfo.top = false;
	mCollideInfo.bottom = false;
	mCollideInfo.bottomLeft = false;
	mCollideInfo.bottomRight = false;
}

float Mover::getSlopeTileY(const Tilemap& tilemap, const Tile& tile, float x) const
{
	float x1 = tile.data->slope.y0 < tile.data->slope.y1 ? x + mHitbox.w * 0.5f : x - mHitbox.w * 0.5f;

	float x0 = (tile.x - 0.5f) * tilemap.getTileWidth();
	float y0 = (tile.y - 0.5f + tile.data->slope.y0) * tilemap.getTileHeight();
	float y1 = (tile.y - 0.5f + tile.data->slope.y1) * tilemap.getTileHeight();

	float t = std::min(std::max((x1 - x0) / tilemap.getTileWidth(), 0.0f), 1.0f);
	return y0 + (y1 - y0) * t;
}

void Mover::collideX(Level& level, bool resolveCollision)
{
	auto resolve = [this, resolveCollision](float posX) {
		if (resolveCollision) {
			position.x = posX;
			velocity.x = 0;
		}
		};
	const Tilemap& tilemap = level.tilemap;
	const TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	int y0 = tilemap.getRowIdx(getBottom());
	int y1 = tilemap.getRowIdx(getTop() - EPSILON);

	if (velocity.x < 0) {
		int x = tilemap.getColIdx(getLeft());
		std::vector<Tile> tiles;
		layer.GetTileData(tiles, x, y0, x, y1);

		float posX;
		for (const auto& tile : tiles) {
			if (!tile.data->collide || (tile.data->type == TileData::Type::Slope && !isCollisionWithSlopeX(tilemap, tile, -1))) continue;
			posX = (tile.x + 0.5f) * tilemap.getTileWidth() + mHitbox.w * 0.5f;
			if (position.x <= posX) {
				mCollideInfo.left = true;
				resolve(posX);
				break;
			}
		}
		if (position.x <= (posX = -0.5f * tilemap.getTileWidth() + mHitbox.w * 0.5f)) {
			mCollideInfo.left = true;
			resolve(posX);
		}
	}
	else if (velocity.x > 0) {
		int x = tilemap.getColIdx(getRight());
		std::vector<Tile> tiles;
		layer.GetTileData(tiles, x, y0, x, y1);

		float posX;
		for (const auto& tile : tiles) {
			if (!tile.data->collide || (tile.data->type == TileData::Type::Slope && !isCollisionWithSlopeX(tilemap, tile, 1))) continue;
			posX = (tile.x - 0.5f) * tilemap.getTileWidth() - mHitbox.w * 0.5f;
			if (position.x >= posX) {
				mCollideInfo.right = true;
				resolve(posX);
				break;
			}
		}
		if (position.x >= (posX = (tilemap.getWidth() - 0.5f) * tilemap.getTileWidth() - mHitbox.w * 0.5f)) {
			mCollideInfo.right = true;
			resolve(posX);
		}
	}
}

void Mover::collideY(Level& level, bool resolveCollision)
{
	auto resolve = [this, resolveCollision](float posY) {
		if (resolveCollision) {
			position.y = posY;
			velocity.y = 0;
		}
		};
	const Tilemap& tilemap = level.tilemap;
	const auto& platforms = level.platforms;
	const TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	int x0 = tilemap.getColIdx(getLeft());
	int x1 = tilemap.getColIdx(getRight() - EPSILON);

	int y0 = tilemap.getRowIdx(getBottom());
	int y1 = tilemap.getRowIdx(position.y);
	std::vector<Tile> tiles;
	float posY = 0;
	layer.GetTileData(tiles, x0, y0, x1, y1);
	tilemap.getTileLayer(TILE_LAYER_BACKGROUND).GetTileData(tiles, x0, y0, x1, y1);

	if (slopesEnabled) {
		for (const auto& tile : tiles) {
			if (tile.data->type != TileData::Type::Slope) continue;
			posY = getSlopeTileY(tilemap, tile, position.x) + mHitbox.h * 0.5f;
			if (position.y <= posY) {
				mCollideInfo.bottom = true;
				resolve(posY);
				if (resolveCollision) {
					mSlopeTileIndex = tile.x + tile.y * layer.getWidth();
				}
			}
		}
	}

	if (velocity.y <= 0) {
		for (const auto& tile : tiles) {
			if (!tilemap.isGroundTile(tile, mLedderTileIndex == -1) || tile.data->type == TileData::Type::Slope) continue;

			posY = (tile.y + 0.5f) * tilemap.getTileHeight() + mHitbox.h * 0.5f;
			if (position.y <= posY && ((tile.data->type != TileData::Type::Ledder && 
				tile.data->type != TileData::Type::Platform) || getBottom() - velocity.y >= (tile.y + 0.5f) * tilemap.getTileHeight())) {
				mCollideInfo.bottom = true;
				resolve(posY);
			}
		}
		if (position.y <= (posY = -0.5f * tilemap.getTileHeight() + mHitbox.h * 0.5f)) {
			mCollideInfo.bottom = true;
			resolve(posY);
		}

		if (mCollideInfo.bottom) {
			for (const auto& tile : tiles) {
				if (!tilemap.isGroundTile(tile, mLedderTileIndex == -1) || tile.data->type == TileData::Type::Slope) continue;
				if (tile.x == x0) {
					mCollideInfo.bottomLeft = true;
				}
				if (tile.x == x1) {
					mCollideInfo.bottomRight = true;
				}
			}
		}

		if (mPlatform.id == 0) {
			for (const auto& platform : platforms) {
				if (platform->id == mPlatform.prevID || !Collide(*platform)) continue;

				if (getBottom() - velocity.y >= platform->position.y - platform->velocity.y) {
					mPlatform.id = platform->id;
					mPlatform.prevID = 0;
					mCollideInfo.bottom = true;
					resolve(platform->getTop() + mHitbox.h * 0.5f);
				}
			}
		}
	}
	else if(velocity.y > 0) {
		y0 = y1 = tilemap.getRowIdx(getTop());
		tiles.clear();
		layer.GetTileData(tiles, x0, y0, x1, y1);

		for (const auto& tile : tiles) {
			if (!tile.data->collide || tile.data->type == TileData::Type::Slope) continue;
			posY = (tile.y - 0.5f) * tilemap.getTileHeight() - mHitbox.h * 0.5f;
			if (position.y >= posY) {
				mCollideInfo.top = true;
				resolve(posY);
				break;
			}
		}
	}
}

void Mover::updateSlope(const Tilemap& tilemap)
{
	if (mSlopeTileIndex == -1) return;

	int prevSlopeTileIndex = mSlopeTileIndex;
	const TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	const Tile& slopeTile = layer.GetTile(mSlopeTileIndex);

	if ((getLeft() - (slopeTile.x + 0.5f) * tilemap.getTileWidth()) *
		(getRight() - (slopeTile.x - 0.5f) * tilemap.getTileWidth()) > 0) {
		int x0 = tilemap.getColIdx(getLeft());
		int x1 = tilemap.getColIdx(getRight() - EPSILON);
		int y = tilemap.getRowIdx(getBottom());

		std::vector<Tile> tiles;
		layer.GetTileData(tiles, x0, y - 1, x1, y);

		float maxY = 0;
		int idx = -1;

		for (int i = 0; i < tiles.size(); ++i) {
			if (tiles[i].data->type != TileData::Type::Slope) continue;

			float y = getSlopeTileY(tilemap, tiles[i], position.x);
			if (y > maxY) {
				maxY = y;
				idx = i;
			}
		}
		mSlopeTileIndex = idx == -1 ? idx : tiles[idx].x + tiles[idx].y * layer.getWidth();
	}
	
	if (mSlopeTileIndex != -1) {
		const Tile& slopeTile = tilemap.getTileLayer(TILE_LAYER_MIDGROUND).GetTile(mSlopeTileIndex);
		position.y = getSlopeTileY(tilemap, slopeTile, position.x) + 0.5f * mHitbox.h;
	}
	else {
		const Tile& slopeTile = tilemap.getTileLayer(TILE_LAYER_MIDGROUND).GetTile(prevSlopeTileIndex);
		float y = position.x < slopeTile.x * tilemap.getTileWidth() ? slopeTile.data->slope.y0 : slopeTile.data->slope.y1;
		position.y = (slopeTile.y - 0.5f + y) * tilemap.getTileHeight() + mHitbox.h * 0.5f;
	}

	mCollideInfo.bottom = true;
}

void Mover::updatePlatform(Level& level)
{
	if (mPlatform.prevID) {
		const Platform& platform = *level.GetPlatformByID(mPlatform.prevID);
		float bottom = getBottom();
		if (bottom < platform.position.y || bottom >= platform.getTop()) {
			mPlatform.prevID = 0;
		}
	}

	if (mPlatform.id == 0) return;

	const Platform& platform = *level.GetPlatformByID(mPlatform.id);
	position.x += platform.velocity.x;
	if ((getLeft() - platform.getRight()) * (getRight() - platform.getLeft()) <= 0) {
		position.y = platform.getTop() + mHitbox.h * 0.5f;
		velocity.y = 0;
		mCollideInfo.bottom = true;
	}
	else {
		mPlatform.prevID = mPlatform.id;
		mPlatform.id = 0;
	}
}

void Mover::jump(float jumpSpeed)
{
	mCollideInfo.bottom = false;
	mSlopeTileIndex = -1;
	mLedderTileIndex = -1;
	mPlatform.id = 0;
	velocity.y = jumpSpeed;
}

void Mover::handleLinearMovement()
{
	if (moveState == MoveState::LEFT) {
		orientation = Orientation::LEFT;
		velocity.x = -speed;
	}
	else if (moveState == MoveState::RIGHT) {
		orientation = Orientation::RIGHT;
		velocity.x = speed;
	}
	else {
		velocity.x = 0;
	}
}

void Mover::updatePhysics(Level& level, bool resolveCollision)
{
	if (maxGravity > 0.0f) {
		velocity.y = std::max(-maxGravity, velocity.y - gravity);
	}
	
	resetCollisionInfo();
	position.x += velocity.x;
	updatePlatform(level);
	updateSlope(level.tilemap);
	collideX(level, resolveCollision);
	position.y += velocity.y;
	collideY(level, resolveCollision);
}

bool Mover::isCollisionWithSlopeX(const Tilemap& tilemap, const Tile& tile, int direction) const
{
	if (!slopesEnabled) {
		return true;
	}

	return false;
}

Item::Item(const std::string& name, ItemType type)
	: Entity(0, Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_ITEMS), { 6, 6 }),
	name(name),
	type(type)
{
	if (name == "key_red") {
		spriteRegion = mSpritesheet.GetRegion(1);
	}
	else if (name == "key_blue") {
		spriteRegion = mSpritesheet.GetRegion(2);
	} 
	else if (name == "key_yellow") {
		spriteRegion = mSpritesheet.GetRegion(3);
	}
	
}

void Item::Update(Level& level)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		Player* player = level.players[i].get();
		if (player && player->Collide(*this)) {
			alive = false;
			player->inventory.AddItem(*this);
		}
	}
}

Inventory::Inventory()
	: mItems(INVETORY_SIZE),
	mSelectedIdx(0),
	mAnimCounter(0),
	mMode(Mode::Idle),
	mTransferIdx(-1)
{
}

bool Inventory::AddItem(const Item& item)
{
	for (int i = 0; i < INVETORY_SIZE; ++i) {
		if (mItems[i] == nullptr) {
			mItems[i] = std::make_unique<Item>(item.name, item.type);
			return true;
		}
	}
	return false;
}

int Inventory::GetEmptySlotIdx() const
{
	for (int i = 0; i < INVETORY_SIZE; ++i) {
		if (mItems[i] == nullptr) {
			return i;
		}
	}
	return -1;
}

void Inventory::RemoveItem(int idx)
{
	mItems[idx] = nullptr;
}

void Inventory::StartSelect()
{
	mMode = Mode::Select;
	mAnimCounter = 0;
}

void Inventory::StopSelect()
{
	mMode = Mode::Idle;
}

void Inventory::Update(Level& level)
{
	if (mMode == Mode::Idle) return;

	mAnimCounter += 1000 / 60.0;
	if (mAnimCounter >= INVENTORY_ANIM_FRAME_DURATION * 2) {
		mAnimCounter = 0;
	}

	Input& input = Engine::GetInput();
	if (input.IsClicked(Input::BUTTON_SELECT)) {
		if (mMode == Mode::Transfer) {
			endTransfer(level);
		}
		mMode = Mode::Idle;
	}
	else if (input.IsClicked(Input::BUTTON_B)) {
		if (mMode == Mode::Select && mItems[mSelectedIdx]) {
			mMode = Mode::Transfer;
			mAnimCounter = 0;
		}
		else if (mMode == Mode::Transfer) {
			mMode = Mode::Select;
			mAnimCounter = 0;
			endTransfer(level);
		}
	}
	else if (mMode == Mode::Select) {
		int col = mSelectedIdx % INVENTORY_COLS;
		int row = mSelectedIdx / INVENTORY_COLS;
		if (input.IsClicked(Input::DPAD_LEFT)) {
			col = std::max(0, col - 1);
		}
		else if (input.IsClicked(Input::DPAD_RIGHT)) {
			col = std::min(col + 1, INVENTORY_COLS - 1);
		}
		else if (input.IsClicked(Input::DPAD_UP)) {
			row = std::max(0, row - 1);
		}
		else if (input.IsClicked(Input::DPAD_DOWN)) {
			row = std::min(row + 1, INVENTORY_ROWS - 1);
		}
		mSelectedIdx = row * INVENTORY_COLS + col;
	}
	else {
		if (input.IsClicked(Input::DPAD_LEFT) || input.IsClicked(Input::DPAD_RIGHT)) {
			int direction = input.IsClicked(Input::BUTTON_RIGHT) ? 1 : -1;
			const glm::vec2& selectedPlayerPos = level.players[level.GetSelectedPlayerIdx()]->position;
			bool found = false;
			int currentPlayerIdx = mTransferIdx == -1 ? level.GetSelectedPlayerIdx() : mTransferIdx;
			for (int i = 1; i < MAX_PLAYERS; ++i) {
				int nextPlayerIdx = (level.GetSelectedPlayerIdx() + MAX_PLAYERS + i * direction) % MAX_PLAYERS;
				if (level.isPlayerAlive(nextPlayerIdx) && currentPlayerIdx != nextPlayerIdx && glm::distance(level.players[nextPlayerIdx]->position, selectedPlayerPos) < 32.0f) {
					mTransferIdx = nextPlayerIdx;
					found = true;
					break;
				}
			}
			if (!found) {
				mTransferIdx = -1;
			}
		}
	}
}

int Inventory::GetAnimationFrame() const
{
	return mAnimCounter < INVENTORY_ANIM_FRAME_DURATION ? 0 : 1;
}

void Inventory::endTransfer(Level& level)
{
	if (mTransferIdx != -1 && level.players[mTransferIdx]) {
		level.players[mTransferIdx]->inventory.AddItem(*mItems[mSelectedIdx]);
		RemoveItem(mSelectedIdx);
	}
	mTransferIdx = -1;
}

Player::Player(int baseSpriteID, float speed)
	: Mover(Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_PLAYER), baseSpriteID, { 8, 16 }, 3, speed, GRAVITY, true),
	acceleration(0.06f),
	decceleration(0.32f),
	climbSpeed(0.6f),
	climbState(ClimbState::IDLE),
	mIsAction(false),
	mClimbStarted(false),
	mUsingItem(false),
	jumpSpeed(1.9f),
	mJumping(false),
	mPunching(false),
	mPunchStarted(false),
	mShooting(false),
	mShootStarted(false),
	mFloating(false),
	mFloatChanged(false)
{
}

void Player::Update(Level& level)
{
	handleAnimation();

	if (isDead()) {
		velocity.x = 0;
		mLedderTileIndex = -1;
		mFloating = false;
	}
	else {
		Tilemap& tilemap = level.tilemap;

		if (mPunchStarted) {
			velocity.x = 0;

			if (!mAnimator.IsPlaying()) {
				mPunchStarted = false;
				glm::vec2 pos = getHandPosition();

				TileLayer& midground = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
				midground.BreakTile(
					tilemap.getColIdx(pos.x),
					tilemap.getRowIdx(pos.y));
			}
		}
		else if (mShootStarted) {
			velocity.x = 0;

			if (!mAnimator.IsPlaying()) {
				mShootStarted = false;

				std::unique_ptr<PlasmaBullet> bullet = std::make_unique<PlasmaBullet>();
				bullet->position = getHandPosition();
				bullet->moveState = orientation == Orientation::LEFT ? MoveState::LEFT : MoveState::RIGHT;
				level.AddEntity(std::move(bullet));
			}
		}
		else if (mLedderTileIndex == -1) {
			float minSpeed = (mCollideInfo.bottom ? 0.25f : 0.5f) * speed;
			float frameAcceleration = (mFloating ? 0.6f : 1.0f) * acceleration;
			float frameDecceleration = (mCollideInfo.bottom ? mFloating ? 0.5f : 1.0f : 0.1f) * decceleration;
			if (moveState == MoveState::LEFT) {
				orientation = Orientation::LEFT;
				velocity.x = std::min(std::max(velocity.x - frameAcceleration, -speed), minSpeed);
			}
			else if (moveState == MoveState::RIGHT) {
				orientation = Orientation::RIGHT;
				velocity.x = std::min(std::max(velocity.x + frameAcceleration, -minSpeed), speed);
			}
			else {
				float vx = abs(velocity.x);
				if (vx > 0) {
					float dx = frameDecceleration;
					velocity.x = ((velocity.x > 0.0f) - (velocity.x < 0.0f)) * std::max(vx - dx, 0.0f);
				}
			}

			if (mFloatChanged) {
				mFloating = !mFloating;
			}

			if (climbState != ClimbState::IDLE && (mClimbStarted || mCollideInfo.bottom)) {
				Tile ledderTile;
				if (findNearbyLedder(tilemap, ledderTile)) {
					bool isValidLedder = true;

					if (mCollideInfo.bottom && climbState == ClimbState::DOWN) {
						isValidLedder = !tilemap.isGroundLedderTile(ledderTile);
					}
					else if (mCollideInfo.bottom && climbState == ClimbState::UP) {
						bool isTop = tilemap.isTopLedderTile(ledderTile);
						isValidLedder = !isTop;
					}

					if (isValidLedder) {
						mLedderTileIndex = ledderTile.y * tilemap.getWidth() + ledderTile.x;
						velocity.x = 0;
						velocity.y = 0;
						position.x = ledderTile.x * tilemap.getTileWidth();
					}
				}
			}
		}
		else {
			Tile ledderTile;
			bool isClimbing = findNearbyLedder(tilemap, ledderTile);
			if (isClimbing) {
				mLedderTileIndex = ledderTile.y * tilemap.getWidth() + ledderTile.x;

				if (climbState == ClimbState::UP) {
					velocity.y = climbSpeed;
				}
				else if (climbState == ClimbState::DOWN) {
					velocity.y = -climbSpeed;
					if (mCollideInfo.bottom) {
						mLedderTileIndex = -1;
					}
				}
				else {
					velocity.y = 0;
					if (moveState != MoveState::IDLE) {
						mLedderTileIndex = -1;
					}
				}
			}
			else {
				if (climbState == ClimbState::UP) {
					position.y = (mLedderTileIndex / tilemap.getWidth() + 0.5f) * tilemap.getTileHeight() + mHitbox.h * 0.5f;
				}
				velocity.y = 0;
				mLedderTileIndex = -1;
			}
		}

		if (mIsAction) {
			Interactable* fixture = level.GetOverlappingFixture(*this);
			if (fixture) {
				fixture->OnAction(level);
			}
		}

		if (mUsingItem) {
			int idx = inventory.getSelectedIdx();
			const Item* item = inventory.getItem(idx);
			if (useItem(level, item)) {
				inventory.RemoveItem(idx);
			}
		}

		if (mJumping && !mPunchStarted && !mShootStarted && (mCollideInfo.bottom || (mLedderTileIndex != -1 && climbState == ClimbState::IDLE))) {
			jump(jumpSpeed * (0.1f * abs(velocity.x) / speed + 0.9f));
		}

		if (mCollideInfo.bottom && !mFloating) {
			if (mPunching && !mPunchStarted) {
				mPunchStarted = true;

				std::unique_ptr<FXEffect> effect = std::make_unique<FXEffect>(21);
				effect->position = getHandPosition();
				level.AddEntity(std::move(effect));
			}
			else if (mShooting && !mShootStarted) {
				mShootStarted = true;
			}
		}

		int x0 = tilemap.getColIdx(getLeft()),
			x1 = tilemap.getColIdx(getRight()),
			y0 = tilemap.getRowIdx(getBottom()),
			y1 = tilemap.getRowIdx(getTop());

		std::vector<Tile> tiles;
		tilemap.getTileLayer(TILE_LAYER_BACKGROUND).GetTileData(tiles, x0, y0, x1, y1);
		tilemap.getTileLayer(TILE_LAYER_MIDGROUND).GetTileData(tiles, x0, y0, x1, y1);

		for (const auto& tile : tiles) {
			float h = 1.0f;
			switch (tile.data->type)
			{
			case TileData::Type::Spikes:
				h = 0.5f;
				break;
			case TileData::Type::Lava:
				h = 0.75f;
				break;
			default:
				break;
			}
			float posY = (tile.y - 0.5f + h) * tilemap.getTileHeight() + mHitbox.w * 0.5f;
			if (position.y > posY) continue;
			switch (tile.data->type)
			{
			case TileData::Type::Spikes:
				TakeDamage(999, DamageType::Spikes);
				break;
			case TileData::Type::Lava:
				TakeDamage(999, DamageType::Burn);
				break;
			case TileData::Type::Water:
				TakeDamage(999, DamageType::Water);
				break;
			default:
				break;
			}
			if (isDead()) {
				if (diedBy == DamageType::Water || diedBy == DamageType::Burn || diedBy == DamageType::Spikes) {
					position.y = posY;
					velocity.y = 0;
				}
				break;
			}
		}
	}

	float gravityScale = 1.0f;
	if (mLedderTileIndex != -1 || diedBy == DamageType::Water || diedBy == DamageType::Burn || diedBy == DamageType::Spikes) {
		gravityScale = 0.0f;
	}
	else if (mFloating) {
		gravityScale = 0.2f;
	}
	maxGravity = gravityScale * MAX_VELOCITY;
	updatePhysics(level);
}

void Player::HandleInput()
{
	const Input& input = Engine::GetInput();

	if (input.IsClicked(Input::BUTTON_SELECT)) {
		inventory.StartSelect();
	}

	mIsAction = input.IsClicked(Input::BUTTON_A);
	mUsingItem = input.IsClicked(Input::BUTTON_X);

	if (input.IsPressed(Input::DPAD_LEFT)) {
		moveState = MoveState::LEFT;
	}
	else if (input.IsPressed(Input::DPAD_RIGHT)) {
		moveState = MoveState::RIGHT;
	}
	else {
		moveState = MoveState::IDLE;
	}
	
	if(input.IsPressed(Input::DPAD_UP)) {
		climbState = ClimbState::UP;
	}
	else if (input.IsPressed(Input::DPAD_DOWN)) {
		climbState = ClimbState::DOWN;
	}
	else {
		climbState = ClimbState::IDLE;
	}
	mClimbStarted = input.IsClicked(Input::DPAD_DOWN) || input.IsClicked(Input::DPAD_UP);
}

void Player::ResetInput()
{
	mIsAction = false;
	mUsingItem = false;
	moveState = MoveState::IDLE;
	climbState = ClimbState::IDLE;
	mClimbStarted = false;
	mJumping = false;
	mPunching = false;
	mShooting = false;
	mFloatChanged = false;
}

void Player::handleAnimation()
{
	if (isDead()) {
		switch (diedBy)
		{
		case DamageType::Hit:
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_DIE), false, false);
			break;
		case DamageType::Spikes:
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_SPIKES), false, false);
			break;
		case DamageType::Water:
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_DROWN), false, false);
			break;
		case DamageType::Burn:
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_BURN), false, false);
			break;
		default:
			break;
		}
	}
	else if (mLedderTileIndex != -1) {
		if (climbState == ClimbState::IDLE) {
			mAnimator.Pause();
		}
		else {
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_CLIMB), true, false);
		}
	}
	else if (mFloating) {
		mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_FLOAT), true, false);
	}
	else if (!mCollideInfo.bottom) {
		mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_JUMP), true, false);
	}
	else {
		if (mPunchStarted || mShootStarted) {
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_SHOOT), false, false);
		}
		else if (moveState == MoveState::IDLE) {
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_IDLE), true, false);
		}
		else {
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID - 1 + PLAYER_ANIM_RUN), true, false);
		}
	}
	mAnimator.Update();
}

bool Player::findNearbyLedder(const Tilemap& tilemap, Tile& out)
{
	const TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_BACKGROUND);

	int x = tilemap.getColIdx(position.x);
	int y0 = tilemap.getRowIdx(getBottom() - EPSILON);
	int y1 = tilemap.getRowIdx(position.y + mHitbox.h * 0.25f);

	layer.forEach(x, y0, x, y1, [&out](const Tile& tile, const TileLayer&) {
		if (tile.data && tile.data->type == TileData::Type::Ledder) {
			out.id = tile.id;
			out.x = tile.x;
			out.y = tile.y;
		}
		});
	
	return out.id != 0;
}

bool Player::useItem(Level& level, const Item* item)
{
	if (item == nullptr) return false;
	switch (item->type) {
	case ItemType::Key:
		Interactable* fixture = level.GetOverlappingFixture(*this);
		if (fixture) {
			if (fixture->OnItem(level, *item)) {
				return true;
			}
		}
		break;
	}
	return false;
}

glm::vec2 Player::getHandPosition() const
{
	float range = 8.0f;
	return glm::vec2(orientation == Orientation::LEFT ? position.x - range : position.x + range, position.y);
}

Ralph::Ralph() 
	: Player(1, 1.4f)
{
}

void Ralph::HandleInput()
{
	const Input& input = Engine::GetInput();
	mJumping = input.IsClicked(Input::BUTTON_B);
	mPunching = input.IsClicked(Input::BUTTON_Y);
	Player::HandleInput();
}

Gregory::Gregory()
	: Player(61, 1.2f)
{
}

void Gregory::HandleInput()
{
	const Input& input = Engine::GetInput();
	mFloatChanged = input.IsClicked(Input::BUTTON_B);
	mShooting = input.IsClicked(Input::BUTTON_Y);
	Player::HandleInput();
}

Enemy::Enemy(int baseSpriteID, const Hitbox& hitbox, int hitpoints, float speed, float gravity, bool slopesEnabled)
	: Mover(Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_ENEMIES), baseSpriteID, hitbox, hitpoints, speed, gravity, slopesEnabled)
{
}

Slime::Slime()
	: Enemy(1, { 16, 16 }, 2, 0.4f, GRAVITY)
{
	moveState = MoveState::LEFT;
}

void Slime::Update(Level& level)
{
	if (mCollideInfo.left || !mCollideInfo.bottomLeft) {
		moveState = MoveState::RIGHT;
	}
	else if (mCollideInfo.right || !mCollideInfo.bottomRight) {
		moveState = MoveState::LEFT;
	}

	mAnimator.Update();
	handleLinearMovement();
	updatePhysics(level);
}

Bat::Bat()
	: Enemy(9, { 12, 12 }, 2, 1.6f, 0)
{
	moveState = MoveState::LEFT;
}

void Bat::Update(Level& level)
{
	if (mCollideInfo.left) {
		moveState = MoveState::RIGHT;
	}
	else if (mCollideInfo.right) {
		moveState = MoveState::LEFT;
	}

	mAnimator.Update();
	handleLinearMovement();
	updatePhysics(level);
}

Dragon::Dragon()
	: Enemy(5, { 16, 16 }, 3, 0, GRAVITY),
	mFiring(false),
	mFireCounter(0)
{
}

void Dragon::Fire(Level& level)
{
	std::unique_ptr<Fireball> fireball = std::make_unique<Fireball>();
	fireball->position = position;
	fireball->moveState = orientation == Orientation::LEFT ? MoveState::LEFT : MoveState::RIGHT;
	level.AddEntity(std::move(fireball));

	mFireCounter = 0;
	mFiring = false;
}

void Dragon::Update(Level& level)
{
	if (!mFiring) {
		mFireCounter += 1000.0 * FIXED_DT;
		Player* player = level.GetNearestPlayer(position, 80);
		if (player) {
			if (!mFiring) {
				orientation = position.x < player->position.x ? Orientation::RIGHT : Orientation::LEFT;
			}
			if (mFireCounter >= 2000) {
				mFiring = true;
				mAnimator.Play(mSpritesheet.GetAnimation(8), false, false);
			}
		}
	}

	if (!mAnimator.IsPlaying()) {
		Fire(level);
		mAnimator.Play(mSpritesheet.GetAnimation(5), true, false);
	}

	mAnimator.Update();
	handleLinearMovement();
	updatePhysics(level);
}

Projectile::Projectile(int baseSpriteID, const Hitbox& hitbox, int dmg, DamageType damageType, float speed, float gravity, double lifetime, bool isPlayer)
	: Mover(Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_ENEMIES), baseSpriteID, hitbox, 1, speed, gravity, true),
	mDamage(dmg),
	mDamageType(damageType),
	mLifetime(lifetime),
	mIsPlayer(isPlayer)
{
	mCollideInfo.bottom = false;
}

void Projectile::Update(Level& level)
{
	mAnimator.Update();

	if (moveState != MoveState::IDLE) {
		mLifetime -= 1000.0 * FIXED_DT;

		if (mCollideInfo.left || mCollideInfo.right || mCollideInfo.bottom) {
			moveState = MoveState::IDLE;
			mAnimator.Play(mSpritesheet.GetAnimation(baseSpriteID + 3), false, false);
		}
	}
	handleLinearMovement();
	updatePhysics(level, false);

	if (mIsPlayer) {
		Interactable* fixture = level.GetOverlappingFixture(*this);
		if (fixture && fixture->projectileTrigger) {
			if (fixture->OnAction(level)) {
				alive = false;
				std::unique_ptr<FXEffect> effect = std::make_unique<FXEffect>(21);
				effect->position = position;
				level.AddEntity(std::move(effect));
			}
		}
	}
	else {
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (level.isPlayerAlive(i) && Collide(*level.players[i])) {
				level.players[i]->TakeDamage(mDamage, mDamageType);
				alive = false;
			}
		}
	}

	if ((moveState == MoveState::IDLE && !mAnimator.IsPlaying()) || mLifetime <= 0) {
		alive = false;
	}
}

Fireball::Fireball()
	: Projectile(13, { 4, 4 }, 2, DamageType::Burn, 2.0f, 0, 2500)
{
}

PlasmaBullet::PlasmaBullet()
	: Projectile(17, { 4, 4 }, 1, DamageType::Hit, 2.0f, 0, 2500, true)
{
}

Controlable::Controlable(int id, float w, float h, bool active)
	: Entity(id, Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES), { w, h }),
	mIsActive(active)
{
}

void Controlable::Update(Level& level)
{
}

void Controlable::Activate()
{
	mIsActive = true;
}

void Controlable::Deactivate()
{
	mIsActive = false;
}

Door::Door(int id, float w, float h)
	: Controlable(id, w, h),
	mIsOpened(false),
	mCounter(0),
	mOpenStarted(false),
	mOpenProgress(0)
{
}

void Door::Update(Level& level)
{
	if (IsActive()) {
		mOpenStarted = true;
	}

	if (!mOpenStarted || mIsOpened) return;

	Tilemap& tilemap = level.tilemap;
	TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	mCounter += 1000.0 * FIXED_DT;
	if (mCounter >= 200) {
		mCounter = 0;
		int y0 = tilemap.getRowIdx(position.y - mHitbox.h * 0.5f);
		int y1 = tilemap.getRowIdx(position.y + mHitbox.h * 0.5f);
		if (y0 + mOpenProgress > y1) {
			mIsOpened = true;
		}
		else {
			int x0 = tilemap.getColIdx(position.x - mHitbox.w * 0.5f);
			int x1 = tilemap.getColIdx(position.x + mHitbox.w * 0.5f);
			for (int x = x0; x <= x1; ++x) {
				layer.DestroyTile(x, y1 - mOpenProgress);
			}
			++mOpenProgress;
		}
	}
}

Bridge::Bridge(int id, float w, float h, int direction, int tileID)
	: Controlable(id, w, h),
	mIsOpened(false),
	mCounter(0),
	mOpenStarted(false),
	mOpenProgress(0),
	mDirection(direction),
	mTileID(tileID)
{
}

void Bridge::Update(Level& level)
{
	if (IsActive()) {
		mOpenStarted = true;
	}

	if (!mOpenStarted || mIsOpened) return;

	Tilemap& tilemap = level.tilemap;
	TileLayer& layer = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	mCounter += 1000.0 * FIXED_DT;
	if (mCounter >= 200) {
		mCounter = 0;
		int x0 = tilemap.getColIdx(position.x - mHitbox.w * 0.5f);
		int x1 = tilemap.getColIdx(position.x + mHitbox.w * 0.5f);
		if (x0 + mOpenProgress > x1) {
			mIsOpened = true;
		}
		else {
			int y0 = tilemap.getRowIdx(position.y - mHitbox.h * 0.5f);
			int y1 = tilemap.getRowIdx(position.y + mHitbox.h * 0.5f);
			for (int y = y0; y <= y1; ++y) {
				layer.AddTile(mDirection > 0 ? x0 + mOpenProgress : x1 - mOpenProgress, y, mTileID);
			}
			++mOpenProgress;
		}
	}
}

Spawner::Spawner(int id, ProjectileType projectileType, int reloadTime, int direction, bool active)
	: Controlable(id, 0, 0, active),
	mProjectileType(projectileType),
	mReloadTime(reloadTime),
	mDirection(direction),
	mCounter(0)
{
}

void Spawner::Update(Level& level)
{
	if (!IsActive()) return;

	mCounter += 1000.0 / 60;

	if (mCounter >= mReloadTime) {
		mCounter = 0;

		spawn(level);
	}
}

void Spawner::spawn(Level& level)
{
	std::unique_ptr<Projectile> projectile;

	switch (mProjectileType)
	{
	case ProjectileType::Fireball:
		projectile = std::make_unique<Fireball>();
		break;
	case ProjectileType::PlasmaBullet:
		projectile = std::make_unique<PlasmaBullet>();
		break;
	default:
		break;
	}

	if (projectile) {
		projectile->position = position;
		projectile->moveState = mDirection > 0 ? MoveState::RIGHT : MoveState::LEFT;

		level.AddEntity(std::unique_ptr<Projectile>(projectile.release()));
	}
}

Platform::Platform(int id, float speed, bool solid, bool active, const std::vector<glm::vec2>& points)
	: Controlable(id, 32, 16, active),
	speed(speed),
	mIsSolid(solid),
	mPoints(points),
	mPointIndex(0),
	velocity(0)
{
	spriteRegion = mSpritesheet.GetRegion(55, 2, 1);
}

void Platform::Update(Level& level)
{
	if (!IsActive()) {
		velocity = glm::vec2(0);
		return;
	}

	int nextPointIndex = (mPointIndex + 1) % mPoints.size();
	glm::vec2 targetPos = mPoints[nextPointIndex];
	glm::vec2 dir = glm::normalize(targetPos - position);
	float dist = glm::distance(targetPos, position);
	if (dist <= speed) {
		mPointIndex = nextPointIndex;
		velocity = dir * dist;
	}
	else {
		velocity = dir * speed;
	}
	position += velocity;
}

//Obstacle::Obstacle()
//	: Mover()
//{}

Interactable::Interactable(Target target, bool projectileTrigger)
	: Entity(0, Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_FIXTURES), { 8, 8 }),
	mTarget(target),
	projectileTrigger(projectileTrigger)
{
}

bool Interactable::OnAction(Level& level)
{
	return false;
}

bool Interactable::OnItem(Level& level, const Item& item)
{
	return false;
}

void Interactable::OnTarget(Level& level, bool active) const
{
	Controlable* target = nullptr;
	switch (mTarget.type) {
	case EntityType::Platform:
		target = level.GetPlatformByID(mTarget.id);
		break;
	case EntityType::Controlable:
		target = level.GetStructureByID(mTarget.id);
		break;
	default:
		break;
	}
	if (target) {
		if (active) {
			target->Activate();
		}
		else {
			target->Deactivate();
		}
	}
}

void Interactable::Update(Level& level)
{
}

Lever::Lever(Target target, bool isUp)
	: Interactable(target),
	isUp(isUp)
{
	handleSprite();
}

bool Lever::OnAction(Level& level)
{
	isUp = !isUp;
	handleSprite();
	OnTarget(level, isUp);
	return true;
}

void Lever::handleSprite()
{
	if (isUp) {
		spriteRegion = mSpritesheet.GetRegion(3);
	}
	else {
		spriteRegion = mSpritesheet.GetRegion(4);
	}
}

Button::Button(Target target)
	: Interactable(target),
	mPressedDownCounter(0)
{
	spriteRegion = mSpritesheet.GetRegion(1);
}

void Button::Update(Level& level)
{
	if (mPressedDownCounter > 0) {
		mPressedDownCounter -= 1000.0 * FIXED_DT;
		if (mPressedDownCounter <= 0) {
			spriteRegion = mSpritesheet.GetRegion(1);
		}
	}
}

bool Button::OnAction(Level& level)
{
	if (mPressedDownCounter > 0) return false;
	mPressedDownCounter = 300;
	spriteRegion = mSpritesheet.GetRegion(2);
	OnTarget(level, true);
	return true;
}

DoorLock::DoorLock(Target target, const std::string& color)
	: Interactable(target),
	color(color)
{
	if (color == "red") {
		spriteRegion = mSpritesheet.GetRegion(5);
	}
	else if (color == "blue") {
		spriteRegion = mSpritesheet.GetRegion(6);
	}
	else if (color == "yellow")
	{
		spriteRegion = mSpritesheet.GetRegion(7);
	}
}

bool DoorLock::OnItem(Level& level, const Item& item)
{
	if (item.name != "key_" + color) return false;
	alive = false;
	OnTarget(level, true);
	return true;
}

HelpBlock::HelpBlock(const std::string& text)
	: Interactable(Target(), false),
	text(text)
{
	spriteRegion = mSpritesheet.GetRegion(8);
}

bool HelpBlock::OnAction(Level& level)
{
	Dialog& dialog = level.dialogs[LEVEL_DIALOG_HELP];
	dialog.text = text;
	dialog.position = position + glm::vec2(0, 24);
	dialog.SetOpened(true);
	return true;
}

FXEffect::FXEffect(int animationID)
	: Entity(0, Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_ENEMIES), { 0, 0 })
{
	mAnimator.Play(mSpritesheet.GetAnimation(animationID), false, true);
}

void FXEffect::Update(Level& level)
{
	mAnimator.Update();

	if (!mAnimator.IsPlaying()) {
		alive = false;
	}
}
