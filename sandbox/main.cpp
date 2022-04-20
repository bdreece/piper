#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

#include "piper/spmc.hpp"
using namespace piper::spmc;

#define N 1

struct Worker {
        int id;
        std::mutex &mutex;
        Sender<int> tx;
        Receiver<int> rx;
        std::thread job;
        Worker(int id, std::mutex &mutex, Receiver<int> &&rx)
            : id{id}, mutex{mutex}, tx{N}, rx{rx} {
            job = std::thread([this]() {
                int value;
                std::unique_ptr<Worker> child;
                while (1) {
                    this->rx >> value;
                    if (value < 0) {
                        // STOP condition
                        auto guard = std::lock_guard(this->mutex);
                        std::cout << "    Thread " << this->id
                                  << " received stop condition" << std::endl;
                        break;
                    } else if (value % this->id == 0) {
                        // Not prime
                        auto guard = std::lock_guard(this->mutex);
                        std::cout << "    Thread " << this->id
                                  << " received non-prime: " << value
                                  << std::endl;
                        continue;
                    } else {
                        // Prime
                        {
                            auto guard = std::lock_guard(this->mutex);
                            std::cout << "    Thread " << this->id
                                      << " received prime: " << value
                                      << std::endl;
                        }
                        if (!child) {
                            {
                                auto guard = std::lock_guard(this->mutex);
                                std::cout << "    Thread " << this->id
                                          << " creating child" << std::endl;
                            }
                            child = std::make_unique<Worker>(
                                value, this->mutex, Receiver<int>(this->tx));
                        }

                        {
                            auto guard = std::lock_guard(this->mutex);
                            std::cout << "    Thread " << this->id
                                      << " sending " << value << " to child"
                                      << std::endl;
                        }
                        this->tx << value;
                    }
                }

                // Stop children
                if (child) {
                    this->tx << value;
                    child->job.join();
                }
                auto guard = std::lock_guard(this->mutex);
                std::cout << "    Thread " << this->id << " stopping"
                          << std::endl;
            });
        }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }

    auto n = std::atoi(argv[1]);
    auto tx = Sender<int>(N);
    auto mutex = std::mutex();

    auto worker = Worker(2, mutex, Receiver<int>(tx));

    for (int i = 3; i < n; i++) {
        {
            auto guard = std::lock_guard(mutex);
            std::cout << "Master before sending " << i << std::endl;
        }

        tx << i;

        auto guard = std::lock_guard(mutex);
        std::cout << "Master after sending " << i << std::endl;
    }

    {
        auto guard = std::lock_guard(mutex);
        std::cout << "Master before sending stop" << std::endl;
    }

    tx << -1;

    {
        auto guard = std::lock_guard(mutex);
        std::cout << "Master after sending stop" << std::endl;
    }

    worker.job.join();

    return 0;
}
