/* 
 * File:   ThreadedQueue.h
 * Author: Catherine
 *
 * Created on May 3, 2009, 6:57 AM
 *
 * This file is a part of Shoddy Battle.
 * Copyright (C) 2009  Catherine Fitzpatrick and Benjamin Gwin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, visit the Free Software Foundation, Inc.
 * online at http://gnu.org.
 */

#ifndef _THREADED_QUEUE_H_
#define _THREADED_QUEUE_H_

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <queue>

namespace shoddybattle { namespace network {

/**
 * This is a class used for running code on a dispatch thread. Client threads
 * can post messages of type T to the ThreadedQueue, and the ThreadedQueue
 * will call a delegate method on each message.
 */
template <class T>
class ThreadedQueue {
public:
    typedef boost::function<void (T &)> DELEGATE;
    typedef boost::function<void ()> INITIALISER;

    ThreadedQueue(DELEGATE delegate):
            m_impl(new ThreadedQueueImpl(delegate)) {
        m_impl->thread = boost::thread(
                boost::bind(&ThreadedQueue::process, m_impl));
    }

    ThreadedQueue(INITIALISER f, DELEGATE delegate):
            m_impl(new ThreadedQueueImpl(delegate)) {
        m_impl->thread = boost::thread(
                boost::bind(&ThreadedQueue::initialise, f, m_impl));
    }

    void post(T elem) {
        {
            boost::unique_lock<boost::mutex> lock(m_impl->mutex);
            m_impl->queue.push(elem);
        }
        m_impl->condition.notify_one();
    }

    ~ThreadedQueue() {
        if (boost::this_thread::get_id() == m_impl->thread.get_id()) {
            // We get here if m_impl->thread is at Point B in process().
            m_impl->terminated = true;
            m_impl->thread.detach();
            return;
        }
        // We get here if m_impl->thread is at Point A in process().
        boost::unique_lock<boost::mutex> lock(m_impl->mutex);
        m_impl->terminated = true;
        while (!m_impl->queue.empty()) {
            m_impl->condition.wait(lock);
        }
        lock.unlock();
        m_impl->condition.notify_one();
        m_impl->thread.join();
    }

    struct ThreadedQueueImpl {
        boost::mutex mutex;
        boost::condition_variable condition;
        bool terminated;
        DELEGATE delegate;
        std::queue<T> queue;
        boost::thread thread;

        ThreadedQueueImpl(DELEGATE delegate):
                terminated(false),
                delegate(delegate) { }
    };

private:

    static void initialise(INITIALISER f,
            boost::shared_ptr<ThreadedQueueImpl> impl) {
        f();
        process(impl);
    }

    static void process(boost::shared_ptr<ThreadedQueueImpl> impl) {
        while (!impl->terminated) {
            boost::unique_lock<boost::mutex> lock(impl->mutex);
            while (impl->queue.empty()) {
                impl->condition.wait(lock); // Point A
                if (impl->terminated) {
                    // Break out of two levels of while loops. Goto is the most
                    // transparent way to do this.
                    goto cleanup;
                }
            }
            T item = impl->queue.front();
            lock.unlock();
            impl->delegate(item); // Point B
            lock.lock();
            impl->queue.pop();
            impl->condition.notify_one();
        }
        cleanup:
        // Run the last entries in the queue.
        boost::unique_lock<boost::mutex> lock(impl->mutex);
        while (!impl->queue.empty()) {
            impl->delegate(impl->queue.front());
            impl->queue.pop();
        }
    }

    boost::shared_ptr<ThreadedQueueImpl> m_impl;
};

}} // namespace shoddybattle::network

#endif

