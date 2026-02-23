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

/**
 * Set to true to enter test_main().
 */
constexpr bool dev = false;

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-convert-member-functions-to-static)

namespace fs = std::filesystem;

using SystemClock = system_clock;
using SystemTimePoint = std::chrono::time_point<SystemClock>;

inline SystemTimePoint now_sys() noexcept
{
    return SystemClock::now();
}

inline u64 now_sys_ns() noexcept
{
    return static_cast<u64>(now_sys().time_since_epoch().count());
}

inline fs::path home_dir()
{
    if (char* home_dir = std::getenv("HOME"))
        return home_dir;

    if (char* home_dir = std::getenv("USERPROFILE"))
        return home_dir;

    /**
     * TODO: Check how to get windows path if the above fails.
     * Maybe something like this:
     */
    if (char *drive = std::getenv("HOMEDRIVE"), *path = std::getenv("HOMEPATH"); drive && path)
        return fs::path{drive}.append(path);

    return "";
}

inline std::string default_username()
{
    if (char* user = std::getenv("USER"))
        return user;

    if (char* user = std::getenv("USERNAME"))
        return user;

    return "any";
}

inline std::string default_email()
{
    return "none";
}

/* clang-format off */
inline const fs::path main_dir = ".tt";                      /* .tt/         */
inline const fs::path tasks_global_dir = main_dir / "tasks"; /* .tt/tasks/   */
inline const fs::path md_file = main_dir / "md";             /* .tt/md       */
inline const fs::path msg_file = main_dir / "desc_msg";      /* .tt/desc_msg */

inline const fs::path cfg_file = home_dir() / ".ttconfig";   /* ~/.ttconfig  */

inline const fs::path refs_filename = "refs";                /* refs filename, located in .tt/<user>/refs */
/* clang-format on */

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
enum class Scope : u8 { global = 0, local = 1 };
enum class Type : u8 { task = 0, bug = 1, feature = 2 };
enum class Status : u8 { not_started = 0, in_progress = 1, done = 2 };

enum class Offset : u64 {};

