#include "concurrency/cross_thread_signal.hpp"

#include <boost/bind.hpp>

cross_thread_signal_t::cross_thread_signal_t(signal_t *source, int dest) :
    source_thread(get_thread_id()), dest_thread(dest),
    rethreader((signal_t *)this, dest_thread),
    subs(boost::bind(&cross_thread_signal_t::on_signal_pulsed,
        this, auto_drainer_t::lock_t(&drainer)))
{
    rassert(source->home_thread() == source_thread);
    if (source->is_pulsed()) {
        on_signal_pulsed(auto_drainer_t::lock_t(&drainer));
    } else {
        subs.resubscribe(source);
    }
}

void cross_thread_signal_t::on_signal_pulsed(auto_drainer_t::lock_t keepalive) {
    /* We can't do anything that blocks when we're in a signal callback, so we
    have to spawn a new coroutine to do the thread switching. */
    coro_t::spawn_sometime(boost::bind(&cross_thread_signal_t::deliver, this, keepalive));
}

void cross_thread_signal_t::deliver(UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t thread_switcher(dest_thread);
    signal_t::pulse();
}