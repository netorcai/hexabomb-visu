#include "renderer.hpp"

#include <algorithm>
#include <random>

#include "util.hpp"

static sf::Vector2f axialToCartesian(Coordinates axial)
{
    double base_length = 128.0;

    sf::Vector2f res;
    res.x = base_length * (sqrt(3.0) * axial.q + sqrt(3.0) / 2.0 * axial.r);
    res.y = base_length * (                           3.0  / 2.0 * axial.r);

    return res;
}

HexabombRenderer::HexabombRenderer()
{
    _bombTexture.loadFromFile(searchImageAbsoluteFilename("bomb.png"));
    _characterTexture.loadFromFile(searchImageAbsoluteFilename("char.png"));
    _emptyTexture.loadFromFile(searchImageAbsoluteFilename("empty.png"));

    _bombTexture.setSmooth(true);
    _characterTexture.setSmooth(true);
    _emptyTexture.setSmooth(true);

    _monospaceFont.loadFromFile(searchFontAbsoluteFilename("DejaVuSansMono.ttf"));
}

HexabombRenderer::~HexabombRenderer()
{
    for (const auto & [coord, sprite] : _cellSprites)
        delete sprite;

    // Draw characters
    for (const auto & [coord, sprite] : _characterSprites)
        delete sprite;

    // Draw bombs
    for (const auto & sprite : _bombSprites)
        delete sprite;
}

void HexabombRenderer::onGameInit(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs,
    const std::map<int, int> & score,
    const std::map<int, int> & cellCount,
    int lastTurnNumber,
    const std::vector<netorcai::PlayerInfo> & playersInfo)
{
    float xmin = std::numeric_limits<float>::max();
    float ymin = std::numeric_limits<float>::max();
    float xmax = std::numeric_limits<float>::min();
    float ymax = std::numeric_limits<float>::min();

    generatePlayerColors(2);

    _nbNeutralCells = 0;
    for (const auto & [coord, cell] : cells)
    {
        auto * sprite = new sf::Sprite;
        sprite->setTexture(_emptyTexture);

        sf::Vector2f cartesian = axialToCartesian(coord);
        sprite->setPosition(cartesian);
        sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
        sprite->setColor(_colors[cell.color]);

        _cellSprites[coord] = sprite;

        if (cell.color == 0)
            _nbNeutralCells++;

        // Update bounding box
        if (cartesian.x < xmin) xmin = cartesian.x;
        if (cartesian.y < ymin) ymin = cartesian.y;
        if (cartesian.x > xmax) xmax = cartesian.x;
        if (cartesian.y > ymax) ymax = cartesian.y;
    }

    for (const auto & character : characters)
    {
        auto * sprite = new sf::Sprite;
        sprite->setTexture(_characterTexture);
        sprite->setPosition(axialToCartesian(character.coord));
        sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
        sprite->setScale(_characterScale);
        sprite->setOrigin(sf::Vector2f((2.0/3.0)*_textureSize, _textureSize/2.0));

        _characterSprites[character.id] = sprite;
        _aliveCharacters.push_back(sprite);
    }

    for (const auto & bomb : bombs)
    {
        auto * sprite = new sf::Sprite;
        sprite->setTexture(_bombTexture);
        sprite->setPosition(axialToCartesian(bomb.coord));
        sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
        sprite->setScale(_bombScale);

        _bombSprites.push_back(sprite);
    }

    // Set view
    _boardBoundingBox = sf::FloatRect(
        xmin-_textureSize/2.0, ymin-_textureSize/2.0,
        xmax - xmin + _textureSize,
        ymax - ymin + _textureSize
    );
    _boardView.reset(_boardBoundingBox);

    // Initialize misc. info
    updatePlayerInfo(1, lastTurnNumber, playersInfo);
    _score = score;
    updateCellCount(cellCount);
}

