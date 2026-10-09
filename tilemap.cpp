#include "tilemap.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include "engine.h"

const int TILE_BREAK_DURATION = 300;

TileLayer::TileLayer(int width, int height) :
	mWidth(width),
	mHeight(height),
	mData(width * height)
{
}

void TileLayer::Update(Level& level)
{
	for (auto& progress : mBreakingProgress) {
		progress.counter += 1000.0 * FIXED_DT;
		if (progress.counter >= TILE_BREAK_DURATION) {
			breakNeighborTiles(progress.x, progress.y);
		}
	}

	std::erase_if(mBreakingProgress,
		[](const auto& progress) {
			return progress.counter >= TILE_BREAK_DURATION;
		}
	);

	std::erase_if(mDynamicTiles, 
		[this](int idx) { 
			return mData[idx].id == 0;
		}
	);
}

void TileLayer::GetTileData(std::vector<Tile>& out, int x0, int y0, int x1, int y1) const
{
	for (int y = std::max(y0, 0); y <= std::min(y1, mHeight - 1); ++y) {
		for (int x = std::max(x0, 0); x <= std::min(x1, mWidth - 1); ++x) {
			const Tile& tile = mData[y * mWidth + x];
			if (tile.id && tile.data) {
				out.push_back(tile);
			}
		}
	}
}

void TileLayer::DestroyTile(int x, int y)
{
	if (x < 0 || x > mWidth - 1 || y < 0 || y > mHeight - 1) return;
	Tile& tile = mData[y * mWidth + x];
	tile.id = 0;
	tile.breakable = false;
	tile.data = nullptr;
}

void TileLayer::BreakTile(int x, int y)
{
	if (x < 0 || x > mWidth - 1 || y < 0 || y > mHeight - 1) return;
	Tile& tile = mData[y * mWidth + x];
	if (tile.breakable) {
		DestroyTile(x, y);
		mBreakingProgress.push_back({ tile.x, tile.y });
	}
}

void TileLayer::AddTile(int x, int y, int id)
{
	if (x < 0 || x > mWidth - 1 || y < 0 || y > mHeight - 1 || mData[y * mWidth + x].id != 0) return;
	const Spritesheet& tileset = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES);
	const TileData& data = tileset.GetTileData(id);
	Tile tile;
	tile.x = x;
	tile.y = y;
	tile.id = id;
	tile.data = &data;
	tile.isDynamic = true;
	mData[y * mWidth + x] = tile;
	addDynamicTile(x, y);
}

void TileLayer::addDynamicTile(int x, int y)
{
	mDynamicTiles.insert(y * mWidth + x);
}

void TileLayer::forEach(int x0, int y0, int x1, int y1, std::function<void(Tile& tile, TileLayer& layer)> callback)
{
	for (int y = std::max(y0, 0); y <= std::min(y1, mHeight - 1); ++y) {
		for (int x = std::max(x0, 0); x <= std::min(x1, mWidth - 1); ++x) {
			Tile& tile = mData[y * mWidth + x];
			callback(tile, *this);
		}
	}
}

void TileLayer::forEach(int x0, int y0, int x1, int y1, std::function<void(const Tile& tile, const TileLayer& layer)> callback) const
{
	for (int y = std::max(y0, 0); y <= std::min(y1, mHeight - 1); ++y) {
		for (int x = std::max(x0, 0); x <= std::min(x1, mWidth - 1); ++x) {
			const Tile& tile = mData[y * mWidth + x];
			callback(tile, *this);
		}
	}
}

void TileLayer::breakNeighborTiles(int x, int y)
{
	bool done = false;

	if (y < mHeight - 1) {
		const Tile& above = mData[(y + 1) * mWidth + x];
		if (above.breakable) {
			BreakTile(x, y + 1);
			done = true;
		}
	}

	if (!done && y > 0) {
		const Tile& below = mData[(y - 1) * mWidth + x];
		if (below.breakable) {
			BreakTile(x, y - 1);
			done = true;
		}
	}

	if (!done && x < mWidth - 1) {
		const Tile& right = mData[y * mWidth + (x + 1)];
		if (right.breakable) {
			BreakTile(x + 1, y);
			done = true;
		}
	}

	if (!done && x > 0) {
		const Tile& left = mData[y * mWidth + (x - 1)];
		if (left.breakable) {
			BreakTile(x - 1, y);
			done = true;
		}
	}
}

Tilemap::Tilemap(int width, int height, int tileWidth, int tileHeight)
	: mWidth(width),
	mHeight(height),
	mTileWidth(tileWidth),
	mTileHeight(tileHeight),
	mTileLayers(TILE_LAYER_COUNT)
{
}

