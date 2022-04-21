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
 *  @file 		piper.hpp
 *  @brief 		Abstract channel interfaces
 *  @author 	Brian Reece
 *  @version 	0.1
 *  @copyright 	MIT License
 *  @date	 	2022-04-19
 */

#include <memory>
#include <tuple>
#include <utility>
namespace piper {
    template <typename T> class Sender;
    /**
     * @interface	Receiver
     * @brief 		The interface for channel receivers
     * @tparam 		T The type of the item being received over the channel
     */
    template <typename T> class Receiver {
        public:
            /**
             * @brief	Destructs a Receiver
             */
            virtual ~Receiver() {}

            /**
             * @brief 	Receives an item from the channel
             * @return 	The item received over the channel
             * @note 	Implementors of this interface may block on this method,
             * 		 	and may also throw exceptions.
             */
            virtual T recv() = 0;

            /**
             * @brief	Copies item from channel
             * @param 	item The output of the Receiver
             * @return 	The receiver, for chained extractions
             * @note 	This method assigns item to the return of recv()
             */
            Receiver<T>& operator>>(T& item);

            /**
             * @brief 	Moves received item into Sender
             * @param 	tx The sender into which the item is moved
             * @return 	The receiver, for chained extractions
             * @note 	This method forwards the return of recv()
             * 		 	to Sender::send()
             * @warning Passing the connected sender to this
             * 			method with a rendezvous channel will
             * 			cause deadlock.
             */
            Receiver<T>& operator>>(Sender<T>& tx);
    };

    /**
     * @interface 	Sender
     * @brief 		The interface for channel senders
     * @tparam 		T The type of the item being sent over the channel
     */
    template <typename T> class Sender {
        public:
            /**
             * @brief 	Destructs a Sender
             */
            virtual ~Sender() {}

            /**
             * @brief 	Copies and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note 	Implementors of this interface may block on this method,
             * 		 	and may also throw exceptions.
             */
            virtual void send(const T& item) = 0;

            /**
             * @brief 	Moves and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @note 	Implementors of this interface may block on this method,
             * 		 	and may also throw exceptions.
             */
            virtual void send(T&& item) = 0;

            /**
             * @brief	Copies and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @return 	The sender, for chained insertions
             * @note 	This method forwards item to send()
             */
            Sender<T>& operator<<(const T& item);

            /**
             * @brief 	Moves and sends an item over the channel
             * @param 	item The item being sent over the channel
             * @return 	The sender, for chained insertions
             * @note 	This method forwards item to send()
             */
            Sender<T>& operator<<(T&& item);

            /**
             * @brief 	Moves received item into Sender
             * @param 	rx The receiver from which item is moved
             * @return 	The sender, for chained insertions
             * @note 	This method forwards Receiver::recv() to
             * 		 	send()
             * @warning Passing the connected receiver to this
             * 			method with a rendezvous channel will
             * 			cause deadlock.
             */
            Sender<T>& operator<<(Receiver<T>& rx);
    };

    /**
     * @interface	Channel
     * @brief 		A bidirectional channel interface
     * @tparam 		T The type of item being exchanged over the channel
     * @implements	Sender, Receiver
     */
    template <typename T> class Channel : public Sender<T>, public Receiver<T> {
        public:
            virtual ~Channel() {}
    };

    template <typename T> Receiver<T>& Receiver<T>::operator>>(T& item) {
        item = recv();
        return *this;
    }

    template <typename T> Receiver<T>& Receiver<T>::operator>>(Sender<T>& tx) {
        tx.send(std::forward<T>(recv()));
        return *this;
    }

    template <typename T> Sender<T>& Sender<T>::operator<<(const T& item) {
        send(item);
        return *this;
    }

    template <typename T> Sender<T>& Sender<T>::operator<<(T&& item) {
        send(std::forward<T>(item));
        return *this;
    }

    template <typename T> Sender<T>& Sender<T>::operator<<(Receiver<T>& rx) {
        send(std::forward<T>(rx.recv()));
    }

} // namespace piper
