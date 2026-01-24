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
#include <iostream>
#include <stdexcept>
#include <string>

#include "types.hpp"

constexpr bool testing = true;

inline const std::string issue_dir = ".issue"; // NOLINT
inline const std::string md_file = "md";       // NOLINT

enum class Type : u8 { task, bug, feature };
enum class Status : u8 { not_started, in_prograss, done };

template<class T>
u64 for_write(T v)
{
    return static_cast<u64>(v);
}

inline std::ostream& operator<<(std::ostream& os, Type t)
{
    return os << for_write(t);
}

inline std::istream& operator>>(std::istream& os, Type& t)
{
    u8 read_t = 0;
    os >> read_t;
    t = Type(read_t);

    return os;
}

inline std::ostream& operator<<(std::ostream& os, Status s)
{
    return os << for_write(s);
}

inline std::istream& operator>>(std::istream& os, Status& s)
{
    u8 read_s = 0;
    os >> read_s;
    s = Status(read_s);

    return os;
}

struct MD {
    u64 m_id;
};

static const MD initial_md = {
    .m_id = 1,
};

inline std::ostream& operator<<(std::ostream& os, const MD& md)
{
    os << for_write(md.m_id);
    return os;
}

inline std::istream& operator>>(std::istream& os, MD& md)
{
    os >> md.m_id;
    return os;
}

class Issue {
public:
    Issue(u64 id, Type type, Status status, std::string text)
        : m_id{id}
        , m_type{type}
        , m_status{status}
        , m_text{std::move(text)}
    {
    }

    [[nodiscard]] u64 id() const noexcept { return m_id; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    [[nodiscard]] usize text_size() const noexcept { return m_text.size(); }

private:
    u64 m_id;
    Type m_type;
    Status m_status;
    std::string m_text;
};

inline std::ostream& operator<<(std::ostream& os, const Issue& issue)
{
    os << "ID " << issue.id() << "\n";
    os << "T " << issue.type() << "\n";
    os << "S " << issue.status() << "\n";
    os << "\n";
    os << issue.text() << "\n";

    return os;
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
        if (std::filesystem::exists(issue_dir))
            throw std::runtime_error{"Issue tracker already initialized."};

        std::filesystem::create_directory(issue_dir);
        std::ofstream mdfs{issue_dir + "/" + md_file};
        mdfs << initial_md;
    }

    static std::string new_path(u64 id) { return issue_dir + "/" + std::to_string(id); }

    void new_issue(const std::string& message)
    {
        u64 id = next_id();
        std::ofstream file{new_path(id)};
        file << Issue{id, Type::task, Status::not_started, message};
    }

private:
    static std::ifstream open_md_read()
    {
        if (!std::filesystem::exists(issue_dir)) {
            if constexpr (!testing)
                throw std::runtime_error{"Issue tracker not initialized. Please run init."};

            cmd_init();
        }

        return std::ifstream{issue_dir + "/" + md_file};
    }

    static std::ofstream open_md_write()
    {
        return std::ofstream{issue_dir + "/" + md_file, std::ios::out | std::ios::trunc};
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

#endif
