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
 * @file		mpsc.cpp
 * @brief		MPSC testing suite
 * @author		Brian Reece
 * @version		0.1
 * @copyright	MIT License
 * @date		2022-04-19
 */

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>

#define BOOST_TEST_MODULE mpsc
#include <boost/test/unit_test.hpp>

#include "piper/mpsc.hpp"
using namespace piper::mpsc;

BOOST_AUTO_TEST_SUITE(mpsc_exceptions)

/**
 * @test 	mpsc_exceptions/expired
 * @brief 	Asserts that tx.send() throws exception
 * 		  	after rx is destroyed.
 */
BOOST_AUTO_TEST_CASE(expired) {
    using namespace piper::mpsc;
    auto rx = new Receiver<int>{};
    auto tx = Sender<int>{*rx};
    delete rx;
    try {
        tx.send(1);
    } catch (const std::runtime_error& e) {
        BOOST_TEST(e.what() == "receiver is expired");
    }
}

BOOST_AUTO_TEST_SUITE_END() // mpsc_exceptions

BOOST_AUTO_TEST_SUITE(mpsc_async)

struct fixture {
        std::unique_ptr<piper::mpsc::Receiver<int>> rx;

        fixture() {
            using namespace piper::mpsc;
            rx = std::make_unique<Receiver<int>>();
        }
};

/**
 * @test one_sender
 * @brief Asserts that one sender can send one receiver
 * 		  five integers over the channel to the receiver.
 */
BOOST_FIXTURE_TEST_CASE(one_sender, fixture) {
    using namespace piper::mpsc;
    std::thread worker(
        [](auto&& tx) {
            for (int i = 0; i < 5; i++) {
                tx << i;
            }
        },
        std::move(Sender<int>{*rx}));
    for (int i = 0; i < 5; i++) {
        BOOST_TEST(rx->recv() == 1);
    }
    worker.join();
}

/**
 * @test mpsc_async/five_senders
 * @brief Asserts that five senders can send one receiver
 * 		  five integers over the channel.
 */
BOOST_FIXTURE_TEST_CASE(five_senders, fixture) {
    using namespace piper::mpsc;
    std::vector<std::thread> workers{5};
    std::generate(workers.begin(), workers.end(), [this]() {
        return std::thread([](auto tx) { tx << 1; }, Sender<int>{*rx});
    });

    for (int i = 0; i < 5; i++) {
        BOOST_TEST(rx->recv() == 1);
    }

    std::for_each(workers.begin(), workers.end(), [](auto& tx) { tx.join(); });
}

BOOST_AUTO_TEST_SUITE_END() // mpsc_async
