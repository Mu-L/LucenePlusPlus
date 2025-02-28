/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <boost/asio.hpp>
#include <boost/any.hpp>
#include <boost/thread/thread.hpp>
#include "LuceneObject.h"

namespace Lucene {


typedef boost::asio::io_context io_context_t;
typedef boost::asio::executor_work_guard<io_context_t::executor_type> work_t;

/// A Future represents the result of an asynchronous computation. Methods are provided to check if the computation
/// is complete, to wait for its completion, and to retrieve the result of the computation. The result can only be
/// retrieved using method get when the computation has completed, blocking if necessary until it is ready.
class Future : public LuceneObject {
public:
    virtual ~Future();

protected:
    boost::any value;

public:
    void set(const boost::any& value) {
        SyncLock syncLock(this);
        this->value = value;
    }

    template <typename TYPE>
    TYPE get() {
        SyncLock syncLock(this);
        while (value.empty()) {
            wait(10);
        }
        return value.empty() ? TYPE() : boost::any_cast<TYPE>(value);
    }
};

/// Utility class to handle a pool of threads.
class ThreadPool : public LuceneObject {
public:
    ThreadPool();
    virtual ~ThreadPool();

    LUCENE_CLASS(ThreadPool);

protected:
    io_context_t io_context;
    work_t work;
    boost::thread_group threadGroup;

    static const int32_t THREADPOOL_SIZE;

public:
    /// Get singleton thread pool instance.
    static ThreadPoolPtr getInstance();

    template <typename FUNC>
    FuturePtr scheduleTask(FUNC func) {
        FuturePtr future(newInstance<Future>());
        boost::asio::post(io_context, boost::bind(&ThreadPool::execute<FUNC>, this, func, future));
        return future;
    }

protected:
    // this will be executed when one of the threads is available
    template <typename FUNC>
    void execute(FUNC func, const FuturePtr& future) {
        future->set(func());
        future->notifyAll();
    }
};

}

#endif
