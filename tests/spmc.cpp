/**
 * MIT License

 * Copyright (c) 2022 Brian Reece

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file		spmc.cpp
 * @brief		SPMC testing suite
 * @author		Brian Reece
 * @version		0.1
 * @copyright	MIT License
 * @date		2022-04-19
 */

#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <vector>

#define BOOST_TEST_MODULE spmc
#include <boost/test/unit_test.hpp>

#include "piper/spmc.hpp"
using namespace piper::spmc;

BOOST_AUTO_TEST_SUITE(spmc_exceptions)

/**
 * @test spmc_exceptions/expired
 * @brief Asserts that exception is throw if rx.recv()
 * 		  is called after tx is destroyed.
 */
BOOST_AUTO_TEST_CASE(expired) {
    auto tx = new Sender<int>{};
    auto rx = Receiver<int>{*tx};
    delete tx;
    try {
        int val = rx.recv();
    } catch (const std::runtime_error& e) {
        BOOST_TEST(e.what() == "sender is expired");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(spmc_async)

struct fixture {
        std::unique_ptr<Sender<int>> tx;

        fixture() { tx = std::make_unique<Sender<int>>(); }
};

/**
 * @test spmc_async/one_receiver
 * @brief Asserts that one sender can send one receiver
 * 		  five integers.
 */
BOOST_FIXTURE_TEST_CASE(one_receiver, fixture) {
    std::thread worker(
        [](auto&& rx) {
            for (int i = 0; i < 5; i++) {
                int j;
                BOOST_TEST(rx.recv() == i);
            }
        },
        std::move(Receiver<int>{*tx}));

    for (int i = 0; i < 5; i++) {
        *tx << i;
    }
}

/**
 * @test spmc_async/five_receivers
 * @brief Asserts that one sender can send five receivers
 * 		  ten integers.
 */
BOOST_FIXTURE_TEST_CASE(five_receivers, fixture) {
    std::vector<std::thread> workers(5);
    std::generate(workers.begin(), workers.end(), [this]() {
        return std::thread(
            [](auto rx) {
                int n = 0;
                while (n < 2) {
                    auto i = rx.recv();
                    n++;
                }
                BOOST_TEST(true);
            },
            Receiver<int>(*tx));
    });

    for (int i = 0; i < 10; i++) {
        *tx << i;
    }
}

BOOST_AUTO_TEST_SUITE_END() // spmc_async

BOOST_AUTO_TEST_SUITE(spmc_sync)

struct fixture {
        std::unique_ptr<Sender<int>> tx;

        fixture() { tx = std::make_unique<Sender<int>>(); }
};

BOOST_AUTO_TEST_SUITE_END() // synch
