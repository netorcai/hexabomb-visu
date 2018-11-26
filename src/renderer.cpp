#include "renderer.hpp"

HexabombRenderer::HexabombRenderer()
{

}

HexabombRenderer::~HexabombRenderer()
{

}

void HexabombRenderer::onGameInit(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs)
{

}

void HexabombRenderer::onTurn(
    const std::unordered_map<Coordinates, Cell> & cells,
    const std::vector<Character> & characters,
    const std::vector<Bomb> & bombs,
    const std::map<int, int> & score,
    const std::map<int, int> & cellCount)
{

}

void HexabombRenderer::render(sf::RenderWindow & window)
{

}
