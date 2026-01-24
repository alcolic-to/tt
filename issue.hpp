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

#ifndef ISSUE_HPP
#define ISSUE_HPP

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
inline const std::string main_dir = ".it";                            /* .it/        */ // NOLINT
inline const std::string issues_dir = main_dir + path_sep + "issues"; /* .it/issues/ */ // NOLINT
inline const std::string md_file = main_dir + path_sep + "md";        /* .it/md      */ // NOLINT
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

enum class Type : u8 { task, bug, feature };
enum class Status : u8 { not_started, in_prograss, done };

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

template<class T>
inline T as(u64 value)
{
    if constexpr (std::is_same_v<T, Type>) {
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

inline std::ofstream& operator<<(std::ofstream& ofs, Type t)
{
    ofs << as_num(t);
    return ofs;
}

inline std::stringstream& operator<<(std::stringstream& os, Type t)
{
    os << as_string(t);
    return os;
}

inline std::ifstream& operator>>(std::ifstream& os, Type& type)
{
    u64 read_t = 0;
    os >> read_t;
    type = as<Type>(read_t);

    return os;
}

inline std::ofstream& operator<<(std::ofstream& os, Status s)
{
    os << as_num(s);
    return os;
}

inline std::stringstream& operator<<(std::stringstream& os, Status s)
{
    os << as_string(s);
    return os;
}

inline std::ifstream& operator>>(std::ifstream& os, Status& s)
{
    u64 read_s = 0;
    os >> read_s;
    s = as<Status>(read_s);

    return os;
}

struct MD {
    u64 m_id;
};

static const MD initial_md = {
    .m_id = 1,
};

inline std::ofstream& operator<<(std::ofstream& os, const MD& md)
{
    os << as_num(md.m_id);
    return os;
}

inline std::ifstream& operator>>(std::ifstream& os, MD& md)
{
    os >> md.m_id;
    return os;
}

class Issue {
public:
    Issue(u64 id, Type type, Status status, std::string desc)
        : m_id{id}
        , m_type{type}
        , m_status{status}
        , m_desc{std::move(desc)}
    {
    }

    Issue(u64 id, auto type, auto status, std::string desc) // NOLINT
        : Issue(id, static_cast<Type>(type), static_cast<Status>(status), std::move(desc))
    {
    }

    [[nodiscard]] u64 id() const noexcept { return m_id; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& desc() const noexcept { return m_desc; }

    [[nodiscard]] std::string short_decs() const noexcept
    {
        size_t start = 0;
        while (start < m_desc.size() && std::isspace(m_desc[start]))
            ++start;

        return m_desc.substr(start, m_desc.find('\n', start) - start);
    }

    [[nodiscard]] std::string for_log() const noexcept
    {
        std::stringstream ss;
        ss << id() << " ";
        ss << type() << " ";
        ss << status() << " ";
        ss << short_decs();

        return ss.str();
    }

    [[nodiscard]] usize text_size() const noexcept { return m_desc.size(); }

    static Issue from_fstream(std::ifstream& ifs)
    {
        auto check = [](bool cond) {
            if (!cond)
                throw std::runtime_error{"Bad issue format."};
        };

        u64 id{};
        Type type{};
        Status status{};
        std::string token;

        ifs >> token >> std::skipws;
        check(token == "ID");
        ifs >> id;

        ifs >> token >> std::skipws;
        check(token == "T");
        ifs >> type;

        ifs >> token >> std::skipws;
        check(token == "S");
        ifs >> status;

        ifs >> std::skipws;
        std::string text{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};

        return Issue{id, type, status, text};
    }

private:
    u64 m_id;
    Type m_type;
    Status m_status;
    std::string m_desc;
};

inline std::ofstream& operator<<(std::ofstream& ofs, const Issue& issue)
{
    ofs << "ID ";
    ofs << issue.id() << "\n";
    ofs << "T ";
    ofs << issue.type() << "\n";
    ofs << "S ";
    ofs << issue.status() << "\n\n";
    ofs << "\n";
    ofs << issue.desc() << "\n";

    return ofs;
}

class IssueTracker {
public:
    explicit IssueTracker() : m_md{read_md()} {}

    IssueTracker(const IssueTracker&) = delete;
    IssueTracker(IssueTracker&&) noexcept = delete;
    IssueTracker& operator=(const IssueTracker&) = delete;
    IssueTracker& operator=(IssueTracker&&) noexcept = delete;

    ~IssueTracker()
    {
        try {
            auto md_stream{open_md_write()};
            md_stream << m_md;
        }
        catch (const std::exception& ex) {
            std::cout << "Failed to write new md: " << ex.what() << "\n";
        }
    }

    static void cmd_init()
    {
        if (std::filesystem::exists(main_dir))
            throw std::runtime_error{"Issue tracker already initialized."};

        std::filesystem::create_directory(main_dir);
        std::filesystem::create_directory(issues_dir);
        std::ofstream mdfs{md_file};
        mdfs << initial_md;
    }

    static std::string new_issue_path(u64 id) { return issues_dir + path_sep + std::to_string(id); }

    static std::vector<Issue> all_issues()
    {
        std::vector<Issue> issues;
        for (const auto& entry : fs::recursive_directory_iterator{issues_dir}) {
            log(entry.path());
            std::ifstream is{entry.path()};
            issues.emplace_back(Issue::from_fstream(is));
        }

        return issues;
    }

    void new_issue(const std::string& desc)
    {
        u64 id = next_id();
        std::ofstream file{new_issue_path(id)};
        file << Issue{id, Type::task, Status::not_started, desc};
    }

private:
    static std::ifstream open_md_read()
    {
        if (!std::filesystem::exists(main_dir)) {
            if constexpr (!dev)
                throw std::runtime_error{"Issue tracker not initialized. Please run init."};

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
        MD md{};
        md_stream >> md;
        return md;
    }

    u64 next_id() { return m_md.m_id++; }

private: // NOLINT
    MD m_md;
};

// NOLINTEND(readability-implicit-bool-conversion)

#endif
