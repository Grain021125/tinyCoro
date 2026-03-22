#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    linfo.egn            = this;
    m_num_io_running     = 0;
    m_num_io_wait_submit = 0;
    m_upxy.init(config::kEntryLength);
}

auto engine::deinit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.deinit();
    m_num_io_running     = 0;
    m_num_io_wait_submit = 0;
    mpmc_queue<coroutine_handle<>> task_queue;
    m_task_queue.swap(task_queue);
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return !m_task_queue.was_empty();
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    return m_upxy.get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    auto coro = m_task_queue.pop();
    assert(bool(coro));
    return coro;
}

auto engine::wake_up(uint64_t val) noexcept -> void
{
    m_upxy.write_eventfd(val);
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes
    assert(handle != nullptr && "engine get nullptr task handle");
    m_task_queue.push(handle);
    wake_up();
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::do_io_submit() noexcept -> void
{
    // IO操作不跨线程
    if (m_num_io_wait_submit > 0)
    {
        m_upxy.submit();
        m_num_io_running += m_num_io_wait_submit;
        m_num_io_wait_submit = 0;
    }
}

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    // submit I/O
    do_io_submit();

    // 等待I/O执行
    auto cnt = m_upxy.wait_eventfd(); // 等待 IO 执行
    if (!wake_by_cqe(cnt))
    {
        return;
    }

    // 取出IO
    auto num = m_upxy.peek_batch_cqe(m_urc.data(), m_num_io_running); // 一个io_uring只绑定一个线程，不需要原子变量

    if (num != 0)
    {
        for (int i = 0; i < num; ++i) {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
        // m_num_io_running.fetch_sub(num, std::memory_order_acq_rel);
        m_num_io_running -= num;
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    ++m_num_io_wait_submit;
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return m_num_io_running == 0 && m_num_io_wait_submit == 0;
}
}; // namespace coro::detail
