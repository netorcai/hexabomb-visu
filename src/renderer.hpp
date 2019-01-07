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
        const std::vector<Bomb> & bombs,
        const std::map<int, int> & score,
        const std::map<int, int> & cellCount,
        const std::vector<netorcai::PlayerInfo> & playersInfo);

    void onTurn(
        const std::unordered_map<Coordinates, Cell> & cells,
        const std::vector<Character> & characters,
        const std::vector<Bomb> & bombs,
        const std::map<int, int> & score,
        const std::map<int, int> & cellCount,
        const std::vector<netorcai::PlayerInfo> & playersInfo = {});

    void render(sf::RenderWindow & window);
    void updateView(int newWidth, int newHeight);

private:
    void generatePlayerColors(int nbColors);
    void updatePlayerInfo(const std::vector<netorcai::PlayerInfo> & playersInfo);

private:
    sf::Texture _bombTexture;
    sf::Texture _characterTexture;
    sf::Texture _emptyTexture;

    sf::Font _monospaceFont;

    std::unordered_map<Coordinates, sf::Sprite*> _cellSprites;
    std::unordered_map<int, sf::Sprite*> _characterSprites;
    std::vector<sf::Sprite*> _aliveCharacters;
    std::vector<sf::Sprite*> _bombSprites;
    std::vector<sf::Text> _pInfoTexts;
    std::vector<sf::RectangleShape> _pInfoRectShapes;

    std::vector<netorcai::PlayerInfo> _playersInfo;
    std::map<int, int> _score;
    std::map<int, int> _cellCount;

    std::vector<sf::Color> _colors;
    sf::FloatRect _boardBoundingBox;
    sf::View _boardView;
    sf::View _playersInfoView;

    const float _textureSize = 256.0f;
    const sf::Color _backgroundColor = sf::Color(0xa0a0a0ff);
    const sf::Vector2f _characterScale = sf::Vector2f(0.7f, 0.7f);
    const sf::Vector2f _bombScale = sf::Vector2f(0.5f, 0.5f);
};
