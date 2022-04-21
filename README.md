# piper

![GitHub](https://img.shields.io/github/license/bdreece/piper)
![GitHub Workflow Status](https://img.shields.io/github/workflow/status/bdreece/piper/CMake)
![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/bdreece/piper?include_prereleases)

A template library for concurrent channels in C++

### Table of Contents

* [Overview](#overview)
	* [Sender](#sender)
    * [Receiver](#receiver)
    * [Channel](#channel)
    * [Buffers](#flavors)
    	* [Asynchronous](#asynchronous)
        * [Synchronous](#synchronous)
        * [Rendezvous](#rendezvous)
* [API Reference](https://bdreece.github.io/piper/)
* [Future Plans](#future-plans)

### Overview

piper is a C++20 template library for concurrent channels, using STL concurrency primitives such as `std::mutex` and `std::condition_variable`.

#### Sender

A `piper::Sender` is an abstract template class used to send data through a channel buffer. A `piper::mpsc::Sender` can be copied, whereas a `piper::spmc::Sender` cannot. Note that a `piper::mpsc::Sender` can only be constructed from a `piper::mpsc::Receiver` or `piper::mpsc::Channel`, or copied from another `piper::mpsc::Sender`. Check the API reference for details.

#### Receiver

A `piper::Receiver` is an abstract template class used to receive data from a channel buffer. A `piper::spmc::Receiver` can be copied, whereas a `piper::mpsc::Receiver` cannot. Note that a `piper::spmc::Receiver` can only be constructed from a `piper::spmc::Sender` or `piper::spmc::Channel`, or copied from another `piper::spmc::Receiver`. Check the API reference for details.

#### Channel

A `piper::Channel` is an abstract template class that composes `piper::Sender` and `piper::Receiver`. A channel cannot be copied, only moved. However, Senders or Receivers may be copied from a Channel, depending on the concrete implementation.

#### Flavors

Concurrent channels often come in different "flavors", which correspond to the type of underlying buffer used to transmit data from a Sender to a Receiver. Different flavors may be used to achieve different levels of synchronization between Senders and Receivers.

##### Asynchronous

An asynchronous channel is one whose buffer is unbounded. Aggressive senders may grow the buffer instead of blocking while waiting for a receiver. 

The default constructors for `piper::mpsc::Receiver`, `piper::spmc::Sender`, and all concrete implementations of `piper::Channel` utilize an asynchronous buffer.

##### Synchronous

A synchronous channel is one whose buffer is bounded. Aggressive senders may block while waiting for a receiver, should the buffer be full. 

Constructors for `piper::mpsc::Receiver`, `piper::spmc::Sender`, and all concrete implementations of `piper::Channel` that take a `std::size_t n` parameter will only utilize a synchronous buffer if `n > 0`. See [Rendezvous](#rendezvous) for more details.

##### Rendezvous

A rendezvous channel is one whose buffer has no capacity. In practice, this means that a Sender blocks until a Receiver has collected the transmitted data, allowing both threads to continue at a synchronized point. 

Constructors for `piper::mpsc::Receiver`, `piper::spmc::Sender`, and all concrete implementations of `piper::Channel` that take a `std::size_t n` parameter will only utilize a rendezvous buffer if `n == 0`. See [Synchronous](#synchronous) for more details.

### Future Plans

I plan to implement more robust tests for the existing codebase, and add different ownership schemes in concrete implementations for `piper::Channel` to help with flexibility.
