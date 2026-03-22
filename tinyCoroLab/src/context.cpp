#include "coro/context.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = this;
    m_engine.init();
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = nullptr;
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            if (!(this->m_stop_cb))
            {
                set_stop_cb([&]() { m_job->request_stop(); });
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job->request_stop();
    m_engine.wake_up();
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_add(static_cast<size_t>(register_cnt), std::memory_order_acq_rel);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_sub(static_cast<size_t>(register_cnt), std::memory_order_acq_rel);
}

auto context::process_work() noexcept -> void
{
    auto num = m_engine.num_task_schedule();
    for (int i = 0; i < num; ++i)
    {
        m_engine.exec_one_task();
    }
}

auto context::poll_work() noexcept -> void
{
    m_engine.poll_submit();
}

auto context::empty_wait_task() noexcept -> bool
{
    return m_num_wait_task.load(memory_order_acquire) == 0 && m_engine.empty_io();
}

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    while (!token.stop_requested())
    {
        process_work(); // 执行计算任务
        if (empty_wait_task())
        {
            if (!m_engine.ready())
            {
                m_stop_cb();
            }
            else
            {
                continue;
            }
        }

        poll_work(); // 执行IO任务
    }
}

}; // namespace coro