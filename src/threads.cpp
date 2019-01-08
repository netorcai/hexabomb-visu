#include "threads.hpp"

#include <netorcai-client-cpp/client.hpp>
#include <netorcai-client-cpp/error.hpp>

#include "hexabomb-parse.hpp"
#include "renderer.hpp"

using namespace netorcai;

void network_thread_function(boost::lockfree::queue<Message> * from_renderer,
    boost::lockfree::queue<Message> * to_renderer,
    const std::string & hostname, uint16_t port)
{
    try
    {
        netorcai::Client c;
        Message msg;
        GameStartsMessage * gameStarts = nullptr;
        GameEndsMessage * gameEnds = nullptr;
        TurnMessage * turn = nullptr;
        bool shouldQuit = false;

        printf("Connecting to netorcai (%s:%d)... ", hostname.c_str(), port); fflush(stdout);
        c.connect(hostname, port);
        printf("done\n");

        printf("Logging in as a visualization... "); fflush(stdout);
        c.sendLogin("sfml-visu", "visualization");
        c.readLoginAck();
        printf("done\n");

        while (!shouldQuit)
        {
            std::string msgStr;
            if (c.recvStringNonBlocking(msgStr, 50.0))
            {
                // A message has been received.
                json msgJson = json::parse(msgStr);
                if (msgJson["message_type"] == "TURN")
                {
                    turn = new TurnMessage;
                    *turn = parseTurnMessage(msgJson);

                    printf("Received TURN %d\n", turn->turnNumber); fflush(stdout);

                    // Only forward TURN if the queue is empty.
                    // This avoids flooding the renderer if it is slower than the network.
                    if (to_renderer->empty())
                    {
                        msg.type = MessageType::TURN;
                        msg.data = (void*) turn;
                        to_renderer->push(msg);
                    }
                    else
                        delete turn;

                    // Send TURN_ACK to netorcai, so future turns can be received.
                    c.sendTurnAck(turn->turnNumber, json::parse(R"([])"));
                }
                else if (msgJson["message_type"] == "KICK")
                {
                    msg.type = MessageType::ERROR;
                    asprintf((char**)&msg.data, "%s",
                        ((std::string)msgJson["kick_reason"]).c_str());
                    printf("Kicked from netorcai. Reason: %s", (char*) msg.data);
                    to_renderer->push(msg);
                    shouldQuit = true;
                }
                if (msgJson["message_type"] == "GAME_STARTS")
                {
                    printf("Received GAME_STARTS\n"); fflush(stdout);
                    gameStarts = new GameStartsMessage;
                    *gameStarts = parseGameStartsMessage(msgJson);

                    msg.type = MessageType::GAME_STARTS;
                    msg.data = (void*) gameStarts;
                    to_renderer->push(msg);
                }
                else if (msgJson["message_type"] == "GAME_ENDS")
                {
                    printf("Received GAME_ENDS\n"); fflush(stdout);
                    gameEnds = new GameEndsMessage;
                    *gameEnds = parseGameEndsMessage(msgJson);

                    msg.type = MessageType::GAME_ENDS;
                    msg.data = (void*) gameEnds;
                    to_renderer->push(msg);
                    shouldQuit = true;
                }
            }

            // Look whether termination has been requested.
            if (!from_renderer->empty())
            {
                Message msg;
                from_renderer->pop(msg);

                if (msg.type == MessageType::TERMINATE)
                {
                    shouldQuit = true;
                }
            }
        }
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
    int nbTurnsMax = -1;

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
                parseGameState(gameStarts->initialGameState, cells, characters, bombs, score, cellCount);
                nbTurnsMax = gameStarts->nbTurnsMax;
                renderer.onGameInit(cells, characters, bombs, score, cellCount, nbTurnsMax, gameStarts->playersInfo);
                delete gameStarts;
                initialized = true;
            }
            else if (msg.type == MessageType::TURN)
            {
                auto turn = (TurnMessage *) msg.data;
                parseGameState(turn->gameState, cells, characters, bombs, score, cellCount);
                renderer.onTurn(cells, characters, bombs, score, cellCount, turn->turnNumber, nbTurnsMax, turn->playersInfo);
                delete turn;
            }
            else if (msg.type == MessageType::GAME_ENDS)
            {
                auto gameEnds = (GameEndsMessage *) msg.data;
                parseGameState(gameEnds->gameState, cells, characters, bombs, score, cellCount);
                // TODO: print something on the screen.
                renderer.onTurn(cells, characters, bombs, score, cellCount, nbTurnsMax, nbTurnsMax);
                delete gameEnds;
            }
            else if (msg.type == MessageType::ERROR)
            {
                if (!initialized)
                    window.close();
                // TODO: print something on screen otherwise
                free((char *) msg.data);
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

void flush_queues(boost::lockfree::queue<Message> * to_network,
    boost::lockfree::queue<Message> * to_renderer)
{
    Message msg;
    while (to_network->pop(msg));

    while (to_renderer->pop(msg))
    {
        if (msg.type == MessageType::GAME_STARTS)
            delete (GameStartsMessage*) msg.data;
        else if (msg.type == MessageType::TURN)
            delete (TurnMessage*) msg.data;
        else if (msg.type == MessageType::GAME_ENDS)
            delete (GameEndsMessage*) msg.data;
        else if (msg.type == MessageType::ERROR)
            free((char*) msg.data);
    }
}
