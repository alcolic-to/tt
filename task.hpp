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
#include <map>
#include <optional>
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
    return static_cast<u64>(
        std::chrono::duration_cast<nanoseconds>(now_sys().time_since_epoch()).count());
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

inline std::string default_worker()
{
    return "none";
}

/* clang-format off */
inline const fs::path main_dir = ".tt";                      /* .tt/         */
inline const fs::path tasks_global_dir = main_dir / "tasks"; /* .tt/tasks/   */
inline const fs::path md_file = main_dir / "md";             /* .tt/md       */
inline const fs::path msg_file = main_dir / "desc_msg";      /* .tt/desc_msg */

inline const fs::path cfg_file = home_dir() / ".ttconfig";   /* ~/.ttconfig  */

inline const fs::path tasks_idx_filename = "idx";            /* located in .tt/tasks/idx and .tt/tasks/<user>/idx */ 
inline const fs::path tasks_data_filename = "data";          /* located in .tt/tasks/data and .tt/tasks/<user>/data */
inline const fs::path refs_filename = "refs";                /* refs filename, located in .tt/tasks/<user>/refs */

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

/**
 * I don't know how this works (it looks that it doesn't work) and I don't want to know.
 * Setting and reading all positions at once (because it might work that way).
 */

u64 fspos(std::fstream& fs)
{
    u64 g = fs.tellg();
    u64 p = fs.tellp();
    if (g != p)
        throw std::runtime_error{"Invalid stream"};

    return p;
}

void set_fspos(std::fstream& fs, u64 pos, std::ios::seekdir seek = std::ios::beg)
{
    if (fs.bad())
        throw std::runtime_error{"Invalid stream"};

    if (fs.eof())
        fs.clear();

    fs.seekg(pos, seek);
    fs.seekp(pos, seek);
}

