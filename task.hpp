/**
 * Copyright 2025, Aleksandar Colic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#ifndef TASK_HPP
#define TASK_HPP

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "types.hpp"
#include "util.hpp"

using SystemClock = system_clock;
using SystemTimePoint = std::chrono::time_point<SystemClock>;

inline SystemTimePoint now_sys() noexcept
{
    return SystemClock::now();
}

inline u64 now_sys_ns() noexcept
{
    return now_sys().time_since_epoch().count();
}

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-convert-member-functions-to-static)

constexpr bool dev = false;

namespace fs = std::filesystem;

constexpr char path_sep = fs::path::preferred_separator;
const inline std::string path_sep_str = std::string{path_sep}; // NOLINT

// clang-format off
inline const std::string main_dir = ".tt";                            /* .tt/         */ // NOLINT
inline const std::string tasks_dir = main_dir + path_sep + "tasks";   /* .tt/tasks/   */ // NOLINT
inline const std::string md_file = main_dir + path_sep + "md";        /* .tt/md       */ // NOLINT
inline const std::string msg_file = main_dir + path_sep + "desc_msg"; /* .tt/desc_msg */ // NOLINT
// clang-format on

template<class T>
void log(T&& value)
{
    if constexpr (dev)
        std::cout << std::forward<T>(value) << "\n";
}

template<class... Args>
void log(const std::format_string<Args...>& fmt, Args&&... args)
{
    if constexpr (dev)
        std::cout << std::format(fmt, std::forward<Args>(args)...) << "\n";
}

enum class ID : u64 {};
enum class Type : u8 { task = 0, bug = 1, feature = 2 };
enum class Status : u8 { not_started = 0, in_progress = 1, done = 2 };

template<class T>
constexpr T as(u64 value)
{
    if constexpr (std::is_same_v<T, ID>) {
        return ID(value);
    }
    else if constexpr (std::is_same_v<T, Type>) {
        if (value > u64(Type::feature))
            throw std::runtime_error{"Invalid task type."};

        return Type(value);
    }
    else if constexpr (std::is_same_v<T, Status>) {
        if (value > u64(Status::done))
            throw std::runtime_error{"Invalid task status."};

        return Status(value);
    }
    else
        static_assert(!"Invalid return type.");
}

template<class T>
u64 as_num(T v) noexcept
{
    return static_cast<u64>(v);
}

/**
 * Types as strings.
 */

enum class show { short_, long_ };

inline std::string as_string(ID id)
{
    return std::to_string(as_num(id));
}

template<show sh = show::long_>
inline std::string as_string(Type t)
{
    switch (t) { // clang-format off
    case Type::task:    return sh == show::short_ ? "T" : "Task";
    case Type::bug:     return sh == show::short_ ? "B" : "Bug";
    case Type::feature: return sh == show::short_ ? "F" : "Feature";
    default:            return "invalid";
    } // clang-format on
}

template<show sh = show::long_>
inline std::string as_string(Status s)
{
    switch (s) { // clang-format off
    case Status::not_started: return sh == show::short_ ? "N" : "Not started";
    case Status::in_progress: return sh == show::short_ ? "I" : "In progress";
    case Status::done:        return sh == show::short_ ? "R" : "Resolved";
    default:                  return "invalid";
    } // clang-format on
}

struct MD {
    ID m_id;

    ID next_id()
    {
        ID next = m_id;
        m_id = as<ID>(as_num(m_id) + 1);
        return next;
    }
};

static constexpr MD initial_md = {
    .m_id = as<ID>(1),
};

inline MD md_from_fstream(std::ifstream& ifs)
{
    u64 n{};
    ifs >> n;

    return MD{.m_id = as<ID>(n)};
}

inline void md_to_fstream(std::ofstream& ofs, const MD& md)
{
    ofs << as_num(md.m_id);
}

class Task {
    friend inline std::ifstream& operator>>(std::ifstream& ifs, Task& task);

public:
    Task(ID id, Type type, Status status, std::string desc)
        : m_id{id}
        , m_type{type}
        , m_status{status}
        , m_desc{std::move(desc)}
    {
    }

