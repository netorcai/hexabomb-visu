#include "renderer.hpp"

#include <algorithm>
#include <random>

#include "util.hpp"

sf::Vector2f HexabombRenderer::axialToCartesian(Coordinates axial) const
{
    double base_length = _hexBaseLength + _hexOutlineThickness;

    sf::Vector2f res;
    res.x = base_length * (sqrt(3.0) * axial.q + sqrt(3.0) / 2.0 * axial.r);
    res.y = base_length * (                           3.0  / 2.0 * axial.r);

    return res;
}

HexabombRenderer::HexabombRenderer()
{
    _bombTexture.loadFromFile(searchImageAbsoluteFilename("bomb.png"));
    _characterTexture.loadFromFile(searchImageAbsoluteFilename("char.png"));
    _deadCharacterTexture.loadFromFile(searchImageAbsoluteFilename("char_dead.png"));
    _specialCharacterTexture.loadFromFile(searchImageAbsoluteFilename("char_special.png"));
    _explosionTexture.loadFromFile(searchImageAbsoluteFilename("explosion.png"));

    _bombTexture.setSmooth(true);
    _characterTexture.setSmooth(true);

    _monospaceFont.loadFromFile(searchFontAbsoluteFilename("DejaVuSansMono.ttf"));

    _statusText.setFont(_monospaceFont);
    _statusText.setCharacterSize(20);
    _statusText.setFillColor(sf::Color::Black);
}

HexabombRenderer::~HexabombRenderer()
{
    for (const auto & [coord, sprite] : _cellShapes)
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

    float xminHex = xmin;
    float yminHex = ymin;
    float xmaxHex = xmax;
    float ymaxHex = ymax;

    generatePlayerColors(2);

    _nbNeutralCells = 0;

    sf::CircleShape hex(_hexBaseLength, 6);
    sf::Vector2f hexCenter;
    for (size_t i = 0; i < hex.getPointCount(); i++)
    {
        auto p = hex.getPoint(i);
        hexCenter += p;
        if (p.x < xminHex) xminHex = p.x;
        if (p.y < yminHex) yminHex = p.y;
        if (p.x > xmaxHex) xmaxHex = p.x;
        if (p.y > ymaxHex) ymaxHex = p.y;
    }
    hexCenter = hexCenter / (float)hex.getPointCount();
    float hexWidth = xmaxHex - xminHex;
    float hexHeight = ymaxHex - yminHex;

    for (const auto & [coord, cell] : cells)
    {
        auto * shape = new sf::CircleShape(_hexBaseLength, 6);

        sf::Vector2f cartesian = axialToCartesian(coord);
        shape->setPosition(cartesian);
        shape->setOrigin(hexCenter);

        int drawColor = cell.color;
        if (_isSuddenDeath)
            drawColor = 0;
        shape->setFillColor(_colors[drawColor]);

        _cellShapes[coord] = shape;

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
        _charactersToDraw.push_back(sprite);

        if (_isSuddenDeath)
        {
            // Change texture of special characters
            if (character.color == 1)
                sprite->setTexture(_specialCharacterTexture);

            auto * cellSprite = _cellShapes[character.coord];
            cellSprite->setFillColor(_colors[character.color]);
        }
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
        xmin - hexWidth/2.f - 2*_hexOutlineThickness,
        ymin - hexHeight/2.f - 2*_hexOutlineThickness,
        xmax - xmin + hexWidth + 4*_hexOutlineThickness,
        ymax - ymin + hexHeight + 4*_hexOutlineThickness
    );
    _boardView.reset(_boardBoundingBox);

    // Initialize misc. info
    _score = score;
    updatePlayerInfo(0, lastTurnNumber, playersInfo);
    updateCellCount(cellCount);
}