Tilemap Tilemap::CreateFromJson(const std::string& path)
{
	const Spritesheet& tileset = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES);
	std::vector<char> buffer = Engine::GetAssetManager().Load(path);
	nlohmann::json json = nlohmann::json::parse(buffer.begin(), buffer.end());

	int width = json["width"].get<int>();
	int height = json["height"].get<int>();
	int tileWidth = json["tilewidth"].get<int>();
	int tileHeight = json["tileheight"].get<int>();
	Tilemap tilemap(width, height, tileWidth, tileHeight);

	for (const auto& layerJson : json["layers"]) {
		std::string layerName = layerJson["name"];
		std::string layerType = layerJson["type"];

		if (layerType == "tilelayer") {
			int layerIdx;
			if (layerName == "background") {
				layerIdx = TILE_LAYER_BACKGROUND;
			}
			else if (layerName == "midground") {
				layerIdx = TILE_LAYER_MIDGROUND;
			} 
			else if (layerName == "foreground") {
				layerIdx = TILE_LAYER_FOREGROUND;
			}
			else {
				continue;
			}

			int layerWidth = layerJson["width"].get<int>();
			int layerHeight = layerJson["height"].get<int>();

			std::unique_ptr<TileLayer> layer = std::make_unique<TileLayer>(layerWidth, layerHeight);

			const auto& tileDataJson = layerJson["data"];

			for (int i = 0; i < layer->getHeight(); ++i) {
				for (int j = 0; j < layer->getWidth(); ++j) {
					int tileID = tileDataJson[i * layer->getWidth() + j].get<int>();
					int x = j;
					int y = layer->getHeight() - 1 - i;
					Tile tile;
					tile.x = j;
					tile.y = y;
					if (tileID != 0) {
						tile.id = tileID;
						tile.data = &tileset.GetTileData(tileID);
						if (tile.data->animation != nullptr) {
							tile.isDynamic = true;
							layer->addDynamicTile(x, y);
						}
					}
					layer->SetTile(y * layer->getWidth() + x, tile);
				}
			}
			tilemap.mTileLayers[layerIdx] = std::unique_ptr<TileLayer>(layer.release());
		}
		else if (layerType == "objectgroup") {
			for (const auto& objectJson : layerJson["objects"]) {
				ObjectData data;
				data.name = objectJson["name"];
				data.width = objectJson["width"].get<float>();
				data.height = objectJson["height"].get<float>();
				data.x = objectJson["x"].get<float>() - 0.5f * tilemap.getTileWidth() + data.width * 0.5f;
				data.y = (tilemap.getHeight() - 0.5f) * tilemap.getTileHeight() - objectJson["y"].get<float>() - data.height * 0.5f;
				if (objectJson.contains("properties")) {
					for (const auto& propJson : objectJson["properties"]) {
						std::string propName = propJson["name"];
						if (propName == "id") {
							data.id = propJson["value"].get<int>();
						}
						else if (propName == "active") {
							data.active = propJson["value"].get<bool>();
						}
						else if (propName == "direction") {
							data.direction = propJson["value"].get<int>();
						}
						else if (propName == "platform.index") {
							data.platform.index = propJson["value"].get<int>();
						}
						else if (propName == "platform.speed") {
							data.platform.speed = propJson["value"].get<float>();
						}
						else if (propName == "platform.solid") {
							data.platform.solid = propJson["value"].get<bool>();
						}
						else if (propName == "lock.color") {
							data.lock.color = propJson["value"];
						}
						else if (propName == "lever.up") {
							data.lever.up = propJson["value"].get<bool>();
						}
						else if (propName == "help.text") {
							data.help.text = propJson["value"];
						}
						else if (propName == "bridge.tileid") {
							data.bridge.tileid = propJson["value"].get<int>() + 1;
						}
						else if (propName == "target.type") {
							data.target.type = propJson["value"];
						}
						else if (propName == "target.id") {
							data.target.id = propJson["value"].get<int>();
						}
						else if (propName == "spawner.reload") {
							data.spawner.reload = propJson["value"].get<int>();
						}
						else if (propName == "spawner.projectile") {
							data.spawner.projectile = propJson["value"];
						}
					}
				}
				tilemap.mObjects.push_back(std::move(data));
			}
		}
	}

	TileLayer& midground = tilemap.getTileLayer(TILE_LAYER_MIDGROUND);
	for (const auto& data : tilemap.getObjects()) {
		if (data.name == "breakable" || data.name == "door") {
			int x0 = tilemap.getColIdx(data.x - data.width * 0.5f);
			int x1 = tilemap.getColIdx(data.x + data.width * 0.5f);
			int y0 = tilemap.getRowIdx(data.y - data.height * 0.5f);
			int y1 = tilemap.getRowIdx(data.y + data.height * 0.5f);

			midground.forEach(x0, y0, x1, y1, [data](Tile& tile, TileLayer& layer) {
				tile.breakable = data.name == "breakable";
				tile.isDynamic = true;
				layer.addDynamicTile(tile.x, tile.y);
				});
		}
	}

	return tilemap;
}

void Tilemap::Update(Level& level)
{
	for (int i = 0; i < TILE_LAYER_COUNT; ++i) {
		mTileLayers[i]->Update(level);
	}
}

bool Tilemap::isTopLedderTile(const Tile& tile) const
{
	const TileLayer& background = getTileLayer(TILE_LAYER_BACKGROUND);
	if (tile.y < background.getHeight() - 1) {
		const Tile& topTile = background.GetTile((tile.y + 1) * background.getWidth() + tile.x);
		return !topTile.data || topTile.data->type != TileData::Type::Ledder;
	}
	return true;
}

bool Tilemap::isGroundLedderTile(const Tile& tile) const
{
	const TileLayer& midground = getTileLayer(TILE_LAYER_MIDGROUND);
	if (tile.y > 0) {
		const Tile& bottomTile = midground.GetTile((tile.y - 1) * midground.getWidth() + tile.x);
		return bottomTile.data && bottomTile.data->collide;
	}
	return false;
}

bool Tilemap::isGroundTile(const Tile& tile, bool includeTopLedder) const
{
	return tile.data->collide || (tile.data->type == TileData::Type::Ledder && isTopLedderTile(tile) && includeTopLedder);
}
