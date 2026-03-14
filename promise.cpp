#include <coroutine>
#include <iostream>
#include <optional>
#include <string>

// 面向用户的对象
class Task
{
public:
  class promise_type;
  using handle_type = std::coroutine_handle<promise_type>; // 协程句柄类型
  class promise_type // 与 Task 关联的 promise 对象
  {
    friend class Task;

  public:
    Task get_return_object() // 用于构造协程（编译器生成调用代码）
    {
      auto handle = handle_type::from_promise(*this);
      return Task(handle);
    }

    // 控制协程创建完成后的调度逻辑（编译器生成调用代码）
    constexpr std::suspend_always initial_suspend() { return {}; }

    // 协程返回值时会调用该函数（编译器生成调用代码）
    void return_value(std::string result)
    {
      m_result = result;
    }

    // 协程 yeild 值时会调用该函数（编译器生成调用代码）
    std::suspend_always yield_value(int value)
    {
      m_yield = value;
      return {};
    }

    // 协程运行抛出异常时会调用该函数（编译器生成调用代码）
    void unhandled_exception()
    {
      m_exception = std::current_exception();
    }

    // 协程执行完毕后会调用该函数（编译器生成调用代码）
    constexpr std::suspend_always final_suspend() noexcept { return {}; }

  private:
    std::exception_ptr m_exception{nullptr};
    std::optional<std::string> m_result{std::nullopt};
    std::optional<int> m_yield{std::nullopt};
  };

public:
  // 禁止拷贝构造
  Task(const Task &) = delete;

  // 允许移动构造
  Task(Task &&other) : m_handle(other.m_handle)
  {
    other.m_handle = nullptr;
  }

  // 禁止拷贝赋值
  Task &operator=(const Task &other) = delete;

  // 允许移动赋值
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

  // 析构自动释放关联的协程句柄
  ~Task()
  {
    m_handle.destroy();
  }

public:
  void resume()
  {
    m_handle.resume();
  }

  // 用户侧不断调用 next 驱动协程 yield 值
  std::optional<int> next()
  {
    promise_type &p = m_handle.promise(); // 首先获取协程句柄关联的 promise
    p.m_yield = std::nullopt; // 必须先置存储 yield 的变量为 nullopt 这样才能正确使得 yield 结束后 next 返回 nullopt
    p.m_exception = nullptr;
    if (!m_handle.done())
    {
      m_handle.resume(); // 恢复协程运行
    }
    if (p.m_exception)
    {
      std::rethrow_exception(p.m_exception);
    }
    return p.m_yield;
  }

  // 用户手动调用该函数获取协程返回值，注意协程可能此时并没有返回值
  std::optional<std::string> result()
  {
    return m_handle.promise().m_result;
  }

private:
  Task(handle_type handle) : m_handle(handle) {}

private:
  handle_type m_handle;
};

Task run(int i)
{
  std::cout << "task " << i << " start" << std::endl;
  co_yield 1;
  for (int i = 2; i <= 5; i++)
  {
    co_yield i;
  }
  std::cout << "task " << i << " end" << std::endl;
  co_return "task run finish";
}

int main(int argc, char const *argv[])
{
  /* code */
  auto task = run(5);
  std::optional<int> val;
  while ((val = task.next()))
  {
    std::cout << "get yield value: " << (*val) << std::endl;
  }
  std::cout << "get return value: " << (*task.result()) << std::endl;
  return 0;
}