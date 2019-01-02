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
    const std::vector<Bomb> & bombs)
{
    float xmin = std::numeric_limits<float>::max();
    float ymin = std::numeric_limits<float>::max();
    float xmax = std::numeric_limits<float>::min();
    float ymax = std::numeric_limits<float>::min();

    generatePlayerColors(2);

    for (const auto & [coord, cell] : cells)
    {
        auto * sprite = new sf::Sprite;
        sprite->setTexture(_emptyTexture);

        sf::Vector2f cartesian = axialToCartesian(coord);
        sprite->setPosition(cartesian);
        sprite->setOrigin(sf::Vector2f(_textureSize/2.0, _textureSize/2.0));
        sprite->setColor(_colors[cell.color]);

        _cellSprites[coord] = sprite;

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
}

void HexabombRenderer::onTurn(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs,
    const std::map<int, int> & score,
    const std::map<int, int> & cellCount)
{
    for (const auto & [coord, cell] : cells)
    {
        auto * sprite = _cellSprites[coord];
        sprite->setColor(_colors[cell.color]);
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

    // Finally update the screen
    window.display();
}

void HexabombRenderer::updateView(int newWidth, int newHeight)
{
    // https://en.sfml-dev.org/forums/index.php?topic=15802.msg113936#msg113936
    float screenWidth = newWidth;
    float screenHeight = newHeight;

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

    _boardView.setViewport(viewport);
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
