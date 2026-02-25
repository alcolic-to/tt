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
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
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

bool spaces_only(const std::string& s) noexcept
{
    return std::ranges::all_of(s, [](u8 c) { return std::isspace(c); });
}

template<class StringLike>
bool digits_only(const StringLike& s) noexcept
{
    return std::ranges::all_of(s, [](u8 c) { return std::isdigit(c); });
}

enum class ID : u64 {};
enum class Scope : u8 { global = 0, local = 1 };
enum class Type : u8 { task = 0, bug = 1, feature = 2 };
enum class Status : u8 { not_started = 0, in_progress = 1, done = 2 };

enum class VID : u64 {};

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
    else if constexpr (std::is_same_v<T, VID>) {
        return VID(value);
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

// static constexpr MD initial_md = {
//     .m_id = as<ID>(1),
// };

// inline MD md_from_fstream(std::ifstream& ifs)
// {
//     u64 n{};
//     ifs >> n;

//     return MD{.m_id = as<ID>(n)};
// }

// inline void md_to_fstream(std::ofstream& ofs, const MD& md)
// {
//     ofs << as_num(md.m_id);
// }

/**
 * Global identifier for the task which is also task filename.
 */
class UID {
public:
    UID(const fs::path& path)
    {
        u8 scope;
        u64 id;

        std::stringstream ss{path.filename()};
        ss >> scope;

        if (scope == 'G')
            m_scope = Scope::global;
        else if (scope == 'L')
            m_scope = Scope::local;
        else
            throw std::runtime_error{"Invalid task scope."};

        ss >> id, m_id = as<ID>(id);
    }

    UID(const class Task& task);

    UID() = default;

    fs::path as_filename() const { return (global() ? "G" : "L") + ::as_string(m_id); }

    Scope scope() const noexcept { return m_scope; }

    bool global() const noexcept { return m_scope == Scope::global; }

    bool local() const noexcept { return !global(); }

    ID id() const noexcept { return m_id; }

    void set_scope(Scope scope) noexcept { m_scope = scope; }

    void set_id(ID id) noexcept { m_id = id; }

    static bool valid_uid(const std::string& uid) noexcept
    {
        return uid.size() > 1 && (uid.front() == 'G' || uid.front() == 'L') &&
               digits_only(std::string_view{uid.c_str() + 1, uid.size() - 1});
    }

    bool valid() const noexcept
    {
        return (m_scope == Scope::local || m_scope == Scope::global) && as_num(m_id) > 0;
    }

    bool operator==(const UID& other) const noexcept
    {
        return m_scope == other.m_scope && m_id == other.m_id;
    }

    bool operator!=(const UID& other) const noexcept { return !(*this == other); }

private:
    Scope m_scope;
    ID m_id;
};

inline void uid_to_fstream(std::ofstream& ofs, const UID& uid)
{
    ofs << (uid.global() ? 'G' : 'L') << as_num(uid.id()) << "\n";
}

inline std::ifstream& operator>>(std::ifstream& ifs, UID& uid)
{
    u8 scope;
    u64 id;

    ifs >> scope;
    if (!ifs)
        return ifs;

    if (scope != 'G' && scope != 'L')
        throw std::runtime_error{"Invalid task scope."};

    uid.set_scope(scope == 'G' ? Scope::global : Scope::local);

    ifs >> id;
    if (!ifs)
        return ifs;

    uid.set_id(as<ID>(id));
    return ifs;
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

    [[nodiscard]] bool local() const noexcept { return !global(); }

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

    UID uid() const noexcept { return UID{*this}; }

    [[nodiscard]] fs::path as_filename() const { return uid().as_filename(); }

    // clang-format off

    [[nodiscard]] std::string for_log_id() const noexcept { return as_string(id()); }
    [[nodiscard]] std::string for_log_scope() const noexcept { return as_string<show::short_>(scope()); }
    [[nodiscard]] std::string for_log_type() const noexcept { return as_string<show::short_>(type()); }
    [[nodiscard]] std::string for_log_status() const noexcept { return as_string<show::short_>(status()); }
    [[nodiscard]] std::string for_log_desc() const noexcept { return short_desc(); }
    [[nodiscard]] std::string for_log() const noexcept { return std::format("{}{} {} {} {}", for_log_scope(), for_log_id(), for_log_type(), for_log_status(), for_log_desc()); }

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

UID::UID(const Task& task) : m_scope{task.scope()}, m_id{task.id()} {}

inline void task_to_fstream(std::ofstream& ofs, const Task& task)
{
    ofs << as_num(task.id()) << "\n";
    ofs << as_num(task.scope()) << "\n";
    ofs << as_num(task.type()) << "\n";
    ofs << as_num(task.status()) << "\n";
    ofs << task.desc() << "\n";
}

inline Task task_from_fstream(std::ifstream& ifs)
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
        std::ofstream{tasks_global_dir / user / refs_filename};

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

    fs::path task_path(UID uid) const noexcept
    {
        return tasks_dir(uid.scope()) / uid.as_filename();
    }

    fs::path task_path(const Task& task) const noexcept
    {
        return tasks_dir(task.scope()) / task.as_filename();
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks() const
    {
        return all_tasks_where<scope>([](const Task& t) { return true; });
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks_not_done() const
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

        if constexpr (scope == Scope::local) {
            for (const UID& uid : get_task_refs()) {
                Task task{get_task(uid)};
                if (pred(task))
                    tasks.emplace_back(std::move(task));
            }
        }

        for (const auto& entry : fs::directory_iterator{tasks_dir<scope>()}) {
            if (entry.is_directory() || entry.path().filename() == refs_filename)
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
    bool exists(UID uid) const
    {
        return std::filesystem::exists(task_path(uid));
    }

    [[nodiscard]] Task get_task(const fs::path& path) const
    {
        std::ifstream ifs{path};
        if (!ifs)
            throw std::runtime_error{
                std::format("Task {} does not exist.", path.filename().string())};

        return task_from_fstream(ifs);
    }

    [[nodiscard]] Task get_task(UID uid) const { return get_task(task_path(uid)); }

    /**
     * Returns non-resolved local task with provided VID.
     * VID can be seen with log command with current design.
     * This kind of VID based task retrival is done in many commands.
     */
    [[nodiscard]] Task get_task(VID vid) const
    {
        std::vector<Task> tasks{all_tasks_not_done<Scope::local>()};
        if (tasks.empty())
            throw std::runtime_error{"No non-resolved tasks."};

        u64 vid_num = as_num(vid);
        if (vid_num >= tasks.size())
            throw std::runtime_error{"Invalid VID."};

        return tasks[vid_num];
    }

    void change_task_status(Task& task, Status status)
    {
        task.set_status(status);
        save_task(task);
    }

    void change_task_status(UID uid, Status status)
    {
        Task task{get_task(uid)};
        change_task_status(task, status);
    }

    void save_task(const Task& task)
    {
        std::ofstream ofs{task_path(task), std::ios::trunc};
        task_to_fstream(ofs, task);
    }

    void new_task(Scope scope, Type type, std::string desc)
    {
        save_task(Task{next_id(), scope, type, Status::not_started, std::move(desc)});
    }

    void resolve_task(Task& task) { change_task_status(task, Status::done); }

    void resolve_task(UID uid) { change_task_status(uid, Status::done); }

    void roll(Task& task)
    {
        task.roll_status();
        save_task(task);
    }

    void roll(UID uid)
    {
        Task task{get_task(uid)};
        roll(task);
    }

    void rollback(Task& task)
    {
        task.rollback_status();
        save_task(task);
    }

    void rollback(UID uid)
    {
        Task task{get_task(uid)};
        rollback(task);
    }

    std::vector<UID> get_task_refs() const
    {
        std::vector<UID> refs;
        refs.reserve(1024);

        std::ifstream ifs{task_refs_ifstream()};
        UID uid;
        while (ifs >> uid)
            refs.emplace_back(std::move(uid));

        return refs;
    }

    bool task_refs_contains(const std::vector<UID>& refs, const Task& task) const
    {
        const UID uid{task.uid()};
        return std::find(refs.begin(), refs.end(), uid) != refs.end();
    }

    void add_task_ref(const Task& task)
    {
        std::vector<UID> task_refs{get_task_refs()};
        if (task_refs_contains(task_refs, task))
            throw std::runtime_error{"Task already assigned to user."};

        std::ofstream ofs{task_refs_ofstream()};
        uid_to_fstream(ofs, task.uid());
    }

    std::ofstream task_refs_ofstream() const
    {
        return std::ofstream{tasks_global_dir / m_user / refs_filename, std::ios::app};
    }

    std::ifstream task_refs_ifstream() const
    {
        return std::ifstream{tasks_global_dir / m_user / refs_filename};
    }

private:
    ID next_id() { return as<ID>(now_sys_ns()); }

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
