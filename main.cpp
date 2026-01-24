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
#include <iostream>

#include "cli11/CLI11.hpp"
#include "task.hpp"

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto)

const std::string version = "0.0.1"; // NOLINT

const std::string subcmd_init = "init"; // NOLINT
const std::string subcmd_new = "new";   // NOLINT

const std::string opt_message = "-m,--messsage"; // NOLINT
const std::string opt_message_short = "-m";      // NOLINT

namespace {

int test_main()
{
    TaskTracker tt;

    for (u64 i = 0; i < 1024 * 10; ++i)
        tt.new_task(Type::task, std::format("This is {} task.", i));

    auto all = TaskTracker::all_tasks();
    for (const auto& task : all)
        std::cout << task.for_log() << "\n";

    return 0;
}

void it_main(const CLI::App& app)
{
    if (auto* init = app.get_subcommand(subcmd_init); init != nullptr && init->parsed())
        return TaskTracker::cmd_init();

    TaskTracker it;

    if (auto* cmd_new = app.get_subcommand(subcmd_new); cmd_new != nullptr && cmd_new->parsed()) {
        if (CLI::Option* opt = cmd_new->get_option(opt_message_short); *opt) {
            // std::cout << "New subcommand: -m provided\n";
            // std::string message = opt->as<std::string>();
            // std::cout << "Message: " << message << "\n";
        }
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if constexpr (dev)
        return test_main();

    CLI::App app{"Task tracker."};
    argv = app.ensure_utf8(argv);

    app.set_version_flag("-v,--version", version, "Task tracker version.");

    /**
     * Init subcommand.
     */
    auto* cmd_init = app.add_subcommand(subcmd_init, "Initializes task tracker.");
    app.require_subcommand();

    /**
     * New subcommand.
     */
    auto* cmd_new = app.add_subcommand(subcmd_new, "Creates new task.");
    app.require_subcommand();

    cmd_new->add_option(opt_message, "Message that will be written to the task.");

    CLI11_PARSE(app, argc, argv);

    if (*cmd_init)
        std::cout << "init subcommand\n";

    if (*cmd_new)
        std::cout << "new subcommand\n";

    try {
        it_main(app);
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
        return 1;
    }

    return 0;
}

// NOLINTEND(hicpp-use-auto, modernize-use-auto)
