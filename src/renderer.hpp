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
    sf::Texture _bombTexture;
    sf::Texture _characterTexture;

    std::unordered_map<Coordinates, sf::Sprite*> _cellSprites;
    std::unordered_map<int, sf::Sprite*> _characterSprites;
    std::vector<sf::Sprite> _bombSprites;

    std::vector<sf::Color> _colors;
};
