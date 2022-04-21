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
 * implementation
 * @author 		Brian Reece
 * @version 	0.1
 * @copyright 	MIT License
 * @date 		2022-04-19
 */

#include <memory>
#include <stdexcept>

#include "piper/internal/buffer.hpp"
#include "piper/piper.hpp"

/**
 * @namespace 	piper::spmc
 * @brief 		Concrete Channel, Sender, and Receiver implementations
 * 		  		for single producer, multiple consumer channels.
 */
namespace piper::spmc {
    template <typename T> class Sender;
    template <typename T> class Channel;

    /**
     * @class 		Receiver
     * @brief 		SPMC channel receiver
     * @tparam 		T The type of item being received over the channel
     * @implements	piper::Receiver
     */
    template <typename T> class Receiver : public piper::Receiver<T> {
            std::weak_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Copies a Receiver from a Sender
             * @param tx The Sender from which Receiver is copied
             */
            Receiver(const Sender<T>& tx) : buffer{tx.buffer} {}

            /**
             * @brief Copies a Receiver from a Channel
             * @param ch The Channel from which Receiver is copied
             */
            Receiver(const Channel<T>& ch) : Receiver(ch.tx) {}

            /**
             * @brief Copies a Receiver
             * @param rx The Receiver to copy
             */
            Receiver(const Receiver<T>& rx) = default;

            /**
             * @brief Moves a Receiver
             * @param rx The Receiver to move
             */
            Receiver(Receiver<T>&& rx) = default;

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
     * @class 		Sender
     * @brief 		SPMC channel sender
     * @tparam 		T The type of item being sent over the channel
     * @implements	piper::Sender
     */
    template <typename T> class Sender : public piper::Sender<T> {
            friend class Receiver<T>;

            std::shared_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief Constructs an asynchronous Sender
             */
            Sender();

            /**
             * @brief 	Constructs a synchronous Sender
             * @param 	n The size of the buffer
             * @note 	A size of zero represents a rendezvous channel
             */
            Sender(std::size_t n);

            /**
             * @brief	Moves a Sender
             * @param 	tx The Sender to move
             */
            Sender(Sender<T>&& tx) = default;

            /**
             * @brief 	Moves a Sender from a Channel
             * @param   ch The Channel from which Sender is moved
             */
            Sender(Channel<T>&& ch) : Sender(std::forward<Sender<T>>(ch.tx)) {}

            Sender(const Sender<T>&) = delete;

            /**
             * @brief 	Copies and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(const T& item) override;

            /**
             * @brief 	Moves and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(T&& item) override;
    };

    /**
     * @class Channel
     * @brief A single producer, multiple consumer channel
     * @tparam T The type of item being exchanged over the channel
     * @implements piper::Channel
     */
    template <typename T> class Channel final : public piper::Channel<T> {
            friend class Sender<T>;
            friend class Receiver<T>;
            std::unique_ptr<Sender<T>> tx;
            std::unique_ptr<Receiver<T>> rx;

        public:
            /**
             * @brief	Constructs an asynchronous Channel
             */
            Channel() : tx(), rx(this->tx) {}
            /**
             * @brief 	Constructs a synchronous Channel
             * @param 	n The size of the buffer
             * @note	A size of 0 represents a rendezvous channel
             */
            Channel(std::size_t n) : tx(n), rx(this->tx) {}
            /**
             * @brief	Moves a Channel
             * @param 	ch The Channel to move
             */
            Channel(Channel<T>&& ch) = default;

            Channel(const Channel<T>&) = delete;

            /**
             * @brief 	Receive an item over the channel
             * @return 	The item received over the channel
             * @throws 	std::runtime_error Thrown if the sender
             * 			no longer exists.
             * @note 	Blocks on empty buffer
             */
            T recv() override;

            /**
             * @brief 	Copies and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(const T& item) override;

            /**
             * @brief 	Moves and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(T&& item) override;
    };

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

    template <typename T> T Channel<T>::recv() { return rx->recv(); }

    template <typename T> void Channel<T>::send(const T& item) {
        tx->send(item);
    }

    template <typename T> void Channel<T>::send(T&& item) {
        tx->send(std::forward<T>(item));
    }
} // namespace piper::spmc