void HexabombRenderer::onTurn(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs,
    const std::map<int, int> & score,
    const std::map<int, int> & cellCount,
    int currentTurnNumber,
    int lastTurnNumber,
    const std::vector<netorcai::PlayerInfo> & playersInfo)
{
    _pInfoTexts.clear();
    _pInfoRectShapes.clear();
    _ccdRectShapes.clear();

    _nbNeutralCells = 0;
    for (const auto & [coord, cell] : cells)
    {
        auto * sprite = _cellSprites[coord];
        sprite->setColor(_colors[cell.color]);

        if (cell.color == 0)
            _nbNeutralCells++;
    }

    _aliveCharacters.resize(0);
    for (const auto & character : characters)
    {
        auto * sprite = _characterSprites[character.id];
        sprite->setPosition(axialToCartesian(character.coord));

        if (character.isAlive)
            _aliveCharacters.push_back(sprite);
    }

    for (auto * sprite : _bombSprites)
        delete sprite;
    _bombSprites.clear();

    for (const auto & bomb : bombs)
    {
        auto * sprite = new sf::Sprite;
        sprite->setTexture(_bombTexture);
        sprite->setPosition(axialToCartesian(bomb.coord));
        sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
        sprite->setScale(_bombScale);

        _bombSprites.push_back(sprite);
    }

    // Update misc. info
    updatePlayerInfo(currentTurnNumber, lastTurnNumber, playersInfo);
    _score = score;
    updateCellCount(cellCount);
}

void HexabombRenderer::updatePlayerInfo(int currentTurnNumber,
    int lastTurnNumber,
    const std::vector<netorcai::PlayerInfo> & playersInfo)
{
    // Update raw data
    if (playersInfo.empty())
    {
        // Probably coming from a GAME_ENDS.
        for (auto & info : _playersInfo)
        {
            // Empty the address of non-disconnected players.
            if (info.remoteAddress.find("disconnected") != std::string::npos)
                info.remoteAddress = "";
        }
    }
    else if (playersInfo.size() == _playersInfo.size())
    {
        // Updating: Not the first turn nor the last.
        // Change address of disconnected players.
        for (unsigned int i = 0; i < _playersInfo.size(); i++)
        {
            if (_playersInfo[i].isConnected && !playersInfo[i].isConnected)
            {
                _playersInfo[i].isConnected = false;
                _playersInfo[i].remoteAddress = "conn. lost (" + std::to_string(currentTurnNumber) + ")";
            }
        }
    }
    else
        _playersInfo = playersInfo;

    // Update rendering data
    const float hPlayers = 100.f;
    const float hLines = 20.f;
    const float rectX = 2.f;
    const float textX = 4.f;
    const float barThickness = 1.f;

    sf::Text text;
    text.setFont(_monospaceFont);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::Black);

    for (unsigned int i = 0; i < _playersInfo.size(); i++)
    {
        const netorcai::PlayerInfo & info = _playersInfo[i];
        int j = -1;

        sf::RectangleShape rect(sf::Vector2f(_piRectWidth, hPlayers));
        rect.setFillColor(_colors[i+1]);
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(sf::Color::Black);
        rect.setPosition(rectX, hPlayers*i);
        _pInfoRectShapes.push_back(rect);

        j++;
        text.setPosition(textX, hPlayers*i + hLines*j);
        text.setString(info.nickname + " (" + std::to_string(info.playerID) + ")");
        _pInfoTexts.push_back(text);

        j++;
        text.setPosition(textX, hPlayers*i + hLines*j);
        text.setString("  score: " + std::to_string(_score[i]));
        _pInfoTexts.push_back(text);

        j++;
        text.setPosition(textX, hPlayers*i + hLines*j);
        text.setString("  #cells: " + std::to_string(_cellCount[i]));
        _pInfoTexts.push_back(text);

        j++;
        text.setPosition(textX, hPlayers*i + hLines*j);
        text.setString("  " + info.remoteAddress);
        _pInfoTexts.push_back(text);

        if (!info.isConnected)
        {
            const float diagonalLength = sqrt(_piRectWidth*_piRectWidth+hPlayers*hPlayers);
            rect.setFillColor(sf::Color::Black);
            rect.setOutlineThickness(1.f);

            rect.setSize(sf::Vector2f(diagonalLength, barThickness));
            rect.setPosition(rectX, hPlayers*i);
            rect.setOrigin(diagonalLength/2, barThickness/2);
            rect.setRotation(acos(_piRectWidth/diagonalLength) * 180.f/M_PI);
            rect.setOrigin(0.f, 0.f);
            _pInfoRectShapes.push_back(rect);

            rect.setPosition(rectX, hPlayers*(i+1));
            rect.setOrigin(diagonalLength/2, barThickness/2);
            rect.setRotation(-acos(_piRectWidth/diagonalLength) * 180.f/M_PI);
            rect.setOrigin(0.f, 0.f);
            _pInfoRectShapes.push_back(rect);
        }
    }
}

