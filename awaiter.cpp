#include <coroutine>
#include <iostream>

class Event
{
public:
    Event()
    {
        std::cout << "event construct" << std::endl;
    }

    ~Event()
    {
        std::cout << "event deconstruct" << std::endl;
    }

    bool await_ready()
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
    {
        return caller;
    }

    void await_resume()
    {
        std::cout << "await resume called" << std::endl;
    }
};

class Task
{
public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    class promise_type
    {
        friend class Task;

    private:
        int m_id;

    public:
        promise_type(int id) : m_id(id) {}
        Task get_return_object()
        {
            auto handle = handle_type::from_promise(*this);
            return Task(handle, m_id);
        }

        constexpr std::suspend_never initial_suspend()
        {
            return {};
        }

        void return_void() {}

        void unhandled_exception() {}

        constexpr std::suspend_always final_suspend() noexcept { return {}; }
    };

public:
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    Task(Task &&other) : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }
    Task &operator=(Task &&other)
    {
        if (m_handle)
        {
            m_handle.destroy();
        }

        m_handle = other.m_handle;
        other.m_handle = nullptr;
        return *this;
    }

    ~Task()
    {
        m_handle.destroy();
        std::cout << "deconstruct task" << m_id << std::endl;
    }

public:
    auto operator co_await()
    {
        return Event{};
    }

    void resume()
    {
        m_handle.resume();
    }

private:
    Task(handle_type handle, int id) : m_handle(handle), m_id(id)
    {
        std::cout << "construct task" << m_id << std::endl;
    }
    handle_type m_handle;
    int m_id;
};

Task run(int i)
{
    std::cout << "task " << i << " start" << std::endl;
    if (i == 0)
    {
        auto t1 = run(i + 1);
        co_await t1;
        t1.resume();
        std::cout << "task " << i << " will suspend" << std::endl;
        co_await std::suspend_always{};
    }
    else
    {
        std::cout << "task " << i << " will suspend" << std::endl;
        co_await std::suspend_always{};
    }

    std::cout << "back to task " << i << std::endl;

    std::cout << "task " << i << " end" << std::endl;
    co_return;
}

int main(int argc, char const *argv[])
{
    /* code */
    auto task = run(0);
    std::cout << "back to main" << std::endl;
    task.resume();
    std::cout << "run finish" << std::endl;
    return 0;
}