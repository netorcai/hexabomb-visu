#pragma once

#include <map>
#include <vector>
#include <unordered_map>

#include <SFML/Graphics.hpp>

#include "hexabomb-parse.hpp"

class HexabombRenderer
{
public:
    HexabombRenderer();
    ~HexabombRenderer();

    void onGameInit(
        const std::unordered_map<Coordinates, Cell> & cells,
        const std::vector<Character> & characters,
        const std::vector<Bomb> & bombs);

    void onTurn(
        const std::unordered_map<Coordinates, Cell> & cells,
        const std::vector<Character> & characters,
        const std::vector<Bomb> & bombs,
        const std::map<int, int> & score,
        const std::map<int, int> & cellCount);

    void render(sf::RenderWindow & window);

private:
    void generatePlayerColors(int nbColors);

private:
    sf::Texture _bombTexture;
    sf::Texture _characterTexture;
    sf::Texture _emptyTexture;

    std::unordered_map<Coordinates, sf::Sprite*> _cellSprites;
    std::unordered_map<int, sf::Sprite*> _characterSprites;
    std::vector<sf::Sprite*> _bombSprites;

    std::vector<sf::Color> _colors;
    const float _textureSize = 256.0f;
    const sf::Color _backgroundColor = sf::Color(64, 64, 64);
    sf::View _boardView;
};
