#pragma once

#include <vector>
#include <array>
#include "sprite.h"
#include "constants.h"
#include <unordered_set>
#include <functional>

class Level;

struct Tile {
	int id = 0;
	int x = 0;
	int y = 0;
	bool isDynamic = false;
	bool breakable = false;
	const TileData* data = nullptr;
};

struct BreakingTileProgress {
	int x = 0;
	int y = 0;
	double counter = 0;
};

class TileLayer {
public:
	TileLayer(int width, int height);
	void Update(Level& level);
	void GetTileData(std::vector<Tile>& out, int x0, int y0, int x1, int y1) const;
	void DestroyTile(int x, int y);
	void BreakTile(int x, int y);
	void AddTile(int x, int y, int id);
	void addDynamicTile(int x, int y);
	void forEach(int x0, int y0, int x1, int y1, std::function<void(Tile& tile, TileLayer& layer)> callback);
	void forEach(int x0, int y0, int x1, int y1, std::function<void(const Tile& tile, const TileLayer& layer)> callback) const;
	inline const std::unordered_set<int>& getDynamicTiles() const { return mDynamicTiles; }
	inline Tile GetTile(int idx) const { return mData[idx]; }
	inline Tile& GetTile(int idx) { return mData[idx]; }
	inline void SetTile(int idx, const Tile& tile)  { mData[idx] = tile; }
	inline int getWidth() const { return mWidth; }
	inline int getHeight() const { return mHeight; }
	inline void setWidth(int width) { mWidth = width; }
	inline void setHeight(int height) { mHeight = height; }

private:
	int mWidth;
	int mHeight;
	std::vector<Tile> mData;
	std::unordered_set<int> mDynamicTiles;
	std::vector<BreakingTileProgress> mBreakingProgress;

	void breakNeighborTiles(int x, int y);
};

struct ObjectData {
	std::string name;
	float x = 0;
	float y = 0;
	float width = 0;
	float height = 0;
	int id = 0;
	int direction = 1;
	bool active = false;
	struct {
		int id = 0;
		std::string type;
	} target;
	struct {
		int index = 0;
		float speed = 0;
		bool solid = false;
	} platform;
	struct {
		std::string color;
	} lock;
	struct {
		bool up = false;
	} lever;
	struct {
		std::string text;
	} help;
	struct {
		int tileid = 0;
	} bridge;
	struct {
		std::string projectile;
		int reload = 1000;
	} spawner;
};

class Tilemap {
public:
	Tilemap(int width, int height, int tileWidth, int tileHeight);
	static Tilemap CreateFromJson(const std::string& path);
	void Update(Level& level);
	bool isTopLedderTile(const Tile& tile) const;
	bool isGroundLedderTile(const Tile& tile) const;
	bool isGroundTile(const Tile& tile, bool includeTopLedder) const;
	inline TileLayer& getTileLayer(TileLayerIndex idx) { return *mTileLayers[idx].get(); }
	inline const TileLayer& getTileLayer(TileLayerIndex idx) const { return *mTileLayers[idx].get(); }
	inline const std::vector<ObjectData>& getObjects() const { return mObjects; }
	inline int getWidth() const { return mWidth; }
	inline int getHeight() const { return mHeight; }
	inline int getTileWidth() const { return mTileWidth; }
	inline int getTileHeight() const { return mTileHeight; }
	inline int getColIdx(float x) const { return floorf(x / mTileWidth + 0.5f); }
	inline int getRowIdx(float y) const { return floorf(y / mTileHeight + 0.5f); }

private:
	int mWidth;
	int mHeight;
	int mTileWidth;
	int mTileHeight;
	std::vector<std::unique_ptr<TileLayer>> mTileLayers;
	std::vector<ObjectData> mObjects;
};