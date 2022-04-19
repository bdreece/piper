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
#include <stdexcept>
#include <thread>
#include <tuple>

#define BOOST_TEST_MODULE mpsc
#include <boost/test/unit_test.hpp>

#include "piper/mpsc.hpp"

using namespace piper::mpsc;

struct async_fixture {
        Sender<int> tx;
        Receiver<int> rx;

        async_fixture() : rx{}, tx(rx) {}
};

BOOST_AUTO_TEST_SUITE(exceptions)

/**
 * @test 	receiver_expired
 * @brief 	Asserts that piper::mpsc::Sender<T>::send(T &&) throws exception
 * 		  	after piper::mpsc::Receiver<T> is destructed.
 */
BOOST_FIXTURE_TEST_CASE(receiver_expired, async_fixture) {
    { piper::mpsc::Receiver<int> _rx(std::move(rx)); }
    try {
        tx.send(1);
    } catch (const std::runtime_error &e) {
        BOOST_TEST(e.what() == "receiver is expired");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(async)

/**
 * @test one_sender
 * @brief Asserts that one sender can send a value
 * 		  over the channel to the receiver.
 */
BOOST_FIXTURE_TEST_CASE(one_sender, async_fixture) {
    std::thread worker([this]() { tx.send(1); });
    int value = rx.recv();
    worker.join();
    BOOST_TEST(value == 1);
}

BOOST_AUTO_TEST_SUITE_END()
