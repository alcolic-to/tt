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

#include <atomic>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "types.hpp"

inline const std::string issue_dir = ".issue"; // NOLINT

enum class Type : u8 { bug, feature };
enum class Status : u8 { not_started, in_prograss, done };

inline u64 next_id()
{
    static std::atomic<u64> id;
    return id.fetch_add(1, std::memory_order_relaxed);
}

class Options {
public:
    explicit Options(bool init) : m_init{init} {}

    [[nodiscard]] bool init() const noexcept { return m_init; }

private:
    bool m_init;
};

class Issue {
public:
    Issue(Type type, Status status, std::string text)
        : m_type{type}
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
    u64 m_id{next_id()};
    Type m_type;
    Status m_status;
    u64 m_text_size;
    std::string m_text;
};

class IssueTracker {
public:
    explicit IssueTracker(Options opt)
    {
        if (!std::filesystem::exists(issue_dir)) {
            if (!opt.init())
                throw std::runtime_error{"Issue tracker not initialized. Please run init."};

            std::filesystem::create_directory(issue_dir);
            return;
        }
    }
};

#endif
