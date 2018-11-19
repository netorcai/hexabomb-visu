#include <stdio.h>

#include <netorcai-client-cpp/client.hpp>
#include <netorcai-client-cpp/error.hpp>

int main()
{
    using namespace netorcai;

    try
    {
        Client c;

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

        for (int i = 1; i < gameStarts.nbTurnsMax; i++)
        {
            printf("Waiting for TURN... "); fflush(stdout);
            const TurnMessage turn = c.readTurn();
            c.sendTurnAck(turn.turnNumber, json::parse(R"([{"player": "D"}])"));
            printf("done\n");
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
