#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "constants.h"
#include <memory>

struct SpriteRegion {
	float x = 0;
	float y = 0;
	float width = 0;
	float height = 0;
};

struct Animation {
	int frameDuration;
	std::vector<int> frames;

	Animation(const std::vector<int>& frames, int frameDuration = 100);
};

class Animator {
public:
	Animator();
	void Play(const Animation* animation, bool repeat, bool restart);
	void Pause();
	void Resume();
	void Update();
	inline int GetCurrentFrameID() const { return mAnimation ? mAnimation->frames[mFrameIndex] : 0; }
	inline const Animation* GetAnimation() const { return mAnimation; }
	inline bool IsPlaying() const { return mPlaying; }

private:
	const Animation* mAnimation;
	bool mPlaying;
	int mFrameIndex;
	double mCounter;
	bool mRepeat;
};

struct TileData {
	bool collide = false;
	enum class Type {
		Default,
		Lava,
		Water,
		Spikes,
		Slope,
		Platform,
		Ledder
	};
	Type type = Type::Default;
	struct {
		float y0 = 0;
		float y1 = 0;
		bool reversed = false;
	} slope;
	std::unique_ptr<Animation> animation = nullptr;
};

class Spritesheet {
public:
	int id;
	int spriteWidth;
	int spriteHeight;
	int columns;
	int count;
	int spacing;
	int margin;

	Spritesheet();
	Spritesheet(int id, int spriteWidth, int spriteHeight, int columns, int count, int spacing = 0, int margin = 0);
	void Update();
	SpriteRegion GetRegion(int tileID, float w = 1, float h = 1) const;
	static Spritesheet CreateFromJson(int id, const std::string& path, bool includeAnimators = false);
	const TileData& GetTileData(int tileID) const;
	const Animation* GetAnimation(int tileID) const;

private:
	std::vector<TileData> mTileData;
	std::unordered_map<int, Animator> mAnimators;
};

class SpriteCache {
public:
	SpriteCache();
	void AddSpritesheet(Spritesheet&& spritesheet);
	inline Spritesheet& GetSpritesheet(int id) { return mSpritesheets[id - 1]; }

private:
	std::vector<Spritesheet> mSpritesheets;
};