void HexabombRenderer::onTurn(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs,
    const std::unordered_map<int, std::vector<Coordinates> > & explosions,
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
        auto * sprite = _cellShapes[coord];
        int drawColor = cell.color;
        if (_isSuddenDeath)
            drawColor = 0;
        sprite->setFillColor(_colors[drawColor]);

        if (cell.color == 0)
            _nbNeutralCells++;
    }

    _charactersToDraw.resize(0);
    for (const auto & character : characters)
    {
        auto * sprite = _characterSprites[character.id];
        sprite->setPosition(axialToCartesian(character.coord));

        if (_isSuddenDeath && character.isAlive)
        {
            auto * cellSprite = _cellShapes[character.coord];
            cellSprite->setFillColor(_colors[character.color]);
        }

        if (!_isSuddenDeath)
        {
            // Change texture if the character alive state changed
            auto texture = sprite->getTexture();
            if (character.isAlive && texture != &_characterTexture)
                sprite->setTexture(_characterTexture);
            else if (!character.isAlive && texture != &_deadCharacterTexture)
                sprite->setTexture(_deadCharacterTexture);
        }

        // Hide dead characters in sudden death
        if (character.isAlive || !_isSuddenDeath)
            _charactersToDraw.push_back(sprite);
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

    for (auto * sprite : _explosionSprites)
        delete sprite;
    _explosionSprites.clear();

    for (const auto& [color, coordinates] : explosions)
    {
        for (const auto& coord : coordinates)
        {
            auto * sprite = new sf::Sprite;
            sprite->setTexture(_explosionTexture);
            sprite->setPosition(axialToCartesian(coord));
            sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
            sprite->setScale(_explosionScale);

            _explosionSprites.push_back(sprite);
        }
    }

    // Update misc. info
    _score = score;
    updatePlayerInfo(currentTurnNumber, lastTurnNumber, playersInfo);
    updateCellCount(cellCount);
}

