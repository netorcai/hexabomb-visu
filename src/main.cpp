#include <stdio.h>

#include <netorcai-client-cpp/client.hpp>
#include <netorcai-client-cpp/error.hpp>

#include "hexabomb-parse.hpp"
#include "renderer.hpp"

int main()
{
    using namespace netorcai;

    sf::RenderWindow window(sf::VideoMode(800, 600), "hexabomb-visu");
    HexabombRenderer renderer;
    window.display();

    try
    {
        netorcai::Client c;
        std::unordered_map<Coordinates, Cell> cells;
        std::vector<Character> characters;
        std::vector<Bomb> bombs;
        std::map<int, int> score, cellCount;

        printf("Connecting to netorcai... "); fflush(stdout);
        c.connect();
        printf("done\n");

        printf("Logging in as a player... "); fflush(stdout);
        c.sendLogin("sfml-visu", "visualization");
        c.readLoginAck();
        printf("done\n");

        printf("Waiting for GAME_STARTS... "); fflush(stdout);
        const GameStartsMessage gameStarts = c.readGameStarts();
        printf("done\n");

        //printf("GAME_STARTS game state:\n%s\n", gameStarts.initialGameState.dump().c_str());
        parseInitialGameState(gameStarts.initialGameState, cells, characters, bombs);

        renderer.onGameInit(cells, characters, bombs);
        renderer.render(window);

        for (int i = 1; i < gameStarts.nbTurnsMax; i++)
        {
            printf("Waiting for TURN... "); fflush(stdout);
            const TurnMessage turn = c.readTurn();
            c.sendTurnAck(turn.turnNumber, json::parse(R"([])"));
            printf("done\n");

            //printf("TURN %d game state:\n%s\n", i, turn.gameState.dump().c_str());
            parseGameState(turn.gameState, cells, characters, bombs, score, cellCount);
            renderer.onTurn(cells, characters, bombs, score, cellCount);
            renderer.render(window);
        }

        printf("Waiting for GAME_ENDS... "); fflush(stdout);
        const GameEndsMessage gameEnds = c.readGameEnds();
        printf("done\n");

        return 0;
    }
    catch (const netorcai::Error & e)
    {
        printf("Failure: %s\n", e.what());
        return 1;
    }
}
