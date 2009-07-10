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

namespace shoddybattle { namespace network {

/**
 * This is a class used for running code on a dispatch thread. Client threads
 * can post messages of type T to the ThreadedQueue, and the ThreadedQueue
 * will call a delegate method on each message. The ThreadedQueue does not
 * actually have any internal storage: attempts to post a message while one is
 * presently being processed while block until the message can be posted.
 */
template <class T>
class ThreadedQueue {
public:
    typedef boost::function<void (T &)> DELEGATE;

    ThreadedQueue(DELEGATE delegate):
            m_impl(new ThreadedQueueImpl(delegate)) {
        m_impl->thread = boost::thread(
                boost::bind(&ThreadedQueue::process, m_impl));
    }

    void post(T elem) {
        boost::unique_lock<boost::mutex> lock(m_impl->mutex);
        while (!m_impl->empty) {
            m_impl->condition.wait(lock);
        }
        m_impl->item = elem;
        m_impl->empty = false;
        lock.unlock();
        m_impl->condition.notify_one();
    }

    void terminate() {
        if (boost::this_thread::get_id() == m_impl->thread.get_id()) {
            m_impl->terminated = true;
            return;
        }
        boost::unique_lock<boost::mutex> lock(m_impl->mutex);
        if (!m_impl->terminated) {
            m_impl->terminated = true;
            while (!m_impl->empty) {
                m_impl->condition.wait(lock);
            }
            m_impl->empty = false;
            lock.unlock();
            m_impl->condition.notify_one();
            m_impl->thread.join();
        }
    }

    ~ThreadedQueue() {
        terminate();
    }

    struct ThreadedQueueImpl {
        boost::mutex mutex;
        boost::condition_variable condition;
        bool terminated;
        bool empty;
        DELEGATE delegate;
        T item;
        boost::thread thread;

        ThreadedQueueImpl(DELEGATE delegate):
                delegate(delegate),
                empty(true),
                terminated(false) { }
    };

private:

    static void process(boost::shared_ptr<ThreadedQueueImpl> impl) {
        boost::unique_lock<boost::mutex> lock(impl->mutex);
        while (!impl->terminated) {
            while (impl->empty) {
                impl->condition.wait(lock);
            }
            if (impl->terminated) {
                return;
            }
            impl->delegate(impl->item);
            impl->empty = true;
            impl->condition.notify_one();
        }
    }

    boost::shared_ptr<ThreadedQueueImpl> m_impl;
};

}} // namespace shoddybattle::network

#endif