void HexabombRenderer::updateCellCount(const std::map<int, int> & cellCount)
{
    _cellCount = cellCount;

    const float nbCells = _cellSprites.size();

    sf::RectangleShape rect;
    float width = _ccdWidth * _nbNeutralCells / nbCells;
    float offX = 0.f;

    rect.setSize(sf::Vector2f(width, _ccdHeight));
    rect.setFillColor(_colors[0]);
    rect.setPosition(offX, 0.f);
    _ccdRectShapes.push_back(rect);

    for (const auto & it : _cellCount)
    {
        const int & playerID = it.first;
        const int nbPlayerCells = it.second;

        offX += width;
        width = _ccdWidth * nbPlayerCells / nbCells;
        rect.setSize(sf::Vector2f(width, _ccdHeight));
        rect.setFillColor(_colors[playerID + 1]);
        rect.setPosition(offX, 0.f);
        _ccdRectShapes.push_back(rect);
    }
}

void HexabombRenderer::render(sf::RenderWindow & window)
{
    // Clear the window
    window.clear(_backgroundColor);

    // Set view and viewport. Should not be done at each frame
    window.setView(_boardView);

    // Draw cells
    for (const auto & [coord, sprite] : _cellSprites)
    {
        window.draw(*sprite);
    }

    // Draw characters
    for (const auto & sprite : _aliveCharacters)
    {
        window.draw(*sprite);
    }

    // Draw bombs
    for (const auto & sprite : _bombSprites)
    {
        window.draw(*sprite);
    }

    // Draw player informations
    window.setView(_playersInfoView);
    for (const auto & shape : _pInfoRectShapes)
    {
        window.draw(shape);
    }

    for (const auto & text : _pInfoTexts)
    {
        window.draw(text);
    }

    // Draw cell count distribution
    window.setView(_cellCountDistributionView);
    for (const auto & shape : _ccdRectShapes)
    {
        window.draw(shape);
    }

    // Finally update the screen
    window.display();
}

void HexabombRenderer::updateView(int newWidth, int newHeight)
{
    // Keep aspect ratio with a resizable window.
    // https://en.sfml-dev.org/forums/index.php?topic=15802.msg113936#msg113936
    float screenWidth = newWidth - _piRectWidth;
    float screenHeight = newHeight * (1-_ccdHeightRatioInScreen);

    sf::FloatRect viewport;
    viewport.width = 1.f;
    viewport.height = 1.f;

    if(screenWidth > screenHeight)
    {
        viewport.width = screenHeight / screenWidth;
        viewport.left = (1.f - viewport.width) / 2.f;
    }
    else if(screenWidth < screenHeight)
    {
        viewport.height = screenWidth / screenHeight;
        viewport.top = (1.f - viewport.height) / 2.f;
    }

    viewport.width *= (1-(_piRectWidth/newWidth));
    viewport.height *= (1-_ccdHeightRatioInScreen);
    _boardView.setViewport(viewport);

    // Players misc. information.
    _playersInfoView.reset(sf::FloatRect(0.f, 0.f, newWidth, newHeight));
    _playersInfoView.setViewport(sf::FloatRect(1-(_piRectWidth/newWidth), 0.f, 1.f, 1.f));

    // Cell count distribution
    _cellCountDistributionView.reset(sf::FloatRect(0.f, 0.f, _ccdWidth, _ccdHeight));
    _cellCountDistributionView.setViewport(sf::FloatRect(0.f, 1-_ccdHeightRatioInScreen, 1.f, 1.f));
}

void HexabombRenderer::generatePlayerColors(int nbColors)
{
    // Viridis color palette. Generated in R: viridis_pal(begin=0.2, direction=-1)(6)
    std::vector<sf::Color> viridis = {
        sf::Color(0xfde725ff),
        sf::Color(0x93d741ff),
        sf::Color(0x3aba76ff),
        sf::Color(0x1f958bff),
        sf::Color(0x2e6f8eff),
        sf::Color(0x414487ff)
    };

    // This is to maximize color distance between players if the number of players is small.
    int traversal_order[6] = {0, 5, 2, 4, 1, 3};

    // Generate the palette.
    _colors.clear();
    _colors.push_back(sf::Color::White); // Neutral

    while ((int)_colors.size() < nbColors + 1)
    {
        for (int i = 0; i < 6; i++)
            _colors.push_back(viridis[traversal_order[i]]);
    }
}
