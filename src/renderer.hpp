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
        int lastTurnNumber,
        const std::vector<netorcai::PlayerInfo> & playersInfo);

    void onTurn(
        const std::unordered_map<Coordinates, Cell> & cells,
        const std::vector<Character> & characters,
        const std::vector<Bomb> & bombs,
        const std::unordered_map<int, std::vector<Coordinates> > & explosions,
        const std::map<int, int> & score,
        const std::map<int, int> & cellCount,
        int currentTurnNumber,
        int lastTurnNumber,
        const std::vector<netorcai::PlayerInfo> & playersInfo = {});

    void onStatusChange(const std::string & status);

    void render(sf::RenderWindow & window);
    void updateView(int newWidth, int newHeight);
    void toggleShowCoordinates();
    void setSuddenDeath(bool isSuddenDeath);

private:
    void generatePlayerColors(int nbColors);
    void updatePlayerInfo(
        int currentTurnNumber,
        int lastTurnNumber,
        const std::vector<netorcai::PlayerInfo> & playersInfo);
    void updateCellCount(const std::map<int, int> & cellCount);
    sf::Vector2f axialToCartesian(Coordinates axial) const;

private:
    bool _showCoordinates = false;
    bool _isSuddenDeath = false;

    sf::Texture _bombTexture;
    sf::Texture _characterTexture;
    sf::Texture _deadCharacterTexture;
    sf::Texture _specialCharacterTexture;
    sf::Texture _explosionTexture;

    sf::Font _monospaceFont;

    std::unordered_map<Coordinates, sf::CircleShape*> _cellShapes;
    std::unordered_map<int, sf::Sprite*> _characterSprites;
    std::vector<sf::Sprite*> _charactersToDraw;
    std::vector<sf::Sprite*> _bombSprites;
    std::vector<sf::Sprite*> _explosionSprites;
    std::vector<sf::Text> _pInfoTexts;
    std::vector<sf::RectangleShape> _pInfoRectShapes;
    std::vector<sf::RectangleShape> _ccdRectShapes;
    sf::Text _statusText;

    std::vector<netorcai::PlayerInfo> _playersInfo;
    std::map<int, int> _score;
    std::map<int, int> _cellCount;
    int _nbNeutralCells = 0;
    std::string _status;

    std::vector<sf::Color> _colors;
    sf::FloatRect _boardBoundingBox;
    sf::View _boardView;
    sf::View _playersInfoView;
    sf::View _cellCountDistributionView;

    const float _textureSize = 256.0f;
    const float _hexBaseLength = 128.0f;
    const float _hexOutlineThickness = 8.0f;
    const sf::Color _backgroundColor = sf::Color(0xa0a0a0ff);
    const sf::Vector2f _characterScale = sf::Vector2f(0.7f, 0.7f);
    const sf::Vector2f _bombScale = sf::Vector2f(0.5f, 0.5f);
    const sf::Vector2f _explosionScale = sf::Vector2f(0.7f, 0.7f);
    const float _piRectWidth = 280.f;
    const float _ccdWidth = 100.f;
    const float _ccdHeight = 10.f;
    const float _ccdHeightRatioInScreen = 0.02f;
};
