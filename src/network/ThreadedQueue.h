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

#include <memory>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace shoddybattle { namespace network {

/**
 * This is a class used for running code on a dispatch thread. Client threads
 * can post messages of type T to the ThreadedQueue, and the ThreadedQueue
 * will call a delegate method on each message.
 */
template <class T>
class ThreadedQueue : boost::noncopyable {
public:
    typedef boost::function<void (T &)> DELEGATE;

    ThreadedQueue(DELEGATE delegate):
            m_delegate(delegate),
            m_service(new boost::asio::io_service()),
            m_work(new boost::asio::io_service::work(*m_service)),
            m_thread(boost::bind(&ThreadedQueue::run, this)) {
    }

    void post(T elem) {
        m_service->post(boost::bind(m_delegate, elem));
    }

    template <class U> void post(U elem) {
        m_service->post(elem);
    }

    void join() {
        m_work.reset();
        if (m_thread.get_id() != boost::this_thread::get_id()) {
            m_thread.join();
        }
    }

    ~ThreadedQueue() {
        join();
    }

private:
    void run() {
        // We make a copy of the shared_ptr<> holding the io_service so that
        // the io_service does not get deconstructed until after run() exits.
        boost::shared_ptr<boost::asio::io_service> service = m_service;
        service->run();
    }

    DELEGATE m_delegate;
    // Note: Do not change the order of the following three declarations.
    boost::shared_ptr<boost::asio::io_service> m_service;
    std::auto_ptr<boost::asio::io_service::work> m_work;
    boost::thread m_thread;
};

}} // namespace shoddybattle::network

#endif