void HexabombRenderer::updatePlayerInfo(int currentTurnNumber,
    int lastTurnNumber,
    const std::vector<netorcai::PlayerInfo> & playersInfo)
{
    // Update raw data
    if (playersInfo.empty())
    {
        // Coming from a GAME_ENDS.
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

    typedef std::tuple<int, int, int> ScorePidIndex;
    std::vector<ScorePidIndex> pInfoTravarsalOrder;
    for (int i = 0; i < _playersInfo.size(); i++)
    {
        const auto & info = _playersInfo[i];
        pInfoTravarsalOrder.push_back(std::make_tuple(_score[info.playerID], info.playerID, i));
    }

    if (_isSuddenDeath)
    {
        // Sort traversal order by decreasing score.
      std::sort(pInfoTravarsalOrder.begin(), pInfoTravarsalOrder.end(), std::greater<std::tuple<int,int,int>>());
    }

    // Update rendering data
    const float baseH = 70.f;
    float hPlayers = 100.f;
    if (_isSuddenDeath)
        hPlayers = 75.f;
    const float hLines = 20.f;
    const float rectX = 2.f;
    const float textX = 4.f;
    const float barThickness = 1.f;

    sf::Text text = _statusText;

    text.setPosition(textX, 0.f);
    char * turnCString = nullptr;
    asprintf(&turnCString, "turn: %0*d/%d", (int)log10f(lastTurnNumber)+1, currentTurnNumber, lastTurnNumber);
    text.setString(std::string(turnCString));
    free(turnCString);
    _pInfoTexts.push_back(text);

    _statusText.setPosition(textX, hLines);

    for (int i = 0; i < pInfoTravarsalOrder.size(); i++)
    {
        const auto & scorePidIndex = pInfoTravarsalOrder[i];
        const int originalI = std::get<2>(scorePidIndex);
        const netorcai::PlayerInfo & info = _playersInfo[originalI];
        int j = -1;

        sf::RectangleShape rect(sf::Vector2f(_piRectWidth, hPlayers));
        rect.setFillColor(_colors[info.playerID+1]);
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(sf::Color::Black);
        rect.setPosition(rectX, baseH + hPlayers*i);
        _pInfoRectShapes.push_back(rect);

        j++;
        text.setPosition(textX, baseH + hPlayers*i + hLines*j);
        text.setString(info.nickname + " (" + std::to_string(info.playerID) + ")");
        _pInfoTexts.push_back(text);

        j++;
        text.setPosition(textX, baseH + hPlayers*i + hLines*j);
        text.setString("  score: " + std::to_string(_score[info.playerID]));
        _pInfoTexts.push_back(text);

        if (!_isSuddenDeath)
        {
            j++;
            text.setPosition(textX, baseH + hPlayers*i + hLines*j);
            text.setString("  #cells: " + std::to_string(_cellCount[info.playerID]));
            _pInfoTexts.push_back(text);
        }

        j++;
        text.setPosition(textX, baseH + hPlayers*i + hLines*j);
        text.setString("  " + info.remoteAddress);
        _pInfoTexts.push_back(text);

        if (!info.isConnected)
        {
            const float diagonalLength = sqrt(_piRectWidth*_piRectWidth+hPlayers*hPlayers);
            rect.setFillColor(sf::Color::Black);
            rect.setOutlineThickness(1.f);

            rect.setSize(sf::Vector2f(diagonalLength, barThickness));
            rect.setPosition(rectX, baseH + hPlayers*i);
            rect.setOrigin(diagonalLength/2, barThickness/2);
            rect.setRotation(acos(_piRectWidth/diagonalLength) * 180.f/M_PI);
            rect.setOrigin(0.f, 0.f);
            _pInfoRectShapes.push_back(rect);

            rect.setPosition(rectX, baseH + hPlayers*(i+1));
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

    const float nbCells = _cellShapes.size();

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

void HexabombRenderer::onStatusChange(const std::string & status)
{
    if (_status != "game over")
    {
        _status = status;
        _statusText.setString(_status);
    }
}

void HexabombRenderer::render(sf::RenderWindow & window)
{
    // Clear the window
    window.clear(_backgroundColor);

    // Set view and viewport. Should not be done at each frame
    window.setView(_boardView);

    // Draw cells borders. Copy is delibarate here.
    for (const auto & [coord, _] : _cellShapes)
    {
        sf::CircleShape blackHex(128.f + 16.f, 6);
        sf::Vector2f cartesian = axialToCartesian(coord);
        blackHex.setPosition(cartesian);

        sf::Vector2f center;
        for (size_t i = 0; i < blackHex.getPointCount(); i++)
            center += blackHex.getPoint(i);
        center = center / (float)blackHex.getPointCount();

        blackHex.setOrigin(center);
        blackHex.setFillColor(sf::Color::Black);

        window.draw(blackHex);
    }

    // Draw cells
    for (const auto & [coord, shape] : _cellShapes)
    {
        window.draw(*shape);

        const int charSize = 64;
        const sf::Glyph glyph = _monospaceFont.getGlyph('0', charSize, false);

        if (_showCoordinates)
        {
            sf::Text text;
            text.setFont(_monospaceFont);
            text.setCharacterSize(charSize);
            text.setFillColor(sf::Color::Black);
            text.setString("(" + std::to_string(coord.q) + "," + std::to_string(coord.r) + ")");
            text.setOrigin(1.1f*(text.getString().getSize() * glyph.bounds.width / 2.f), 1.1f*(glyph.bounds.height/2.f));

            sf::Vector2f cartesian = axialToCartesian(coord);
            text.setPosition(cartesian);
            window.draw(text);
        }
    }

    // Draw characters
    for (const auto & sprite : _charactersToDraw)
    {
        window.draw(*sprite);
    }

    // Draw bombs
    for (const auto & sprite : _bombSprites)
    {
        window.draw(*sprite);
    }

    // Draw explosions
    for (const auto & sprite : _explosionSprites)
    {
        window.draw(*sprite);
    }

    // Draw player informations
    window.setView(_playersInfoView);
    window.draw(_statusText);
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

void HexabombRenderer::toggleShowCoordinates()
{
    _showCoordinates = !_showCoordinates;
}

void HexabombRenderer::setSuddenDeath(bool isSuddenDeath)
{
    _isSuddenDeath = isSuddenDeath;
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
    if (_isSuddenDeath)
        _colors.push_back(sf::Color::White); // Special player

    while ((int)_colors.size() < nbColors + 1)
    {
        for (int i = 0; i < 6; i++)
            _colors.push_back(viridis[traversal_order[i]]);
    }
}
