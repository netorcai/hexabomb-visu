#include <stdio.h>

#include <iostream>
#include <string>
#include <thread>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include "threads.hpp"

int main(int argc, char * argv[])
{
    // Parse main arguments.
    std::string hostname = "localhost";
    uint16_t port = 4242;

    namespace po = boost::program_options;
    po::options_description desc("Options description");
    desc.add_options()
            ("help", "print usage message")
            ("hostname,h", po::value(&hostname),
             "netorcai instance's hostname")
            ("port,p", po::value(&port),
             "netorcai instance's TCP port")
            ;

    try
    {
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm); // throws on error

        if (vm.count("help") > 0)
        {
            printf("Usage : %s [options]\n\n", argv[0]);
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);
    }
    catch(boost::program_options::required_option& e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch(boost::program_options::error& e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    // End of argument parsing.
    boost::lockfree::queue<Message> to_network(2);
    boost::lockfree::queue<Message> to_renderer(2);

    auto network_thread = std::thread(network_thread_function,
        &to_network, &to_renderer, hostname, port);
    renderer_thread_function(&to_renderer, &to_network);

    network_thread.join();
    flush_queues(&to_network, &to_renderer);

    return 0;
}
