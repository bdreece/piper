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
 * @file 		mpsc.hpp
 * @brief 		Multiple producer, single consumer channel
 * @author 		Brian Reece
 * @version 	0.1
 * @copyright 	MIT License
 * @date 		2022-04-18
 */

#include <memory>
#include <stdexcept>
#include <tuple>

#include "piper/channel.hpp"
#include "piper/internal/buffer.hpp"

namespace piper::mpsc {
    template <typename T> class Sender;

    /**
     * @class Receiver
     * @brief MPSC receiver
     * @tparam T The item being received over the channel
     * @extends piper::internal::Receiver<T>
     */
    template <typename T> class Receiver : piper::Receiver<T> {
            friend class Sender<T>;

            std::shared_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates an asynchronous receiver
             */
            Receiver();

            /**
             * @brief Creates a synchronous receiver
             * @param The size of the buffer
             * @note A size of zero represents a rendezvous channel
             */
            Receiver(std::size_t);
            Receiver(Receiver<T> &&) = default;
            Receiver(const Receiver<T> &) = delete;

            /**
             * @brief Receives an item from the channel
             * @return The item received from the channel
             * @note Blocks on an empty buffer
             */
            T recv() override;
    };

    /**
     * @class Sender
     * @brief MPSC sender
     * @tparam T The item being sent over the channel
     * @extends piper::internal::Sender<T>
     */
    template <typename T> class Sender : piper::Sender<T> {
            std::weak_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates a new Sender from a Receiver
             * @param The connected receiver
             */
            Sender(const Receiver<T> &);
            /**
             * @brief Clones a sender
             * @param The sender to clone
             */
            Sender(const Sender<T> &) = default;
            Sender(Sender<T> &&) = default;
            Sender() = delete;

            /**
             * @brief Copies and sends an item over the channel
             * @param The item to send over the channel
             * @throws std::runtime_error("receiver is expired")
             * 		   Thrown if the receiver no longer exists.
             * @note Blocks if using a synchronous buffer
             */
            void send(const T &) noexcept(false) override;

            /**
             * @brief Moves and sends an item over the channel
             * @param The item to send over the channel
             * @throws std::runtime_error("receiver is expired")
             * 		   Thrown if the receiver no longer exists.
             * @note Blocks if using a synchronous buffer
             */
            void send(T &&) noexcept(false) override;
    };

    /**
     * @brief Creates an asynchronous channel
     * @return A sender and a receiver
     */
    template <typename T>
    std::tuple<piper::Sender<T>, piper::Receiver<T>> channel() {
        Receiver<T> rx;
        Sender<T> tx(rx);
        return std::make_tuple(tx, rx);
    }

    /**
     * @brief Creates a synchronous channel
     * @param The size of the buffer
     * @return A sender and a receiver
     * @note A size of 0 represents a rendezvous channel
     */
    template <typename T>
    std::tuple<piper::Sender<T>, piper::Receiver<T>> channel(std::size_t n) {
        Receiver<T> rx(n);
        Sender<T> tx(rx);
        return std::make_tuple(tx, rx);
    }

    template <typename T> Receiver<T>::Receiver() {
        buffer.reset(new piper::internal::AsyncBuffer<T>());
    }

    template <typename T> Receiver<T>::Receiver(std::size_t n) {
        n > 0 ? buffer.reset(new piper::internal::SyncBuffer<T>(n))
              : buffer.reset(new piper::internal::RendezvousBuffer<T>());
    }

    template <typename T> T Receiver<T>::recv() { return buffer->pop(); }

    template <typename T>
    Sender<T>::Sender(const Receiver<T> &other) : buffer(other.buffer) {}

    template <typename T> void Sender<T>::send(const T &item) {
        if (buffer.expired())
            throw std::runtime_error("receiver is expired");
        buffer.lock()->push(item);
    }
} // namespace piper::mpsc
