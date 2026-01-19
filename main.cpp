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
#include <exception>
#include <filesystem>
#include <iostream>
#include <system_error>

#include "cli11/CLI11.hpp"
#include "filesystem"
#include "issue.hpp"

const std::string version = "0.0.1"; // NOLINT

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    CLI::App app{"Issue tracker."};
    argv = app.ensure_utf8(argv);

    bool init = false;

    // clang-format off
    app.set_version_flag("-v,--version",   version,      "Finder version.");
    app.add_flag        ("--init",         init,         "Initializes issue tracker.");
    // clang-format on

    CLI11_PARSE(app, argc, argv);

    Options opt{init};

    try {
        IssueTracker it{opt};
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
    }
}
