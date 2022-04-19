#include <algorithm>
#include <array>
#include <boost/test/unit_test_suite.hpp>
#include <iterator>
#include <thread>

#define BOOST_TEST_MODULE mpsc
#include <boost/test/included/unit_test.hpp>

#include "piper/mpsc.hpp"

using namespace piper::mpsc;

struct async_fixture {
        Sender<int> tx;
        Receiver<int> rx;

        async_fixture() : rx{}, tx(rx) {}
};

BOOST_AUTO_TEST_SUITE(async)

BOOST_FIXTURE_TEST_CASE(one_sender, async_fixture) {
    std::thread worker([this]() { tx.send(1); });
    int value = rx.recv();
    worker.join();
    BOOST_TEST(value == 1);
}

BOOST_AUTO_TEST_SUITE_END()
