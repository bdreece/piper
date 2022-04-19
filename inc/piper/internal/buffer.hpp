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
 * @internal
 * @file 		buffer.hpp
 * @brief		Shared channel buffer base class and derivations
 * @author		Brian Reece
 * @version 	0.1
 * @copyright 	MIT License
 * @date		2022-04-19
 */

#include <condition_variable>
#include <deque>
#include <mutex>

namespace piper::internal {
    /**
     * @class Buffer
     * @brief Shared channel buffer base class
     * @tparam T The type of item stored in the buffer
     */
    template <typename T> class Buffer {
        protected:
            std::mutex mutex;
            std::deque<T> queue;

        public:
            /**
             * @brief Copies and pushes an item into the buffer
             * @param The item being pushed into the buffer
             * @note Implementors of this virtual method may block.
             */
            virtual void push(const T &) = 0;

            /**
             * @brief Moves and pushes a temporary item into the buffer
             * @param The item being pushed into the buffer
             * @note Implementors of this virtual method may block.
             */
            virtual void push(T &&) = 0;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Implementors of this virtual method will block
             * 		 on an empty buffer
             */
            virtual T pop() = 0;
    };

    /**
     * @class AsyncBuffer
     * @brief An asynchronous, unbounded buffer
     * @tparam T The type of item stored in the buffer
     * @extends Buffer<T>
     */
    template <typename T> class AsyncBuffer final : Buffer<T> {
            std::condition_variable available;

        public:
            /**
             * @brief Creates an unbounded buffer
             */
            AsyncBuffer() = default;
            AsyncBuffer(const AsyncBuffer<T> &) = delete;
            AsyncBuffer(AsyncBuffer<T> &&) = delete;

            /**
             * @brief Copies and pushes an item into the buffer
             * @param The item being pushed into the buffer
             * @note This method should not block
             */
            void push(const T &) override;

            /**
             * @brief Moves and pushes a temporary item into the buffer
             * @param The item being pushed into the buffer
             * @note This method should not block
             */
            void push(T &&) override;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Blocks on an empty buffer
             */
            T pop() override;
    };

    /**
     * @class SyncBuffer
     * @brief A synchronous, bounded buffer
     * @tparam T The type of item stored in the buffer
     * @extends Buffer<T>
     */
    template <typename T> class SyncBuffer : Buffer<T> {
        protected:
            std::size_t n;
            std::condition_variable available[2];

        public:
            /**
             * @brief Creates a bounded buffer
             * @param n The size of the buffer
             * @note The size of the buffer should be at least 1. A
             * 		 synchronous buffer of size 0 is implemented as
             * 		 RendezvousBuffer<T>
             */
            SyncBuffer(std::size_t n) : Buffer<T>(), n(n){};
            SyncBuffer() = delete;
            SyncBuffer(const SyncBuffer<T> &) = delete;
            SyncBuffer(SyncBuffer<T> &&) = delete;

            /**
             * @brief Copies and pushes an item into the buffer
             * @param The item being pushed into the buffer
             * @note Blocks on a full buffer
             */
            virtual void push(const T &) override;

            /**
             * @brief Moves and pushes a temporary item into the buffer
             * @param The item being pushed into the buffer
             * @note Blocks on a full buffer
             */
            virtual void push(T &&) override;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Blocks on an empty buffer
             */
            T pop() override;
    };

    /**
     * @class RendezvousBuffer
     * @brief A synchronous, rendezvous buffer
     * @details A rendezvous buffer has no capacity. Subsequently, calls to
     * 			push will block until another thread calls pop, and vice
     * versa.
     * @tparam T The type of item transferred over the buffer
     * @extends SyncBuffer<T>
     */
    template <typename T> class RendezvousBuffer final : SyncBuffer<T> {
            bool ready;

        public:
            /**
             * @brief Creates a rendezvous buffer
             */
            RendezvousBuffer() : SyncBuffer<T>(0), ready(true){};
            RendezvousBuffer(const RendezvousBuffer<T> &) = delete;
            RendezvousBuffer(RendezvousBuffer<T> &&) = delete;

            /**
             * @brief Copies and pushes an item into the buffer
             * @param The item being pushed into the buffer
             * @note Blocks awaiting a call to pop()
             */
            void push(const T &) override;

            /**
             * @brief Moves and pushes an item into the buffer
             * @param The item being pushed into the buffer
             * @note Blocks awaiting a call to pop()
             */
            void push(T &&) override;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Blocks awaiting a call to push()
             */
            T pop() override;
    };

    template <typename T> void AsyncBuffer<T>::push(const T &item) {
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            // Push item to queue
            this->queue.push_back(item);
        }

        this->available.notify_one();
    }

    template <typename T> void AsyncBuffer<T>::push(T &&item) { push(item); }

    template <typename T> T AsyncBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            // Block receiver if queue is empty
            this->available.wait(lock, [this] { return !this->queue.empty(); });

            // Pop item from queue
            item = this->queue.front();
            this->queue.pop_front();
        }
        return item;
    }

    template <typename T> void SyncBuffer<T>::push(const T &item) {
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            // Block sender if queue is full
            this->available[1].wait(lock,
                                    [this] { return this->queue.size() < n; });

            // Push item to queue
            this->queue.push_back(item);
        }
        // Notify a waiting receiver
        this->available[0].notify_one();
    }

    template <typename T> void SyncBuffer<T>::push(T &&item) { push(item); }

    template <typename T> T SyncBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            // Block receiver if queue is empty
            this->available[0].wait(lock,
                                    [this] { return !this->queue.empty(); });

            // Pop item from queue
            item = this->queue.front();
            this->queue.pop_front();
        }
        // Notify a waiting sender
        this->available[1].notify_one();

        return item;
    }

    template <typename T> void RendezvousBuffer<T>::push(const T &item) {
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            // Block sender until ready with receivers
            this->available[1].wait(lock,
                                    [this] { return ready && this->n > 0; });

            // No longer ready for input
            ready = false;

            // Push item to queue
            this->queue.push_back(item);
        }
        // Notify a waiting receiver
        this->available[0].notify_one();
    }

    template <typename T> void RendezvousBuffer<T>::push(T &&item) {
        push(item);
    }

    template <typename T> T RendezvousBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(this->mutex);

            this->n++;
            // Block until sender ready
            this->available[0].wait(lock, [this] { return !ready; });

            // Pop item from queue
            item = this->queue.front();
            this->queue.pop_front();

            // Signal to sender that queue is empty
            this->n--;
            ready = true;
        }

        // Notify a waiting sender
        this->available[1].notify_one();
        return item;
    }
} // namespace piper::internal