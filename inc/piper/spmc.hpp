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
 * @file 		spmc.hpp
 * @brief 		Single producer, multiple consumer channel
 * @author 		Brian Reece
 * @version 	0.1
 * @copyright 	MIT License
 * @date 		2022-04-19
 */

#include <memory>
#include <stdexcept>

#include "piper/channel.hpp"
#include "piper/internal/buffer.hpp"

namespace piper::spmc {
    template <typename T> class Sender;

    /**
     * @class Receiver
     * @brief SPMC receiver
     * @tparam T The item being received over the channel
     * @extends piper::Receiver<T>
     */
    template <typename T> class Receiver : public piper::Receiver<T> {
            std::weak_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates a Receiver from a Sender
             * @param tx The connected Sender
             */
            Receiver(const Sender<T>& tx) : buffer{tx.buffer} {}
            Receiver(const Receiver<T>&) = default;
            Receiver(Receiver<T>&&) = default;
            Receiver() = delete;

            /**
             * @brief 	Receive an item over the channel
             * @return 	The item received over the channel
             * @throws 	std::runtime_error Thrown if the sender
             * 			no longer exists.
             * @note 	Blocks on empty buffer
             */
            T recv() noexcept(false) override;
    };

    /**
     * @class 	Sender
     * @brief 	SPMC sender
     * @tparam 	T The item being sent over the channel
     * @extends	piper::Sender<T>
     */
    template <typename T> class Sender : public piper::Sender<T> {
            friend class Receiver<T>;

            std::shared_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Creates an asynchronous Sender
             */
            Sender();

            /**
             * @brief 	Creates a synchronous Sender
             * @param 	The size of the buffer
             * @note 	A size of zero represents a rendezvous channel
             */
            Sender(std::size_t n);
            Sender(Sender<T>&&) = default;
            Sender(const Sender<T>&) = delete;

            /**
             * @brief Sends an item over the channel
             * @param item The item being sent over the channel
             * @note  May block if using a synchronous buffer
             */
            void send(const T& item) override;
            void send(T&& item) override;
    };

    /**
     * @brief Creates an asynchronous channel
     * @return A sender and a receiver
     */
    template <typename T> std::tuple<Sender<T>, Receiver<T>> channel() {
        auto tx = Sender<T>();
        auto rx = Receiver<T>(tx);
        return std::make_tuple(tx, rx);
    }

    /**
     * @brief Creates a synchronous channel
     * @param n The size of the buffer
     * @return A sender and a receiver
     * @note A size of zero represents a rendezvous channel
     */
    template <typename T>
    std::tuple<Sender<T>, Receiver<T>> channel(std::size_t n) {
        auto tx = Sender<T>(n);
        auto rx = Receiver<T>(tx);
        return std::make_tuple(tx, rx);
    }

    template <typename T> T Receiver<T>::recv() {
        if (buffer.expired())
            throw std::runtime_error("sender is expired");
        return buffer.lock()->pop();
    }

    template <typename T> Sender<T>::Sender() {
        using namespace piper::internal;
        buffer.reset(new AsyncBuffer<T>{});
    }

    template <typename T> Sender<T>::Sender(std::size_t n) {
        using namespace piper::internal;
        if (n > 0) {
            buffer.reset(new SyncBuffer<T>{n});
        } else {
            buffer.reset(new RendezvousBuffer<T>{});
        }
    }

    template <typename T> void Sender<T>::send(const T& item) {
        buffer->push(item);
    }

    template <typename T> void Sender<T>::send(T&& item) {
        buffer->push(std::forward<T>(item));
    }
} // namespace piper::spmc
