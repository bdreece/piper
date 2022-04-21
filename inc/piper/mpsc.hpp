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

#include <stdexcept>

#include "piper/internal/buffer.hpp"
#include "piper/piper.hpp"

/**
 * @namespace 	piper::mpsc
 * @brief 		Concrete Channel, Sender, and Receiver implementations
 * 				for multiple producer, single consumer channels
 */
namespace piper::mpsc {
    template <typename T> class Sender;
    template <typename T> class Channel;

    /**
     * @class Receiver
     * @brief MPSC channel receiver
     * @tparam T The item being received over the channel
     * @implements piper::Receiver
     */
    template <typename T> class Receiver : public piper::Receiver<T> {
            friend class Sender<T>;

            /**
             * @brief The shared channel buffer
             * @note  The buffer will be destructed with the receiver
             */
            std::shared_ptr<piper::internal::Buffer<T>> buffer;

        public:
            ///	Constructs an asynchronous Receiver
            Receiver();

            /**
             * @brief 	Constructs a synchronous Receiver
             * @param 	n The size of the buffer
             * @note 	A size of zero represents a rendezvous channel
             */
            Receiver(std::size_t n);

            /**
             * @brief 	Moves a Receiver
             * @param 	rx The Receiver to move
             */
            Receiver(Receiver<T>&& rx) = default;

            /**
             * @brief 	Moves a Receiver from a Channel
             * @param 	ch The Channel from which Receiver is moved
             */
            Receiver(Channel<T>&& ch)
                : Receiver(std::forward<Receiver<T>>(ch.rx)) {}

            Receiver(const Receiver<T>&) = delete;

            /**
             * @brief 	Receives an item from the channel
             * @return 	The item received from the channel
             * @note 	Blocks on an empty buffer
             */
            T recv() override;
    };

    /**
     * @class 		Sender
     * @brief 		MPSC channel sender
     * @tparam 		T The item being sent over the channel
     * @implements 	piper::Sender
     */
    template <typename T> class Sender : public piper::Sender<T> {

            /**
             * @brief The shared channel buffer
             * @note  The buffer will not be destructed with
             * 		  the sender
             */
            std::weak_ptr<piper::internal::Buffer<T>> buffer;

        public:
            /**
             * @brief 	Copies a Sender from a Receiver
             * @param 	rx The Receiver from which Sender is copied
             */
            Sender(const Receiver<T>& rx) : buffer(rx.buffer) {}

            /**
             * @brief 	Copies a Sender from a Channel
             * @param 	ch The Channel from which Sender is copied
             */
            Sender(const Channel<T>& ch) : Sender(ch.rx) {}

            /**
             * @brief	Moves a Sender
             * @param	tx The Sender to move
             */
            Sender(Sender<T>&& tx) = default;

            Sender() = delete;

            /**
             * @brief 	Copies and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(const T& item) noexcept(false) override;

            /**
             * @brief 	Moves and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note  	May block if using a synchronous buffer
             */
            void send(T&& item) noexcept(false) override;
    };

    /**
     * @class 		Channel
     * @brief 		A multiple producer, single consumer channel
     * @tparam 		T The item being exchanged over the channel
     * @implements 	piper::Channel
     */
    template <typename T> class Channel : public piper::Channel<T> {
            friend class Sender<T>;
            friend class Receiver<T>;

            /// The Sender component
            std::unique_ptr<Sender<T>> tx;

            /// The Receiver component
            std::unique_ptr<Receiver<T>> rx;

        public:
            /// Constructs an asynchronous Channel
            Channel() : rx{new Receiver<T>()}, tx(*this->rx){};

            /**
             * @brief 	Constructs a synchronous Channel
             * @param	n The size of the buffer
             * @note	A size of 0 represents a rendezvous buffer
             */
            Channel(std::size_t n) : rx{new Receiver<T>(n)}, tx(*this->rx) {}

            /**
             * @brief	Moves a Channel
             * @param 	ch The Channel to move
             */
            Channel(Channel<T>&& ch) = default;

            /**
             * @brief 	Receives an item from the channel
             * @return 	The item received from the channel
             * @note 	Blocks on an empty buffer
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
