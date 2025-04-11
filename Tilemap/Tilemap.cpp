#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include "stb_image.h"
#include "stb_image_write.h"
#include <fstream>

using namespace std;

const int TILE_SIZE = 16;


struct Tile {
    vector<unsigned char> pixels;

    bool operator==(const Tile& other) const {
        return pixels == other.pixels;
    }
};

struct TileHash {
    size_t operator()(const Tile& tile) const {
        return hash<string>()(string(tile.pixels.begin(), tile.pixels.end()));
    }
};


// Đọc ảnh và cắt thành danh sách tile
vector<Tile> extractTiles(const string& imagePath, int& outWidth, int& outHeight) {
    int width, height, channels;
    unsigned char* image = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
    if (!image) {
        cerr << "Error loading image: " << imagePath << endl;
        exit(1);
    }

    int rows = height / TILE_SIZE;
    int cols = width / TILE_SIZE;
    outWidth = cols;
    outHeight = rows;

    vector<Tile> tiles;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            vector<unsigned char> tileData(TILE_SIZE * TILE_SIZE * 4);
            for (int y = 0; y < TILE_SIZE; y++) {
                for (int x = 0; x < TILE_SIZE; x++) {
                    int imgIdx = ((row * TILE_SIZE + y) * width + (col * TILE_SIZE + x)) * 4;
                    int tileIdx = (y * TILE_SIZE + x) * 4;
                    for (int c = 0; c < 4; c++) {
                        tileData[tileIdx + c] = image[imgIdx + c];
                    }
                }
            }
            tiles.push_back({ tileData });
        }
    }

    stbi_image_free(image);
    return tiles;
}

// Tìm chỉ số của tile trong tileset
int findTileIndex(const Tile& tile, const vector<Tile>& tileset) {
    for (size_t i = 0; i < tileset.size(); i++) {
        if (tileset[i] == tile) {
            return (int)i;
        }
    }
    return -1;
}

// Lưu tilemap dưới dạng JSON
void saveTileMapTxt(const vector<vector<int>>& tilemap, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error saving tilemap!" << endl;
        return;
    }

    for (size_t i = 0; i < tilemap.size(); ++i) {
        for (size_t j = 0; j < tilemap[i].size(); ++j) {
            file << tilemap[i][j];
            if (j < tilemap[i].size() - 1) file << " ";
        }
        file  << (i < tilemap.size() - 1 ? ",\n" : "\n");
    }

    file.close();
}


// Cắt ảnh thành tiles và lọc các tile trùng lặp
vector<Tile> extractUniqueTiles(const string& imagePath, int tileSize) {
    int width, height, channels;
    unsigned char* image = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
    if (!image) {
        cerr << "Error loading image: " << imagePath << endl;
        exit(1);
    }

    unordered_map<Tile, int, TileHash> uniqueTiles;
    vector<Tile> tiles;

    int rows = height / tileSize;
    int cols = width / tileSize;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            vector<unsigned char> tileData(tileSize * tileSize * 4); // RGBA
            for (int y = 0; y < tileSize; y++) {
                for (int x = 0; x < tileSize; x++) {
                    int imgIdx = ((row * tileSize + y) * width + (col * tileSize + x)) * 4;
                    int tileIdx = (y * tileSize + x) * 4;
                    for (int c = 0; c < 4; c++) {
                        tileData[tileIdx + c] = image[imgIdx + c];
                    }
                }
            }

            Tile tile = { tileData };
            if (uniqueTiles.count(tile) == 0) {
                uniqueTiles[tile] = tiles.size();
                tiles.push_back(tile);
            }
        }
    }

    stbi_image_free(image);
    return tiles;
}

// Lưu tileset từ các tile độc nhất
void saveTileSet(const string& filename, const vector<Tile>& tiles, int tileSize) {
    int totalTiles = tiles.size();
    int rows = 1;
    int sheetWidth = totalTiles * tileSize;
    int sheetHeight = rows * tileSize;

    vector<unsigned char> spriteSheet(sheetWidth * sheetHeight * 4, 255); // Mặc định trắng (RGBA)

    for (size_t i = 0; i < tiles.size(); i++) {
        int row = i / totalTiles;
        int col = i % totalTiles;

        for (int y = 0; y < tileSize; y++) {
            for (int x = 0; x < tileSize; x++) {
                int srcIdx = (y * tileSize + x) * 4;
                int destIdx = ((row * tileSize + y) * sheetWidth + (col * tileSize + x)) * 4;

                for (int c = 0; c < 4; c++) {
                    spriteSheet[destIdx + c] = tiles[i].pixels[srcIdx + c];
                }
            }
        }
    }

    stbi_write_png(filename.c_str(), sheetWidth, sheetHeight, 4, spriteSheet.data(), sheetWidth * 4);
}

int main() {

    string filename = "frame0";
    //Cắt ảnh thành tileset
    string imagePath = filename+".png"; // Ảnh đầu vào
    string tilesetName = filename + "tileset.png";
    vector<Tile> uniqueTiles = extractUniqueTiles(imagePath, TILE_SIZE);
    saveTileSet(tilesetName, uniqueTiles, TILE_SIZE);
    cout << "Tileset co "<<uniqueTiles.size()<<endl;
    cout << "Tileset saved as tileset.png" << endl;





    string tilesetPath = tilesetName;
   

    int tilesetWidth, tilesetHeight, mapWidth, mapHeight;

    // Đọc tileset và bản đồ
    vector<Tile> tileset = extractTiles(tilesetPath, tilesetWidth, tilesetHeight);
    vector<Tile> mapTiles = extractTiles(imagePath, mapWidth, mapHeight);

    vector<vector<int>> tilemap(mapHeight, vector<int>(mapWidth, -1));

    // So sánh và gán index từ tileset vào tilemap
    for (int row = 0; row < mapHeight; row++) {
        for (int col = 0; col < mapWidth; col++) {
            int index = findTileIndex(mapTiles[row * mapWidth + col], tileset);
            tilemap[row][col] = index;
        }
    }

    saveTileMapTxt(tilemap, filename+".txt");
    cout << "Tilemap saved " << endl;


    return 0;
}
