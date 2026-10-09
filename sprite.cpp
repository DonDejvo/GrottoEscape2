#include "sprite.h"

#include <assert.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include "engine.h"

Animation::Animation(const std::vector<int>& frames, int frameDuration)
	: frameDuration(frameDuration),
	frames(frames)
{
}

Spritesheet::Spritesheet()
	: id(0),
	spriteWidth(0),
	spriteHeight(0),
	columns(0),
	count(0),
	spacing(0),
	margin(0)
{
}

Spritesheet::Spritesheet(int id, int spriteWidth, int spriteHeight, int columns, int count, int spacing, int margin)
	: id(id),
	spriteWidth(spriteWidth),
	spriteHeight(spriteHeight),
	columns(columns),
	count(count),
	spacing(spacing),
	margin(margin),
	mTileData(count)
{
}

void Spritesheet::Update()
{
	for (auto& animator : mAnimators) {
		animator.second.Update();
	}
}

SpriteRegion Spritesheet::GetRegion(int tileID, float w, float h) const
{
	int idx = tileID - 1;
	auto it = mAnimators.find(tileID);
	if (it != mAnimators.end()) {
		const Animator& animator = it->second;
		idx = animator.GetCurrentFrameID() - 1;
	}
	int x = idx % columns * (spriteWidth + spacing) + margin;
	int y = idx / columns * (spriteHeight + spacing) + margin;
	return SpriteRegion{
		static_cast<float>(x),
		static_cast<float>(y),
		(spriteWidth + spacing) * w - spacing,
		(spriteHeight + spacing) * h - spacing
	};
}

Spritesheet Spritesheet::CreateFromJson(int id, const std::string& path, bool includeAnimators)
{
	std::vector<char> buffer = Engine::GetAssetManager().Load(path);
	nlohmann::json tilesetJson = nlohmann::json::parse(buffer.begin(), buffer.end());

	int spriteWidth = tilesetJson["tilewidth"].get<int>();
	int spriteHeight = tilesetJson["tileheight"].get<int>();
	int columns = tilesetJson["columns"].get<int>();
	int count = tilesetJson["tilecount"].get<int>();
	int spacing = tilesetJson["spacing"].get<int>();
	int margin = tilesetJson["margin"].get<int>();

	Spritesheet spritesheet(id, spriteWidth, spriteHeight, columns, count, spacing, margin);

	for (const auto& tileDataJson : tilesetJson["tiles"]) {
		int tileID = tileDataJson["id"].get<int>() + 1;
		TileData data;
		if (tileDataJson.contains("properties")) {
			for (const auto& propJson : tileDataJson["properties"]) {
				std::string propName = propJson["name"];
				if (propName == "collide") {
					data.collide = propJson["value"].get<bool>();
				}
				else if (propName == "type") {
					std::string type = propJson["value"];
					if (type == "lava") {
						data.type = TileData::Type::Lava;
					}
					else if (type == "slope") {
						data.type = TileData::Type::Slope;
					}
					else if (type == "water") {
						data.type = TileData::Type::Water;
					}
					else if (type == "spikes") {
						data.type = TileData::Type::Spikes;
					}
					else if (type == "patform") {
						data.type = TileData::Type::Platform;
					}
					else if (type == "ledder") {
						data.type = TileData::Type::Ledder;
					}
				}
				else if (propName == "slope.y0") {
					data.slope.y0 = propJson["value"].get<float>();
				}
				else if (propName == "slope.y1") {
					data.slope.y1 = propJson["value"].get<float>();
				}
				else if (propName == "slope.reversed") {
					data.slope.reversed = propJson["value"].get<bool>();
				}
			}
		}
		if (tileDataJson.contains("animation")) {
			int frameDuration = 0;
			std::vector<int> frames;
			bool firstFrame = true;
			for (const auto& frameJson : tileDataJson["animation"]) {
				if (firstFrame) {
					firstFrame = false;
					frameDuration = frameJson["duration"].get<int>();
				}
				int frameTileID = frameJson["tileid"].get<int>() + 1;
				frames.push_back(frameTileID);
			}
			data.animation = std::make_unique<Animation>(frames, frameDuration);

			if (includeAnimators) {
				Animator animator;
				animator.Play(data.animation.get(), true, false);
				spritesheet.mAnimators[tileID] = std::move(animator);
			}
		}
		spritesheet.mTileData[tileID - 1] = std::move(data);
	}

	return spritesheet;
}

const TileData& Spritesheet::GetTileData(int tileID) const
{
	return mTileData[tileID - 1];
}

const Animation* Spritesheet::GetAnimation(int tileID) const
{
	const Animation* anim = mTileData[tileID - 1].animation.get();
	assert(anim != nullptr);
	return anim;
}

Animator::Animator()
	: mAnimation(nullptr),
	mPlaying(false),
	mFrameIndex(0),
	mCounter(0),
	mRepeat(false)
{
}

void Animator::Play(const Animation* animation, bool repeat, bool restart)
{
	if (animation != mAnimation || restart) {
		mCounter = 0;
		mFrameIndex = 0;
	}
	mPlaying = true;
	mAnimation = animation;
	mRepeat = repeat;
}

void Animator::Pause()
{
	mPlaying = false;
}

void Animator::Resume()
{
	mPlaying = mAnimation != nullptr;
}

void Animator::Update()
{
	if (!mPlaying) return;

	mCounter += 1000.0 / 60;
	if (mCounter >= mAnimation->frameDuration) {
		mCounter = 0;
		++mFrameIndex;
		if (mFrameIndex >= mAnimation->frames.size()) {
			if (mRepeat) {
				mFrameIndex = 0;
			}
			else {
				mFrameIndex = mAnimation->frames.size() - 1;
				mPlaying = false;
			}
		}
	}
}

SpriteCache::SpriteCache()
	: mSpritesheets(SPRITESHEETS_COUNT)
{
}

void SpriteCache::AddSpritesheet(Spritesheet&& spritesheet)
{
	int id = spritesheet.id;
	mSpritesheets[id - 1] = std::move(spritesheet);
}