    [[nodiscard]] ID id() const noexcept { return m_id; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& desc() const noexcept { return m_desc; }

    [[nodiscard]] usize desc_size() const noexcept { return m_desc.size(); }

    void set_type(Type t) { m_type = t; }

    void set_status(Status s) { m_status = s; }

    void set_desc(std::string desc) { m_desc = std::move(desc); }

    void roll_status()
    {
        if (status() == Status::done)
            throw std::runtime_error{"Cannot roll task with status done."};

        m_status = as<Status>(as_num(m_status) + 1);
    }

    void rollback_status()
    {
        if (status() == Status::not_started)
            throw std::runtime_error{"Cannot rollback task with status not started."};

        m_status = as<Status>(as_num(m_status) - 1);
    }

    [[nodiscard]] std::string short_desc() const noexcept
    {
        size_t start = 0;
        while (start < m_desc.size() && std::isspace(m_desc[start]))
            ++start;

        return m_desc.substr(start, m_desc.find('\n', start) - start);
    }

    [[nodiscard]] std::string for_log() const noexcept
    {
        return std::format("{} {} {} {}", as_string(id()), as_string<show::short_>(type()),
                           as_string<show::short_>(status()), short_desc());
    }

    [[nodiscard]] std::string for_show() const noexcept
    {
        return std::format("{}\n{}\n{}\n\n{}", as_string(id()), as_string<show::long_>(type()),
                           as_string<show::long_>(status()), desc());
    }

    /**
     * Spaceship operator first compares id, then type etc.
     */
    auto operator<=>(const Task& other) const noexcept = default;

private:
    ID m_id;
    Type m_type;
    Status m_status;
    std::string m_desc;
};

static void task_to_fstream(std::ofstream& ofs, const Task& task)
{
    ofs << as_num(task.id()) << "\n";
    ofs << as_num(task.type()) << "\n";
    ofs << as_num(task.status()) << "\n";
    ofs << task.desc() << "\n";
}

static Task task_from_fstream(std::ifstream& ifs)
{
    ID id{};
    Type type{};
    Status status{};

    u64 n{};
    ifs >> n, id = as<ID>(n), ifs >> std::ws;
    ifs >> n, type = as<Type>(n), ifs >> std::ws;
    ifs >> n, status = as<Status>(n), ifs >> std::ws;

    std::string text{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

    return Task{id, type, status, std::move(text)};
}

class TaskTracker {
public:
    explicit TaskTracker() : m_md{read_md()} {}

    TaskTracker(const TaskTracker&) = delete;
    TaskTracker(TaskTracker&&) noexcept = delete;
    TaskTracker& operator=(const TaskTracker&) = delete;
    TaskTracker& operator=(TaskTracker&&) noexcept = delete;

    ~TaskTracker()
    {
        try {
            auto md_stream{open_md_write()};
            md_to_fstream(md_stream, m_md);
        }
        catch (const std::exception& ex) {
            std::cout << "Failed to write new md: " << ex.what() << "\n";
        }
    }

    static void cmd_init()
    {
        if (std::filesystem::exists(main_dir))
            throw std::runtime_error{"Task tracker already initialized."};

        std::filesystem::create_directory(main_dir);
        std::filesystem::create_directory(tasks_dir);
        std::ofstream mdfs{open_md_write()};
        md_to_fstream(mdfs, initial_md);
    }

    std::string task_path(ID id) { return tasks_dir + path_sep + as_string(id); }

    /**
     * Returns all tasks in descending order.
     */
    std::vector<Task> all_tasks() const noexcept
    {
        return all_tasks([](const Task& t) { return true; });
    }

    /**
     * Returns all tasks in descending order where tasks match predicate.
     */
    template<typename Pred>
    std::vector<Task> all_tasks(Pred pred) const noexcept
    {
        std::vector<Task> tasks;
        tasks.reserve(1024);

        for (const auto& entry : fs::directory_iterator{tasks_dir}) {
            std::ifstream is{entry.path()};
            Task task{task_from_fstream(is)};
            if (pred(task))
                tasks.emplace_back(task);
        }

        std::ranges::sort(tasks, std::ranges::greater());

        return tasks;
    }

    [[nodiscard]] Task get_task(ID id)
    {
        std::ifstream ifs{task_path(id)};
        if (!ifs)
            throw std::runtime_error{std::format("Task {} does not exist.", as_num(id))};

        return task_from_fstream(ifs);
    }

    void change_task_status(ID id, Status status)
    {
        Task t{get_task(id)};
        t.set_status(status);
        save_task(t);
    }

    void save_task(const Task& task)
    {
        std::ofstream ofs{task_path(task.id()), std::ios::trunc};
        task_to_fstream(ofs, task);
    }

    void new_task(Type type, std::string desc)
    {
        save_task(Task{next_id(), type, Status::not_started, std::move(desc)});
    }

    void new_task(std::string desc) { new_task(Type::task, std::move(desc)); }

    void reopen_task(ID id) { change_task_status(id, Status::not_started); }

    void in_progress_task(ID id) { change_task_status(id, Status::in_progress); }

    void resolve_task(ID id) { change_task_status(id, Status::done); }

    void roll(ID id)
    {
        Task t{get_task(id)};
        t.roll_status();
        save_task(t);
    }

    void rollback(ID id)
    {
        Task t{get_task(id)};
        t.rollback_status();
        save_task(t);
    }

private:
    static std::ifstream open_md_read()
    {
        if (!std::filesystem::exists(main_dir)) {
            if constexpr (!dev)
                throw std::runtime_error{"Task tracker not initialized. Please run init."};

            cmd_init();
        }

        return std::ifstream{md_file};
    }

    static std::ofstream open_md_write() { return std::ofstream{md_file, std::ios::trunc}; }

    static MD read_md()
    {
        std::ifstream md_stream{open_md_read()};
        return md_from_fstream(md_stream);
    }

    // Task id is task creation time (system time point) in nanoseconds.
    ID next_id() { return as<ID>(now_sys_ns()); }

private: // NOLINT
    MD m_md;
};

// NOLINTEND(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-convert-member-functions-to-static)

#endif // TASK_HPP
