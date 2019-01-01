#pragma once

#include <boost/lockfree/queue.hpp>

enum class MessageType
{
    // From Network to Renderer
    GAME_STARTS,
    GAME_ENDS,
    TURN,
    ERROR,

    // From Renderer to Network
    TERMINATE
};

struct Message
{
    MessageType type;
    void * data = nullptr;
};

void network_thread_function(boost::lockfree::queue<Message> * from_renderer,
    boost::lockfree::queue<Message> * to_renderer,
    const std::string & hostname, uint16_t port);

void renderer_thread_function(boost::lockfree::queue<Message> * from_network,
    boost::lockfree::queue<Message> * to_network);

void flush_queues(boost::lockfree::queue<Message> * to_network,
    boost::lockfree::queue<Message> * to_renderer);
