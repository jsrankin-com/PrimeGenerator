#pragma once
#include <coroutine>
#include <optional>

template<std::movable T>
class Generator {
public:
    struct promise_type {
        Generator<T> get_return_object() {
            return Generator{ Handle::from_promise(*this) };
        }
        static auto initial_suspend() noexcept {
            return std::suspend_always{};
        }
        static auto final_suspend() noexcept {
            return std::suspend_always{};
        }
        auto yield_value(T value) noexcept {
            current_value = std::move(value);
            return std::suspend_always{};
        }
        // Disallow co_await in generator coroutines.
        void await_transform() = delete;
        [[noreturn]]
        static void unhandled_exception() {
            throw;
        }
        // Added to supress compiler warnings
        void return_void() noexcept {}
        std::optional<T> current_value;
    };

    using Handle = std::coroutine_handle<promise_type>;

    explicit Generator(const Handle coroutine) :
        m_coroutine{ coroutine }
    {}

    Generator() = default;
    ~Generator() {
        if (m_coroutine) {
            m_coroutine.destroy();
        }
    }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    Generator(Generator&& other) noexcept :
        m_coroutine{ other.m_coroutine }
    {
        other.m_coroutine = {};
    }
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (m_coroutine) {
                m_coroutine.destroy();
            }
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    // Range-based for loop support.
    class Iter {
    public:
        void operator++() {
            m_coroutine.resume();
        }
        const T& operator*() const {
            return *m_coroutine.promise().current_value;
        }
        bool operator==(std::default_sentinel_t) const {
            return !m_coroutine || m_coroutine.done();
        }

        explicit Iter(const Handle coroutine) :
            m_coroutine{ coroutine }
        {}

    private:
        Handle m_coroutine;
    };

    Iter begin() {
        if (m_coroutine) {
            m_coroutine.resume();
        }
        return Iter{ m_coroutine };
    }
    auto end() {
        return std::default_sentinel_t{};
    }

private:
    Handle m_coroutine;
};

/*
References:
https://en.cppreference.com/w/cpp/coroutine/coroutine_handle
Author:
	Jeffrey S. Rankin
	http://jsrankin.com
	admin@jsrankin.com
*/
