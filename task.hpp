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

#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "types.hpp"

// NOLINTBEGIN(readability-implicit-bool-conversion)

constexpr bool dev = true;

namespace fs = std::filesystem;

constexpr char path_sep = fs::path::preferred_separator;
const inline std::string path_sep_str = std::string{path_sep}; // NOLINT

// clang-format off
inline const std::string main_dir = ".tt";                          /* .tt/       */ // NOLINT
inline const std::string tasks_dir = main_dir + path_sep + "tasks"; /* .tt/tasks/ */ // NOLINT
inline const std::string md_file = main_dir + path_sep + "md";      /* .tt/md     */ // NOLINT
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
enum class Status : u8 { not_started = 0, in_prograss = 1, done = 2 };

template<class T>
constexpr T as(u64 value)
{
    if constexpr (std::is_same_v<T, ID>) {
        return ID(value);
    }
    else if constexpr (std::is_same_v<T, Type>) {
        if (value > u64(Type::feature))
            throw std::runtime_error{"Invalid type."};

        return Type(value);
    }
    else if constexpr (std::is_same_v<T, Status>) {
        if (value > u64(Status::done))
            throw std::runtime_error{"Invalid type."};

        return Status(value);
    }
    else
        static_assert(!"Invalid type.");
}

template<class T>
u64 as_num(T v)
{
    return static_cast<u64>(v);
}

inline std::string as_string(ID id)
{
    return std::to_string(as_num(id));
}

inline std::string as_string(Type t)
{
    switch (t) { // clang-format off
    case Type::task:    return "task";
    case Type::bug:     return "bug";
    case Type::feature: return "feature";
    default:            return "invalid";
    } // clang-format on
}

inline std::string as_string(Status s)
{
    switch (s) { // clang-format off
    case Status::not_started: return "not started";
    case Status::in_prograss: return "in progress";
    case Status::done:        return "done";
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

    [[nodiscard]] std::string short_desc() const noexcept
    {
        size_t start = 0;
        while (start < m_desc.size() && std::isspace(m_desc[start]))
            ++start;

        return m_desc.substr(start, m_desc.find('\n', start) - start);
    }

    [[nodiscard]] std::string for_log() const noexcept
    {
        return std::format("ID {:<5} T {:<7} S {:<11} -> {}", as_string(id()), as_string(type()),
                           as_string(status()), short_desc());
    }

    [[nodiscard]] std::string for_show() const noexcept
    {
        return std::format("ID {}\nT {}\nS {}\n\n{}", as_string(id()), as_string(type()),
                           as_string(status()), desc());
    }

private:
    ID m_id;
    Type m_type;
    Status m_status;
    std::string m_desc;
};

static void task_to_fstream(std::ofstream& ofs, const Task& task)
{
    ofs << "ID ";
    ofs << as_num(task.id()) << "\n";
    ofs << "T ";
    ofs << as_num(task.type()) << "\n";
    ofs << "S ";
    ofs << as_num(task.status()) << "\n";
    ofs << "\n";
    ofs << task.desc() << "\n";
}

static Task task_from_fstream(std::ifstream& ifs)
{
    auto check = [](bool cond) {
        if (!cond)
            throw std::runtime_error{"Bad task format."};
    };

    ID id{};
    Type type{};
    Status status{};

    u64 n{};
    std::string token;

    ifs >> token;
    check(token == "ID");
    ifs >> n, id = as<ID>(n);

    ifs >> token;
    check(token == "T");
    ifs >> n, type = as<Type>(n);

    ifs >> token;
    check(token == "S");
    ifs >> n, status = as<Status>(n);

    ifs.get(), ifs.get(); // skip \n\n
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
        std::ofstream mdfs{md_file};
        md_to_fstream(mdfs, initial_md);
    }

    static std::string new_task_path(ID id) { return tasks_dir + path_sep + as_string(id); }

    static std::vector<Task> all_tasks()
    {
        std::vector<Task> tasks;
        for (const auto& entry : fs::recursive_directory_iterator{tasks_dir}) {
            // log(entry.path());
            std::ifstream is{entry.path()};
            tasks.emplace_back(task_from_fstream(is));
        }

        return tasks;
    }

    static Task get_task(u64 id)
    {
        std::ifstream ifs{tasks_dir + path_sep + std::to_string(id)};
        if (!ifs)
            throw std::runtime_error{std::format("Task {} does not exist.", id)};

        return task_from_fstream(ifs);
    }

    void new_task(Type type, std::string desc)
    {
        ID id = next_id();
        std::ofstream file{new_task_path(id)};
        task_to_fstream(file, Task{id, type, Status::not_started, std::move(desc)});
    }

    void new_task(std::string desc) { new_task(Type::task, std::move(desc)); }

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

    static std::ofstream open_md_write()
    {
        return std::ofstream{md_file, std::ios::out | std::ios::trunc};
    }

    static MD read_md()
    {
        std::ifstream md_stream{open_md_read()};
        return md_from_fstream(md_stream);
    }

    ID next_id() { return m_md.next_id(); }

private: // NOLINT
    MD m_md;
};

// NOLINTEND(readability-implicit-bool-conversion)

#endif // TASK_HPP
