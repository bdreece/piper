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
#include <utility>

#include "piper/channel.hpp"
#include "piper/internal/buffer.hpp"

namespace piper::mpsc {
    template <typename T> class Sender;
    template <typename T> class Channel;

    /**
     * @class Receiver
     * @brief MPSC receiver
     * @tparam T The item being received over the channel
     * @implements piper::Receiver<T>
     */
    template <typename T> class Receiver : public piper::Receiver<T> {
            friend class Sender<T>;

            std::shared_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates an asynchronous receiver
             */
            Receiver();

            /**
             * @brief Creates a synchronous receiver
             * @param n The size of the buffer
             * @note A size of zero represents a rendezvous channel
             */
            Receiver(std::size_t n);
            Receiver(Receiver<T>&&) = default;
            Receiver(Channel<T>&& ch)
                : Receiver(std::forward<Receiver<T>>(ch.rx)) {}
            Receiver(const Receiver<T>&) = delete;

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
     * @implements piper::Sender<T>
     */
    template <typename T> class Sender : public piper::Sender<T> {
            std::weak_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates a new Sender from a Receiver
             * @param rx The connected receiver
             */
            Sender(const Receiver<T>& rx) : buffer(rx.buffer) {}

            /**
             * @brief Creates a new Sender from a Channel
             * @param ch The channel
             */
            Sender(const Channel<T>& ch) : Sender(ch.rx) {}

            Sender(Sender<T>&&) = default;
            Sender() = delete;

            /**
             * @brief Sends an item over the channel
             * @param item The item to send over the channel
             * @throws std::runtime_error Thrown if the receiver
             * 		   no longer exists.
             * @note Blocks if using a synchronous buffer
             */
            void send(const T& item) noexcept(false) override;
            void send(T&& item) noexcept(false) override;
    };

    template <typename T> class Channel final : public piper::Channel<T> {
            friend class Sender<T>;
            friend class Receiver<T>;
            std::unique_ptr<Sender<T>> tx;
            std::unique_ptr<Receiver<T>> rx;

        public:
            Channel() : rx{new Receiver<T>()}, tx(*this->rx){};
            Channel(std::size_t n) : rx{new Receiver<T>(n)}, tx(*this->rx) {}

            Channel(Channel<T>&&) = default;

            T recv() override;

            void send(const T& item) override;
            void send(T&& item) override;
    };

    template <typename T> Receiver<T>::Receiver() {
        using namespace piper::internal;
        buffer.reset(new AsyncBuffer<T>());
    }

    template <typename T> Receiver<T>::Receiver(std::size_t n) {
        using namespace piper::internal;
        if (n > 0) {
            buffer.reset(new SyncBuffer<T>(n));
        } else {
            buffer.reset(new RendezvousBuffer<T>());
        }
    }

    template <typename T> T Receiver<T>::recv() { return buffer->pop(); }

    template <typename T> void Sender<T>::send(const T& item) {
        if (buffer.expired())
            throw std::runtime_error("receiver is expired");
        buffer.lock()->push(item);
    }

    template <typename T> void Sender<T>::send(T&& item) {
        if (buffer.expired())
            throw std::runtime_error("receiver is expired");
        buffer.lock()->push(std::forward<T>(item));
    }

    template <typename T> T Channel<T>::recv() { return rx->recv(); }

    template <typename T> void Channel<T>::send(const T& item) {
        tx->send(item);
    }

    template <typename T> void Channel<T>::send(T&& item) {
        tx->send(std::forward<T>(item));
    }

} // namespace piper::mpsc
