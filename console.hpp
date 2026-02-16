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

#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <format>
#include <iostream>
#include <string>

#include "types.hpp"

inline const std::string esc = "\x1b["; // NOLINT

enum class Color : u8 {
    black,
    green,
    blue,
    high_blue,
    red,
    white,
    gray,
    high_gray,
    yellow,
    term_default
};

static constexpr Color black = Color::black;
static constexpr Color green = Color::green;
static constexpr Color blue = Color::blue;
static constexpr Color high_blue = Color::high_blue;
static constexpr Color red = Color::red;
static constexpr Color white = Color::white;
static constexpr Color gray = Color::gray;
static constexpr Color high_gray = Color::high_gray;
static constexpr Color yellow = Color::yellow;

static constexpr Color term_default = Color::term_default;

/**
 * Foreground and background color values with 256 color mode.
 */
template<Color c>
static constexpr u32 color_value()
{ // clang-format off
    if      constexpr (c == black)        return 0;
    else if constexpr (c == green)        return 2;
    else if constexpr (c == blue)         return 4;
    else if constexpr (c == high_blue)    return 14;
    else if constexpr (c == red)          return 1;
    else if constexpr (c == white)        return 7;
    else if constexpr (c == gray)         return 237;
    else if constexpr (c == high_gray)    return 242;
    else if constexpr (c == yellow)       return 3;
    else if constexpr (c == term_default) return 39;
    else static_assert(false, "Invalid color.");
} // clang-format on

template<class... Args>
void command(std::format_string<Args...> str, Args&&... args)
{
    std::cout << esc + std::format(str, std::forward<Args>(args)...);
}

template<class Arg>
void command(Arg&& arg)
{
    std::cout << esc + std::forward<Arg>(arg);
}

template<Color clr>
void set_color()
{
    if constexpr (clr == term_default)
        command("{}m", color_value<term_default>());
    else
        command("38;5;{}m", color_value<clr>());
}

template<Color clr = term_default, class... Args>
void print(std::format_string<Args...> str, Args&&... args)
{
    set_color<clr>();
    std::cout << std::format(str, std::forward<Args>(args)...);
    set_color<term_default>();
}

template<Color clr = term_default, class Arg>
void print(Arg&& arg)
{
    set_color<clr>();
    std::cout << std::forward<Arg>(arg); // NOLINT
    set_color<term_default>();
}

template<Color clr = term_default, class... Args>
void println(std::format_string<Args...> str, Args&&... args)
{
    set_color<clr>();
    std::cout << std::format(str, std::forward<Args>(args)...) << "\n";
    set_color<term_default>();
}

template<Color clr = term_default, class Arg>
void println(Arg&& arg)
{
    set_color<clr>();
    std::cout << std::forward<Arg>(arg) << "\n"; // NOLINT
    set_color<term_default>();
}

#endif // CONSOLE_HPP
