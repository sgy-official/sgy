

#include <ripple/core/Stoppable.h>
#include <cassert>

namespace ripple {

Stoppable::Stoppable (std::string name, RootStoppable& root)
    : m_name (std::move (name))
    , m_root (root)
    , m_child (this)
{
}

Stoppable::Stoppable (std::string name, Stoppable& parent)
    : m_name (std::move (name))
    , m_root (parent.m_root)
    , m_child (this)
{
    assert (! parent.isStopping());

    parent.m_children.push_front (&m_child);
}

Stoppable::~Stoppable ()
{
}

bool Stoppable::isStopping() const
{
    return m_root.isStopping();
}

bool Stoppable::isStopped () const
{
    return m_stopped;
}

bool Stoppable::areChildrenStopped () const
{
    return m_childrenStopped;
}

void Stoppable::stopped ()
{
    std::lock_guard<std::mutex> lk{m_mut};
    m_is_stopping = true;
    m_cv.notify_all();
}

void Stoppable::onPrepare ()
{
}

void Stoppable::onStart ()
{
}

void Stoppable::onStop ()
{
    stopped();
}

void Stoppable::onChildrenStopped ()
{
}


void Stoppable::prepareRecursive ()
{
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->prepareRecursive ();
    onPrepare ();
}

void Stoppable::startRecursive ()
{
    onStart ();
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->startRecursive ();
}

void Stoppable::stopAsyncRecursive (beast::Journal j)
{
    onStop ();

    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->stopAsyncRecursive(j);
}

void Stoppable::stopRecursive (beast::Journal j)
{
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->stopRecursive (j);

    m_childrenStopped = true;
    onChildrenStopped ();

    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lk{m_mut};
    if (!m_cv.wait_for(lk, 1s, [this]{return m_is_stopping;}))
    {
        if (auto stream = j.error())
            stream << "Waiting for '" << m_name << "' to stop";
        m_cv.wait(lk, [this]{return m_is_stopping;});
    }
    m_stopped = true;
}


RootStoppable::RootStoppable (std::string name)
    : Stoppable (std::move (name), *this)
{
}

RootStoppable::~RootStoppable ()
{
    using namespace std::chrono_literals;
    jobCounter_.join(m_name.c_str(), 1s, debugLog());
}

bool RootStoppable::isStopping() const
{
    return m_calledStop;
}

void RootStoppable::prepare ()
{
    if (m_prepared.exchange (true) == false)
        prepareRecursive ();
}

void RootStoppable::start ()
{
    if (m_prepared.exchange (true) == false)
        prepareRecursive ();

    if (m_started.exchange (true) == false)
        startRecursive ();
}

void RootStoppable::stop (beast::Journal j)
{
    assert (m_started);

    if (stopAsync (j))
        stopRecursive (j);
}

bool RootStoppable::stopAsync (beast::Journal j)
{
    bool alreadyCalled;
    {
        std::unique_lock<std::mutex> lock (m_);
        alreadyCalled = m_calledStop.exchange (true);
    }
    if (alreadyCalled)
    {
        if (auto stream = j.warn())
            stream << "Stoppable::stop called again";
        return false;
    }

    using namespace std::chrono_literals;
    jobCounter_.join (m_name.c_str(), 1s, j);

    c_.notify_all();
    stopAsyncRecursive(j);
    return true;
}

}
