void set_fspos(std::fstream& fs, std::ios::seekdir seek = std::ios::beg)
{
    if (fs.bad())
        throw std::runtime_error{"Invalid stream"};

    if (fs.eof())
        fs.clear();

    fs.seekg(0, seek);
    fs.seekp(0, seek);
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

class Config {
public:
    Config(std::string username, std::string email)
        : m_username{!username.empty() ? std::move(username) : default_username()}
        , m_email{!email.empty() ? std::move(email) : default_email()}
    {
    }

    const std::string& username() const noexcept { return m_username; }

    const std::string& email() const noexcept { return m_email; }

    void set_username(std::string username) { m_username = std::move(username); }

    void set_email(std::string email) { m_email = std::move(email); }

private:
    std::string m_username;
    std::string m_email;
};

inline void config_to_file(const fs::path& fpath, const Config& config)
{
    std::ofstream ofs{fpath, std::ios::trunc};
    ofs << config.username() << "\n" << config.email();
}

inline Config config_from_file(const fs::path& fpath)
{
    if (!fs::exists(fpath))
        throw std::runtime_error{
            "TT config not present. Please run tt config (-n <username> -m <email>)."};

    std::string username, email;

    std::ifstream ifs{fpath};
    ifs >> username >> email;

    return Config{std::move(username), std::move(email)};
}

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
    friend inline std::fstream& operator>>(std::fstream& ifs, Task& task);

public:
    Task() = default;

    Task(ID id, Scope scope, Type type, Status status, std::string author, std::string worker,
         std::string desc)
        : m_id{id}
        , m_scope{scope}
        , m_type{type}
        , m_status{status}
        , m_author{std::move(author)}
        , m_worker{std::move(worker)}
        , m_desc{std::move(desc)}
    {
    }

    [[nodiscard]] ID id() const noexcept { return m_id; }

    [[nodiscard]] Scope scope() const noexcept { return m_scope; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& author() const noexcept { return m_author; }

    [[nodiscard]] const std::string& worker() const noexcept { return m_worker; }

    [[nodiscard]] const std::string& desc() const noexcept { return m_desc; }

    [[nodiscard]] usize desc_size() const noexcept { return m_desc.size(); }

    [[nodiscard]] bool global() const noexcept { return m_scope == Scope::global; }

    [[nodiscard]] bool local() const noexcept { return !global(); }

    void set_type(Type t) { m_type = t; }

    void set_status(Status s) { m_status = s; }

    void set_worker(std::string worker) { m_worker = std::move(worker); }

    void unset_worker() { m_worker = default_worker(); }

    void set_desc(std::string desc) { m_desc = std::move(desc); }

    void roll_status()
    {
        if (status() == Status::done)
            throw std::runtime_error{"Cannot roll task with status done."};

        m_status = as<Status>(as_num(m_status) + 1);
    }

    void rollb_status()
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
    [[nodiscard]] std::string for_show_author() const noexcept { return author(); }
    [[nodiscard]] std::string for_show_worker() const noexcept { return worker(); }
    [[nodiscard]] std::string for_show_desc() const noexcept { return desc(); }
    [[nodiscard]] std::string for_show() const noexcept { return std::format("{}\n{}\n{}\n{}\n{}\n{}\n\n{}", for_show_id(), for_show_scope(), for_show_type(), for_show_status(), for_show_author(), for_show_worker(), for_show_desc()); }

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
    std::string m_author;
    std::string m_worker;
    std::string m_desc;
};

UID::UID(const Task& task) : m_scope{task.scope()}, m_id{task.id()} {}

inline void task_to_fstream(std::ofstream& ofs, const Task& task)
{
    ofs << as_num(task.id()) << "\n";
    ofs << as_num(task.scope()) << "\n";
    ofs << as_num(task.type()) << "\n";
    ofs << as_num(task.status()) << "\n";

    ofs << as_num(task.author().size()) << "\n";
    ofs << task.author() << "\n";

    ofs << as_num(task.worker().size()) << "\n";
    ofs << task.worker() << "\n";

    ofs << as_num(task.desc().size()) << "\n";
    ofs << task.desc() << "\n";
}

inline std::fstream& operator>>(std::fstream& fs, Task& task)
{
    u64 n{};

    fs >> n;
    if (!fs)
        return fs;

    task.m_id = as<ID>(n), fs >> std::ws;
    fs >> n, task.m_scope = as<Scope>(n), fs >> std::ws;
    fs >> n, task.m_type = as<Type>(n), fs >> std::ws;
    fs >> n, task.m_status = as<Status>(n), fs >> std::ws;

    fs >> n, fs >> std::ws;
    task.m_author.resize(n);
    fs.read(task.m_author.data(), n), fs >> std::ws;

    fs >> n, fs >> std::ws;
    task.m_worker.resize(n);
    fs.read(task.m_worker.data(), n), fs >> std::ws;

    fs >> n, fs >> std::ws;
    task.m_desc.resize(n);
    fs.read(task.m_desc.data(), n), fs >> std::ws;

    return fs;
}

inline std::fstream& operator<<(std::fstream& fs, const Task& task)
{
    fs << as_num(task.id()) << "\n";
    fs << as_num(task.scope()) << "\n";
    fs << as_num(task.type()) << "\n";
    fs << as_num(task.status()) << "\n";

    fs << as_num(task.author().size()) << "\n";
    fs << task.author() << "\n";

    fs << as_num(task.worker().size()) << "\n";
    fs << task.worker() << "\n";

    fs << as_num(task.desc().size()) << "\n";
    fs << task.desc() << "\n";

    return fs;
}

/**
 * Task index holding task id and task offset in data file.
 */
class Idx {
public:
    Idx(ID id, u64 offset) : m_id{id}, m_offset{offset} {}

    Idx() = default;

    friend inline std::fstream& operator>>(std::fstream& fs, Idx& idx);
    friend inline std::fstream& operator<<(std::fstream& fs, const Idx& idx);

    ID id() const noexcept { return m_id; }

    u64 offset() const noexcept { return m_offset; }

    void set_id(ID id) noexcept { m_id = id; }

    void set_offset(u64 offset) noexcept { m_offset = offset; }

    bool operator==(const Idx& other) const noexcept { return m_id == other.m_id; }

    bool operator!=(const Idx& other) const noexcept { return !(*this == other); }

private:
    ID m_id;
    u64 m_offset;
};

inline void idx_to_fstream(std::ofstream& ofs, const Idx& idx)
{
    ofs << as_num(idx.id()) << " " << idx.offset() << "\n";
}

inline std::fstream& operator>>(std::fstream& fs, Idx& idx)
{
    u64 n{};

    fs >> n;
    if (!fs)
        return fs;

    fs >> std::ws;

    idx.m_id = as<ID>(n);
    fs >> idx.m_offset >> std::ws;

    return fs;
}

inline std::fstream& operator<<(std::fstream& fs, const Idx& idx)
{
    fs << as_num(idx.m_id) << " " << idx.m_offset << "\n";
    return fs;
}

/**
 * Task cache entry.
 * It holds task, if task is loaded from file, nullopt otherwise, and file offset where task is
 * located.
 */
struct TaskCacheEntry {
    TaskCacheEntry() = default;

    TaskCacheEntry(u64 offset) : m_offset{offset} {}

    TaskCacheEntry(Task task, u64 offset) : m_task{std::move(task)}, m_offset{offset} {}

    std::optional<Task> m_task{std::nullopt};
    u64 m_offset{0};
};

class Tasks {
    friend class TaskTracker;

public:
    Tasks() = default;

    Tasks(const fs::path& idx_path, const fs::path& data_path)
        : m_idx_fs{idx_path, std::ios::in | std::ios::out}
        , m_data_fs{data_path, std::ios::in | std::ios::out}
    {
        Idx idx;
        set_fspos(m_idx_fs, std::ios::beg);
        while (m_idx_fs >> idx) {
            m_idxs.emplace_back(idx);
            m_cache.emplace(idx.id(), idx.offset());
        }
    }

    /**
     * Returns task with provided id.
     * If task is not in the cache, it is read from file.
     */
    const Task& get_task(ID id)
    {
        TaskCacheEntry& entry{get_cache_entry(id)};
        if (!entry.m_task)
            entry.m_task = task_from_offset(entry.m_offset);

        return entry.m_task.value();
    }

    void new_task(Task task)
    {
        if (m_cache.contains(task.id()))
            throw std::runtime_error{std::format("Task {} already exist.", as_num(task.id()))};

        set_fspos(m_data_fs, std::ios::end);
        u64 offset = fspos(m_data_fs);
        m_data_fs << task;

        set_fspos(m_idx_fs, std::ios::end);
        m_idx_fs << Idx{task.id(), offset};

        m_cache.emplace(task.id(), TaskCacheEntry{std::move(task), offset});
    }

    /**
     * Iterates over cache entries (sorted by ID) and loads tasks from disk if they are not in
     * cache, after which pred is applied.
     */
    template<class Pred>
    void for_each_sorted(Pred&& pred)
    {
        for (auto& [id, entry] : m_cache) {
            if (!entry.m_task)
                entry.m_task = task_from_offset(entry.m_offset);

            pred(entry.m_task.value());
        }
    }

    /**
     * Iterates over index entries (sorted by file offset) and loads tasks from disk if they are not
     * in cache, after which pred is applied.
     */
    template<class Pred>
    void for_each_unsorted(Pred&& pred)
    {
        for (const Idx& idx : m_idxs)
            pred(get_task(idx.id()));
    }

    template<bool sorted, class Pred>
    void for_each(Pred&& pred)
    {
        if constexpr (sorted)
            return for_each_sorted<Pred>(std::forward<Pred>(pred));

        return for_each_unsorted<Pred>(std::forward<Pred>(pred));
    }

    void bulk_load()
    {
        for_each_unsorted([](const Task& task) {});
    }

private:
    Task task_from_offset(u64 offset)
    {
        Task task;

        set_fspos(m_data_fs, offset);
        m_data_fs >> task;
        if (!m_data_fs)
            throw std::runtime_error{"Invalid task read."};

        return task;
    }

    TaskCacheEntry& get_cache_entry(ID id)
    {
        if (!m_cache.contains(id))
            throw std::runtime_error{std::format("Fatal: ID {} not in cache.", as_num(id))};

        return m_cache[id];
    }

private: /* members */
    std::fstream m_idx_fs;
    std::fstream m_data_fs;

    std::vector<Idx> m_idxs;

    /* clang-format off */ struct Cmp { bool operator()(ID id1, ID id2) const { return as_num(id1) > as_num(id2); } }; /* clang-format on */

    /*
     * Tasks cache.
     * Cache is ordered - newest tasks first.
     */
    std::map<ID, TaskCacheEntry, Cmp> m_cache;
};

class TaskTracker {
public:
    explicit TaskTracker()
        //  : m_md{read_md()}
        : m_config{config_from_file(cfg_file)}
        , m_global_tasks{tasks_global_dir / tasks_idx_filename,
                         tasks_global_dir / tasks_data_filename}
        , m_local_tasks{tasks_global_dir / username() / tasks_idx_filename,
                        tasks_global_dir / username() / tasks_data_filename}
    {
        if (!fs::exists(main_dir))
            throw std::runtime_error{"TT not initialized. Please run init."};

        if (!fs::exists(tasks_global_dir / username()))
            throw std::runtime_error{
                std::format("User {} does not exist. Please run tt register.", username())};

        // m_global_tasks = tasks_from_file(tasks_global_dir / tasks_data_filename);
        // m_local_tasks = tasks_from_file(tasks_global_dir / username() / tasks_data_filename);
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

    const std::string& username() const noexcept { return m_config.username(); }

    const std::string& email() const noexcept { return m_config.email(); }

    static Config cmd_config(const std::string& cmd_user, const std::string& cmd_email)
    {
        /* First time config init. */
        if (!fs::exists(cfg_file)) {
            Config config{cmd_user, cmd_email};
            config_to_file(cfg_file, config);
            return config;
        }

        Config config{config_from_file(cfg_file)};

        if (!cmd_user.empty())
            config.set_username(cmd_user);

        if (!cmd_email.empty())
            config.set_email(cmd_email);

        config_to_file(cfg_file, config);
        return config;
    }

    static void cmd_init()
    {
        Config config{config_from_file(cfg_file)};

        if (fs::exists(main_dir))
            throw std::runtime_error{
                "TT already initialized. If you are new user, please run tt register."};

        fs::create_directory(main_dir);
        fs::create_directory(tasks_global_dir);
        std::ofstream{tasks_global_dir / tasks_idx_filename, std::ios::app};
        std::ofstream{tasks_global_dir / tasks_data_filename, std::ios::app};

        fs::create_directory(tasks_global_dir / config.username());
        std::ofstream{tasks_global_dir / config.username() / tasks_idx_filename, std::ios::app};
        std::ofstream{tasks_global_dir / config.username() / tasks_data_filename, std::ios::app};
        std::ofstream{tasks_global_dir / config.username() / refs_filename, std::ios::app};

        // std::ofstream mdfs{open_md_write()};
        // md_to_fstream(mdfs, initial_md);
    }

    static void cmd_register(std::string username, std::string email)
    {
        Config config{!username.empty() ? Config{std::move(username), std::move(email)} :
                                          config_from_file(cfg_file)};

        if (!fs::exists(main_dir))
            throw std::runtime_error{"TT not initialized. Please run init."};

        if (fs::exists(tasks_global_dir / config.username()))
            throw std::runtime_error{std::format("User {} already registered.", config.username())};

        fs::create_directory(tasks_global_dir / config.username());
        std::ofstream{tasks_global_dir / config.username() / tasks_idx_filename, std::ios::app};
        std::ofstream{tasks_global_dir / config.username() / tasks_data_filename, std::ios::app};
        std::ofstream{tasks_global_dir / config.username() / refs_filename, std::ios::app};
    }

    static std::vector<Config> users()
    {
        std::vector<Config> users;
        users.reserve(1024);

        for (const auto& entry : fs::directory_iterator{tasks_global_dir})
            if (entry.is_directory())
                users.emplace_back(entry.path().filename(), "");

        return users;
    }

    template<Scope scope>
    fs::path tasks_dir() const noexcept
    {
        if constexpr (scope == Scope::local)
            return tasks_global_dir / username();
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

    fs::path task_idx_path(const Task& task) const noexcept
    {
        return tasks_dir(task.scope()) / tasks_idx_filename;
    }

    fs::path task_idx_path(UID uid) const noexcept
    {
        return tasks_dir(uid.scope()) / tasks_idx_filename;
    }

    fs::path task_data_path(const Task& task) const noexcept
    {
        return tasks_dir(task.scope()) / tasks_data_filename;
    }

    fs::path task_data_path(UID uid) const noexcept
    {
        return tasks_dir(uid.scope()) / tasks_data_filename;
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks()
    {
        return all_tasks_where<scope>([](const Task& t) { return true; });
    }

    /**
     * Returns all tasks in descending order.
     */
    template<Scope scope>
    std::vector<Task> all_tasks_not_done()
    {
        return all_tasks_where<scope>([](const Task& t) { return t.status() != Status::done; });
    }

    /* clang-format off */
    template<Scope scope> const Tasks& tasks() const noexcept { if constexpr (scope == Scope::global) return m_global_tasks; return m_local_tasks; }
    template<Scope scope> Tasks& tasks() noexcept { if constexpr (scope == Scope::global) return m_global_tasks; return m_local_tasks; }
    const Tasks& tasks(Scope scope) const noexcept { if (scope == Scope::global) return tasks<Scope::global>(); return tasks<Scope::local>(); }
    Tasks& tasks(Scope scope) noexcept { if (scope == Scope::global) return tasks<Scope::global>(); return tasks<Scope::local>(); }

    /* clang-format on */

    /**
     * Returns all tasks in descending order where tasks match predicate.
     */
    template<Scope scope, typename Pred>
    std::vector<Task> all_tasks_where(Pred pred)
    {
        std::vector<Task> res;
        res.reserve(1024);

        if constexpr (scope == Scope::local) {
            for (const UID& uid : get_task_refs()) {
                const Task& task{get_task(uid)};
                if (pred(task))
                    res.emplace_back(task);
            }
        }

        tasks<scope>().for_each_unsorted([&](const Task& task) {
            if (pred(task))
                res.emplace_back(task);
        });

        std::ranges::sort(res, std::ranges::greater());

        return res;
    }

    [[nodiscard]] const Task& get_task(UID uid) { return tasks(uid.scope()).get_task(uid.id()); }

    /**
     * Returns non-resolved local task with provided VID.
     * VID can be seen with log command with current design.
     * This kind of VID based task retrival is done in many commands.
     */
    [[nodiscard]] Task get_task(VID vid)
    {
        std::vector<Task> tasks{all_tasks_not_done<Scope::local>()};
        if (tasks.empty())
            throw std::runtime_error{"No non-resolved tasks."};

        u64 vid_num = as_num(vid);
        if (vid_num >= tasks.size())
            throw std::runtime_error{"Invalid VID."};

        return tasks[vid_num];
    }

    void rm_task(const Task& task) { fs::remove(task_path(task)); }

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

    void new_task(Scope scope, Type type, std::string worker, std::string desc)
    {
        if (worker.empty())
            worker = scope == Scope::local ? username() : default_worker();

        Task task{next_id(),         scope,          type, Status::not_started, username(),
                  std::move(worker), std::move(desc)};

        tasks(scope).new_task(std::move(task));
    }

    void resolve_task(Task& task) { change_task_status(task, Status::done); }

    void resolve_task(UID uid) { change_task_status(uid, Status::done); }

    void roll(Task& task)
    {
        task.roll_status();
        save_task(task);
    }

    void rollb(Task& task)
    {
        task.rollb_status();
        save_task(task);
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
            throw std::runtime_error{std::format("Task already assigned to {}.", username())};

        std::ofstream ofs{task_refs_ofstream()};
        uid_to_fstream(ofs, task.uid());
    }

    template<bool throws = true>
    void remove_task_ref(const Task& task)
    {
        std::vector<UID> refs{get_task_refs()};
        const UID uid{task.uid()};

        const auto it = std::find(refs.begin(), refs.end(), uid);
        if (it == refs.end()) {
            if constexpr (!throws)
                return;

            throw std::runtime_error{std::format("Task not assigned to {}.", username())};
        }

        refs.erase(it);

        std::ofstream ofs{task_refs_ofstream(true)};
        for (const auto& ref : refs)
            uid_to_fstream(ofs, ref);
    }

    std::ofstream task_refs_ofstream(bool trunc = false) const
    {
        return std::ofstream{tasks_global_dir / username() / refs_filename,
                             trunc ? std::ios::trunc : std::ios::app};
    }

    std::ifstream task_refs_ifstream() const
    {
        return std::ifstream{tasks_global_dir / username() / refs_filename};
    }

    /**
     * Switches context to other user (provided config). This is created only for code
     * reusability.
     */
    void switch_context(Config config)
    {
        m_config = config;

        if (!fs::exists(tasks_global_dir / username()))
            throw std::runtime_error{
                std::format("User {} does not exist. Please run tt register.", username())};
    }

private:
    ID next_id() { return as<ID>(now_sys_ns()); }

    // static std::ifstream open_md_read()
    // {
    //     if (!fs::exists(main_dir)) {
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
    Config m_config;

    // Tasks m_global_tasks;
    // Tasks m_local_tasks;

    Tasks m_global_tasks;
    Tasks m_local_tasks;
};

// NOLINTEND(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-convert-member-functions-to-static)

#endif // TASK_HPP
