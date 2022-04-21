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
#include <optional>
#include <utility>

/**
 * @namespace 	piper::internal
 * @brief 		Channel inner buffer interface and implementations
 */
namespace piper::internal {
    /**
     * @class	Buffer
     * @brief 	Shared channel buffer base class
     * @tparam 	T The type of item stored in the buffer
     */
    template <typename T> class Buffer {
        protected:
            std::mutex mutex;

        public:
            /**
             * @brief 	Copies and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	Implementors of this virtual method may block.
             */
            virtual void push(const T& item) = 0;

            /**
             * @brief 	Moves and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	Implementors of this virtual method may block.
             */
            virtual void push(T&& item) = 0;

            /**
             * @brief 	Pops an item from the buffer
             * @return 	The item being popped from the buffer
             * @note 	Implementors of this virtual method will block
             * 		 	on an empty buffer
             */
            virtual T pop() = 0;
    };

    /**
     * @class	AsyncBuffer
     * @brief 	An asynchronous, unbounded buffer
     * @tparam 	T The type of item stored in the buffer
     * @extends Buffer
     */
    template <typename T> class AsyncBuffer final : public Buffer<T> {
            std::condition_variable available;
            std::deque<T> queue;

        public:
            /**
             * @brief Constructs an asynchronous buffer
             */
            AsyncBuffer() = default;

            AsyncBuffer(const AsyncBuffer<T>&) = delete;
            AsyncBuffer(AsyncBuffer<T>&&) = delete;

            /**
             * @brief 	Copies and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            void push(const T& item) override;

            /**
             * @brief 	Moves and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            void push(T&& item) override;

            /**
             * @brief 	Pops an item from the buffer
             * @return 	The item being popped from the buffer
             * @note 	Blocks on an empty buffer
             */
            T pop() override;
    };

    /**
     * @class 	SyncBuffer
     * @brief 	A synchronous, bounded buffer
     * @tparam 	T The type of item stored in the buffer
     * @extends Buffer
     */
    template <typename T> class SyncBuffer : public Buffer<T> {
        protected:
            std::size_t n;
            std::deque<T> queue;
            std::condition_variable available[2];

        public:
            /**
             * @brief 	Constructs a synchronous buffer
             * @param 	n The size of the buffer
             * @note 	The size of the buffer should be at least 1.
             * 			If an unbuffered channel is desired, use
             * 			RendezvousBuffer.
             * @warning Passing n = 0 to this constructor may result
             * 			in undefined behavior
             */
            SyncBuffer(std::size_t n) : Buffer<T>(), n(n){};

            SyncBuffer() = delete;
            SyncBuffer(const SyncBuffer<T>&) = delete;
            SyncBuffer(SyncBuffer<T>&&) = delete;

            /**
             * @brief 	Copies and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            virtual void push(const T& item) override;

            /**
             * @brief 	Moves and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            virtual void push(T&& item) override;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Blocks on an empty buffer
             */
            T pop() override;
    };

    /**
     * @class 	RendezvousBuffer
     * @brief 	A synchronous, rendezvous buffer
     * @details A rendezvous buffer has no capacity. Subsequently, calls to
     * 			push will block until another thread has collected the
     * 			value, and vice versa.
     * @tparam 	T The type of item transferred over the buffer
     * @extends Buffer
     */
    template <typename T> class RendezvousBuffer final : public Buffer<T> {
            std::optional<T> item;
            std::condition_variable available[3];

        public:
            /**
             * @brief Constructs a rendezvous buffer
             */
            RendezvousBuffer() : Buffer<T>(){};

            RendezvousBuffer(const RendezvousBuffer<T>&) = delete;
            RendezvousBuffer(RendezvousBuffer<T>&&) = delete;

            /**
             * @brief 	Copies and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            void push(const T& item) override;

            /**
             * @brief 	Moves and pushes an item into the buffer
             * @param 	item The item being pushed into the buffer
             * @note 	This implementation should not block
             */
            void push(T&& item) override;

            /**
             * @brief Pops an item from the buffer
             * @return The item being popped from the buffer
             * @note Blocks awaiting a call to push()
             */
            T pop() override;
    };

    template <typename T> void AsyncBuffer<T>::push(const T& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Push item to queue
            this->queue.push_back(item);
        }

        this->available.notify_one();
    }

    template <typename T> void AsyncBuffer<T>::push(T&& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Push item to queue
            this->queue.push_back(std::forward<T>(item));
        }

        this->available.notify_one();
    }

    template <typename T> T AsyncBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block receiver if queue is empty
            this->available.wait(lock, [this] { return !this->queue.empty(); });

            // Pop item from queue
            item = this->queue.front();
            this->queue.pop_front();
        }
        return item;
    }

    template <typename T> void SyncBuffer<T>::push(const T& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender if queue is full
            this->available[1].wait(lock,
                                    [this] { return this->queue.size() < n; });

            // Push item to queue
            this->queue.push_back(item);
        }
        // Notify a waiting receiver
        this->available[0].notify_one();
    }

    template <typename T> void SyncBuffer<T>::push(T&& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender if queue is full
            this->available[1].wait(lock,
                                    [this] { return this->queue.size() < n; });

            // Push item to queue
            this->queue.push_back(std::forward<T>(item));
        }
        // Notify a waiting receiver
        this->available[0].notify_one();
    }

    template <typename T> T SyncBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

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

    template <typename T> void RendezvousBuffer<T>::push(const T& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender until buffer is ready
            this->available[1].wait(lock, [this] { return !this->item; });

            // Push item to queue
            this->item = item;
        }

        // Notify a waiting receiver that buffer is filled
        this->available[0].notify_one();

        {
            // Reacquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender until item has been received
            this->available[2].wait(lock, [this] { return !this->item; });
        }
    }

    template <typename T> void RendezvousBuffer<T>::push(T&& item) {
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender until buffer is ready
            this->available[1].wait(lock, [this] { return !this->item; });

            // Push item to queue
            this->item = std::forward<T>(item);
        }

        // Notify a waiting receiver that buffer is filled
        this->available[0].notify_one();

        {
            // Reacquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block sender until item has been received
            this->available[2].wait(lock, [this] { return !this->item; });
        }
    }

    template <typename T> T RendezvousBuffer<T>::pop() {
        T item;
        {
            // Acquire lock
            auto lock = std::unique_lock(this->mutex);

            // Block receiver until buffer is filled
            this->available[0].wait(lock, [this] { return this->item; });

            // Pop item from queue
            item = *this->item;
            this->item.reset();
        }

        // Notify sender that item is received
        this->available[2].notify_one();

        // Notify a waiting sender
        this->available[1].notify_one();
        return item;
    }
} // namespace piper::internal
