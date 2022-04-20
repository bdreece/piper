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
 *  @file 		channel.hpp
 *  @brief 		Abstract channel interfaces
 *  @author 	Brian Reece
 *  @version 	0.1
 *  @copyright 	MIT License
 *  @date	 	2022-04-19
 */

#include <memory>
#include <utility>
namespace piper {
    template <typename T> class Sender;

    /**
     * @interface Receiver
     * @brief The interface for channel receivers
     * @tparam T The type of the item being received over the channel
     */
    template <typename T> class Receiver {
        public:
            virtual ~Receiver() {}
            /**
             * @brief Receives an item from the channel
             * @return The item received over the channel
             * @note Implementors of this interface may block on this method,
             * 		 and may also throw exceptions.
             */
            virtual T recv() = 0;

            Receiver<T> &operator>>(T &item);
    };

    /**
     * @interface Sender
     * @brief The interface for channel senders
     * @tparam T The type of the item being sent over the channel
     */
    template <typename T> class Sender {
        public:
            virtual ~Sender() {}

            /**
             * @brief Sends an item over the channel
             * @param item The item being sent over the channel
             * @note Implementors of this interface may block on this method,
             * 		 and may also throw exceptions.
             */
            virtual void send(const T &item) = 0;
            virtual void send(T &&item) = 0;

            Sender<T> &operator<<(const T &item);
            Sender<T> &operator<<(T &&item);
    };

    template <typename T> Receiver<T> &Receiver<T>::operator>>(T &item) {
        item = recv();
        return *this;
    }

    template <typename T> Sender<T> &Sender<T>::operator<<(const T &item) {
        send(item);
        return *this;
    }

    template <typename T> Sender<T> &Sender<T>::operator<<(T &&item) {
        send(std::forward<T>(item));
        return *this;
    }

    template <typename T>
    std::basic_ostream<T> &operator<<(std::basic_ostream<T> &os,
                                      Receiver<T> &rx) {
        os.put(rx.recv());
        return os;
    }

    template <typename T>
    std::basic_istream<T> &operator>>(std::basic_istream<T> &is,
                                      Sender<T> &tx) {
        T item = {};
        is.get(item);
        if (item == T{}) {
            is.setstate(std::ios::failbit);
        }
        tx << item;
        return is;
    }

} // namespace piper
