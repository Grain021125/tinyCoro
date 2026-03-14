#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

auto switch_to_new_thread(std::jthread &out)
{
    struct awaiterble
    {
        std::jthread *p_out;
        bool await_ready() {return false;}
        void await_suspend(std::coroutine_handle<> h) {
            std::jthread &out = *p_out;
            if (out.joinable())
                throw std::runtime_error("jthread 输出参数非空");
        }
    };
}