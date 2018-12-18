#include "threads.hpp"

#include <netorcai-client-cpp/client.hpp>
#include <netorcai-client-cpp/error.hpp>

#include "hexabomb-parse.hpp"
#include "renderer.hpp"

using namespace netorcai;

void network_thread_function(boost::lockfree::queue<Message> * from_renderer,
    boost::lockfree::queue<Message> * to_renderer)
{
    try
    {
        netorcai::Client c;
        Message msg;
        GameStartsMessage * gameStartsOnHeap = nullptr;
        GameEndsMessage * gameEndsOnHeap = nullptr;
        TurnMessage * turnOnHeap = nullptr;

        printf("Connecting to netorcai... "); fflush(stdout);
        c.connect();
        printf("done\n");

        printf("Logging in as a visualization... "); fflush(stdout);
        c.sendLogin("sfml-visu", "visualization");
        c.readLoginAck();
        printf("done\n");

        printf("Waiting for GAME_STARTS... "); fflush(stdout);
        const GameStartsMessage gameStarts = c.readGameStarts();
        printf("done\n");

        // Forward GAME_STARTS message to renderer.
        msg.type = MessageType::GAME_STARTS;
        gameStartsOnHeap = new GameStartsMessage;
        *gameStartsOnHeap = gameStarts;
        msg.data = (void*) gameStartsOnHeap;
        to_renderer->push(msg);

        // TODO: make this robust, messages can be lost.
        for (int i = 1; i < gameStarts.nbTurnsMax; i++)
        {
            // Look whether termination has been requested.
            if (!from_renderer->empty())
            {
                Message msg;
                from_renderer->pop(msg);

                if (msg.type == MessageType::TERMINATE)
                {
                    c.close();
                    return;
                }
            }

            printf("Waiting for TURN... "); fflush(stdout);
            const TurnMessage turn = c.readTurn();
            c.sendTurnAck(turn.turnNumber, json::parse(R"([])"));
            printf("done\n");

            if (to_renderer->empty())
            {
                // Forward TURN to renderer.
                msg.type = MessageType::TURN;
                turnOnHeap = new TurnMessage;
                *turnOnHeap = turn;
                msg.data = (void*) turnOnHeap;
                to_renderer->push(msg);
            }
        }

        printf("Waiting for GAME_ENDS... "); fflush(stdout);
        const GameEndsMessage gameEnds = c.readGameEnds();
        printf("done\n");

        // Forward GAME_ENDS to renderer.
        msg.type = MessageType::GAME_ENDS;
        gameEndsOnHeap = new GameEndsMessage;
        *gameEndsOnHeap = gameEnds;
        msg.data = (void*) gameEndsOnHeap;
        to_renderer->push(msg);
    }
    catch (const netorcai::Error & e)
    {
        printf("Failure: %s\n", e.what());

        // Forward ERROR to renderer.
        Message msg;
        msg.type = MessageType::ERROR;
        asprintf((char**)&msg.data, "%s", e.what());
        to_renderer->push(msg);
    }
}

void renderer_thread_function(boost::lockfree::queue<Message> * from_network,
    boost::lockfree::queue<Message> * to_network)
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "hexabomb-visu");
    window.setFramerateLimit(60);
    HexabombRenderer renderer;

    std::unordered_map<Coordinates, Cell> cells;
    std::vector<Character> characters;
    std::vector<Bomb> bombs;
    std::map<int, int> score, cellCount;

    bool initialized = false;

    while (window.isOpen())
    {
        // Check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // User wanted to close the window.
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::Resized)
                renderer.updateView(event.size.width, event.size.height);
        }

        // Something has been received from the network?
        if (!from_network->empty())
        {
            Message msg;
            from_network->pop(msg);
            if (msg.type == MessageType::GAME_STARTS)
            {
                auto gameStarts = (GameStartsMessage *) msg.data;
                parseInitialGameState(gameStarts->initialGameState, cells, characters, bombs);
                renderer.onGameInit(cells, characters, bombs);
                delete gameStarts;
                initialized = true;
            }
            else if (msg.type == MessageType::TURN)
            {
                auto turn = (TurnMessage *) msg.data;
                parseGameState(turn->gameState, cells, characters, bombs, score, cellCount);
                renderer.onTurn(cells, characters, bombs, score, cellCount);
                delete turn;
            }
            else if (msg.type == MessageType::ERROR)
            {
                if (!initialized)
                    window.close();
                // TODO: print something on screen otherwise
            }
        }

        // Render on the window
        renderer.render(window);
    }

    // Window closed. Ask the network to terminate gently.
    Message msg;
    msg.type = MessageType::TERMINATE;
    to_network->push(msg);
}
