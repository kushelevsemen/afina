#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_H

#include <mutex>
#include <vector>
#include <atomic>
#include <thread>

#include <afina/network/Server.h>

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun();

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publisj changes cross thread
    // bounds
    std::atomic<bool> running;

    // Server socket to accept connections on		
    int _server_socket;

    // Thread to run network on
    std::thread _thread;
    
    
    
    //*******************************************
    // Maximum amount of workers
    uint32_t _n_workers;

    // Workers
    using Worker = struct Worker {
        Worker(): thr{}, is_running{false} {};
        std::thread thr;
        bool is_running;
    };
    
    std::mutex _workers_m;
    std::vector<Worker> _workers;

    // If free worker exists, atomically marks him as busy and returns his number
    // Returns -1 if all the workers are busy
    int FindFreeWorker();

    // Function for worker
    void Worker_Run(int client_socket, Worker *worker_ptr);
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
