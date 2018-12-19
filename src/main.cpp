#include <stdio.h>

#include <thread>

#include "threads.hpp"

int main()
{
    boost::lockfree::queue<Message> to_network(2);
    boost::lockfree::queue<Message> to_renderer(2);

    auto network_thread = std::thread(network_thread_function, &to_network, &to_renderer);
    renderer_thread_function(&to_renderer, &to_network);

    network_thread.join();
    flush_queues(&to_network, &to_renderer);

    return 0;
}