template<class T>
constexpr T as(u64 value)
{
    if constexpr (std::is_same_v<T, ID>) {
        return ID(value);
    }
    else if constexpr (std::is_same_v<T, Scope>) {
        if (value > u64(Scope::local))
            throw std::runtime_error{"Invalid task scope."};

        return Scope(value);
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
    else if constexpr (std::is_same_v<T, Offset>) {
        return Offset(value);
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
inline std::string as_string(Scope t)
{
    switch (t) { // clang-format off
    case Scope::global: return sh == show::short_ ? "G" : "Global";
    case Scope::local:  return sh == show::short_ ? "L" : "Local";
    default:            return "invalid";
    } // clang-format on
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
    Task() = default;

    Task(ID id, Scope scope, Type type, Status status, std::string desc)
        : m_id{id}
        , m_scope{scope}
        , m_type{type}
        , m_status{status}
        , m_desc{std::move(desc)}
    {
    }

    [[nodiscard]] ID id() const noexcept { return m_id; }

    [[nodiscard]] Scope scope() const noexcept { return m_scope; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& desc() const noexcept { return m_desc; }

    [[nodiscard]] usize desc_size() const noexcept { return m_desc.size(); }

    [[nodiscard]] bool global() const noexcept { return m_scope == Scope::global; }

    [[nodiscard]] bool local() const noexcept { return m_scope == Scope::local; }

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

    // clang-format off

    [[nodiscard]] std::string for_log_id() const noexcept { return as_string(id()); }
    [[nodiscard]] std::string for_log_scope() const noexcept { return as_string<show::short_>(scope()); }
    [[nodiscard]] std::string for_log_type() const noexcept { return as_string<show::short_>(type()); }
    [[nodiscard]] std::string for_log_status() const noexcept { return as_string<show::short_>(status()); }
    [[nodiscard]] std::string for_log_desc() const noexcept { return short_desc(); }
    [[nodiscard]] std::string for_log() const noexcept { return std::format("{} {} {} {}", for_log_id(), for_log_type(), for_log_status(), for_log_desc()); }

    [[nodiscard]] std::string for_show_id() const noexcept { return as_string(id()); }
    [[nodiscard]] std::string for_show_scope() const noexcept { return as_string<show::long_>(scope()); }
    [[nodiscard]] std::string for_show_type() const noexcept { return as_string<show::long_>(type()); }
    [[nodiscard]] std::string for_show_status() const noexcept { return as_string<show::long_>(status()); }
    [[nodiscard]] std::string for_show_desc() const noexcept { return desc(); }
    [[nodiscard]] std::string for_show() const noexcept { return std::format("{}\n{}\n{}\n{}\n\n{}", for_show_id(), for_show_scope(), for_show_type(), for_show_status(), for_show_desc()); }

    // clang-format on

    /**
     * Spaceship operator first compares id, then type etc.
     */
    auto operator<=>(const Task& other) const noexcept = default;

private:
    ID m_id;
    Scope m_scope;
    Type m_type;
    Status m_status;
    std::string m_desc;
};

static void task_to_fstream(std::ofstream& ofs, const Task& task)
{
    ofs << as_num(task.id()) << "\n";
    ofs << as_num(task.scope()) << "\n";
    ofs << as_num(task.type()) << "\n";
    ofs << as_num(task.status()) << "\n";
    ofs << task.desc() << "\n";
}

static Task task_from_fstream(std::ifstream& ifs)
{
    ID id{};
    Scope scope{};
    Type type{};
    Status status{};

    u64 n{};
    ifs >> n, id = as<ID>(n), ifs >> std::ws;
    ifs >> n, scope = as<Scope>(n), ifs >> std::ws;
    ifs >> n, type = as<Type>(n), ifs >> std::ws;
    ifs >> n, status = as<Status>(n), ifs >> std::ws;

    std::string text{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    text.pop_back(); // remove \n

    return Task{id, scope, type, status, std::move(text)};
}

class TaskTracker {
public:
    explicit TaskTracker()
    // : m_md{read_md()}
    {
        if (!fs::exists(main_dir))
            throw std::runtime_error{"Task tracker not initialized. Please run init."};

        std::ifstream ifs{cfg_file};
        ifs >> m_user;
        ifs >> m_email;

        if (m_user.empty() || m_email.empty())
            throw std::runtime_error{"Unknown user info. Please run tt config."};
    }

    TaskTracker(const TaskTracker&) = delete;
    TaskTracker(TaskTracker&&) noexcept = delete;
    TaskTracker& operator=(const TaskTracker&) = delete;
    TaskTracker& operator=(TaskTracker&&) noexcept = delete;

    ~TaskTracker()
    {
        // try {
        //     auto md_stream{open_md_write()};
        //     md_to_fstream(md_stream, m_md);
        // }
        // catch (const std::exception& ex) {
        //     std::cout << "Failed to write new md: " << ex.what() << "\n";
        // }
    }

    static void cmd_init(const std::string& user, const std::string& email)
    {
        if (std::filesystem::exists(main_dir))
            throw std::runtime_error{"Task tracker already initialized."};

        std::filesystem::create_directory(main_dir);
        std::filesystem::create_directory(tasks_global_dir);
        std::filesystem::create_directory(tasks_global_dir / user);
        std::filesystem::create_directory(tasks_global_dir / user / refs_filename);

        // std::ofstream mdfs{open_md_write()};
        // md_to_fstream(mdfs, initial_md);

        std::ofstream{cfg_file, std::ios::trunc} << user << "\n" << email;
    }

    template<Scope scope>
    fs::path tasks_dir() const noexcept
    {
        if constexpr (scope == Scope::local)
            return tasks_global_dir / m_user;
        else
            return tasks_global_dir;
    }

    fs::path tasks_dir(Scope scope) const noexcept
    {
        return scope == Scope::local ? tasks_dir<Scope::local>() : tasks_dir<Scope::global>();
    }

    template<Scope scope>
    fs::path task_path(ID id) const noexcept
    {
        return tasks_dir<scope>() / as_string(id);
    }

    fs::path task_path(const Task& task) const noexcept
    {
        return tasks_dir(task.scope()) / as_string(task.id());
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks() const noexcept
    {
        return all_tasks_where<scope>([](const Task& t) { return true; });
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks_not_done() const noexcept
    {
        return all_tasks_where<scope>([](const Task& t) { return t.status() != Status::done; });
    }

    /**
     * Returns all tasks in descending order where tasks match predicate.
     */
    template<Scope scope, typename Pred>
    std::vector<Task> all_tasks_where(Pred pred) const
    {
        std::vector<Task> tasks;
        tasks.reserve(1024);

        for (const auto& entry : fs::directory_iterator{tasks_dir<scope>()}) {
            if (!entry.is_regular_file())
                continue;

            Task task{get_task(entry.path())};
            if (pred(task))
                tasks.emplace_back(std::move(task));
        }

        std::ranges::sort(tasks, std::ranges::greater());

        return tasks;
    }

    /**
     * Returns whether task with provided id in given scope exists.
     */
    template<Scope scope>
    bool exists(ID id) const
    {
        return std::filesystem::exists(task_path<scope>(id));
    }

    [[nodiscard]] Task get_task(const fs::path& path) const
    {
        std::ifstream ifs{path};
        if (!ifs)
            throw std::runtime_error{std::format("Task {} does not exist.", path.string())};

        return task_from_fstream(ifs);
    }

    template<Scope scope>
    [[nodiscard]] Task get_task(ID id) const
    {
        return get_task(task_path<scope>(id));
    }

    /**
     * Returns non-resolved local task with provided offset.
     * This kind of offset based task retrival is done in many commands.
     * Offset can be seen with log command with current design.
     */
    [[nodiscard]] Task get_task(Offset offset) const
    {
        std::vector<Task> tasks{all_tasks_not_done<Scope::local>()};
        if (tasks.empty())
            throw std::runtime_error{"No non-resolved tasks."};

        u64 off = as_num(offset);
        if (off >= tasks.size())
            throw std::runtime_error{"Offset to large."};

        return tasks[off];
    }

    void change_task_status(Task& task, Status status)
    {
        task.set_status(status);
        save_task(task);
    }

    template<Scope scope>
    void change_task_status(ID id, Status status)
    {
        Task task{get_task<scope>(id)};
        change_task_status(task, status);
    }

    void save_task(const Task& task)
    {
        std::ofstream ofs{task_path(task), std::ios::trunc};
        task_to_fstream(ofs, task);
    }

    void new_task(ID id, Scope scope, Type type, std::string desc)
    {
        save_task(Task{id, scope, type, Status::not_started, std::move(desc)});
    }

    // void reopen_task(ID id) { change_task_status(id, Status::not_started); }

    // void start_task(ID id) { change_task_status(id, Status::in_progress); }

    void resolve_task(Task& task) { change_task_status(task, Status::done); }

    template<Scope scope>
    void resolve_task(ID id)
    {
        change_task_status<scope>(id, Status::done);
    }

    void roll(Task& task)
    {
        task.roll_status();
        save_task(task);
    }

    template<Scope scope>
    void roll(ID id)
    {
        Task task{get_task<scope>(id)};
        roll(task);
    }

    void rollback(Task& task)
    {
        task.rollback_status();
        save_task(task);
    }

    template<Scope scope>
    void rollback(ID id)
    {
        Task task{get_task<scope>(id)};
        rollback(task);
    }

    ID next_id() { return as<ID>(now_sys_ns()); }

private:
    // static std::ifstream open_md_read()
    // {
    //     if (!std::filesystem::exists(main_dir)) {
    //         if constexpr (!dev)
    //             throw std::runtime_error{"Task tracker not initialized. Please run init."};

    //         cmd_init();
    //     }

    //     return std::ifstream{md_file};
    // }

    // static std::ofstream open_md_write() { return std::ofstream{md_file, std::ios::trunc}; }

    // static MD read_md()
    // {
    //     std::ifstream md_stream{open_md_read()};
    //     return md_from_fstream(md_stream);
    // }

    /**
     * Task id is task creation time (system time point) in nanoseconds.
     */

private: // NOLINT
    MD m_md;

    std::string m_user;
    std::string m_email;
};

// NOLINTEND(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-convert-member-functions-to-static)

#endif // TASK_HPP
