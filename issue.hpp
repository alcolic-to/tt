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

#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "types.hpp"

inline const std::string issue_dir = ".issue";          // NOLINT
inline const std::string md_file = "md_long_file_name"; // NOLINT

enum class Type : u8 { task, bug, feature };
enum class Status : u8 { not_started, in_prograss, done };

struct MD {
    u64 id;
};

static const MD initial_md = {
    .id = 1,
};

class Issue {
public:
    Issue(u64 id, Type type, Status status, std::string text)
        : m_id{id}
        , m_type{type}
        , m_status{status}
        , m_text_size{text.size()}
        , m_text{std::move(text)}
    {
    }

    [[nodiscard]] u64 id() const noexcept { return m_id; }

    [[nodiscard]] Type type() const noexcept { return m_type; }

    [[nodiscard]] Status status() const noexcept { return m_status; }

    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

private:
    u64 m_id;
    Type m_type;
    Status m_status;
    u64 m_text_size;
    std::string m_text;
};

class IssueTracker {
public:
    explicit IssueTracker() : m_md{open_md()}
    {
        MD md; // NOLINT
        m_md.read(std::bit_cast<char*>(&md), sizeof(md));

        m_id = md.id;
    }

    static void cmd_init()
    {
        if (std::filesystem::exists(issue_dir))
            throw std::runtime_error{"Issue tracker already initialized."};

        std::filesystem::create_directory(issue_dir);
        std::ofstream mdfs{issue_dir + "/" + md_file};
        mdfs.write(std::bit_cast<char*>(&initial_md), sizeof(initial_md));
    }

    void new_issue(const std::string& message) { auto id = next_id(); }

private:
    static std::fstream open_md()
    {
        if (!std::filesystem::exists(issue_dir))
            throw std::runtime_error{"Issue tracker not initialized. Please run init."};

        return std::fstream{issue_dir + "/" + md_file};
    }

    u64 next_id() { return m_id++; }

private: // NOLINT
    std::fstream m_md;
    u64 m_id{0};
};

#endif
