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

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-avoid-return-with-void-value)

const std::string version = "0.0.1"; // NOLINT

const std::string subcmd_init = "init"; // NOLINT
const std::string subcmd_new = "new";   // NOLINT
const std::string subcmd_log = "log";   // NOLINT
const std::string subcmd_show = "show"; // NOLINT

const std::string req_id = "id";
const std::string opt_message = "-m,--message"; // NOLINT
const std::string opt_message_short = "-m";     // NOLINT
const std::string opt_type = "-t,--type";       // NOLINT
const std::string opt_type_short = "-t";        // NOLINT

namespace {

int test_main() // NOLINT
{
    try {
        TaskTracker tt;

        // for (u64 i = 0; i < 10; ++i)
        //     tt.new_task(Type::task, std::format("This is {} task.", i));

        // tt.change_task_status(ID(2), Status::in_progress);

        auto all = tt.all_tasks();
        for (const auto& task : all)
            std::cout << task.for_log() << "\n";

        // tt.resolve_task(ID(1));

        // all = tt.all_tasks();
        // for (const auto& task : all)
        //     std::cout << task.for_log() << "\n";

        // Task t1{TaskTracker::get_task(1)};
        // std::cout << t1.for_show() << "\n";

        // Task t2{TaskTracker::get_task(20)};
        // std::cout << t2.for_log() << "\n";
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
        return 1;
    }

    return 0;
}

void tt_cmd_new(TaskTracker& tt, CLI::App& cmd_new)
{
    std::string desc;
    Type type{Type::task};

    if (CLI::Option* opt = cmd_new.get_option(opt_message_short); *opt)
        desc = opt->as<std::string>();

    if (CLI::Option* opt = cmd_new.get_option(opt_type_short); *opt)
        type = as<Type>(opt->as<u64>());

    tt.new_task(type, std::move(desc));
}

void tt_cmd_log(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_log)
{
    for (const Task& task : tt.all_tasks())
        std::cout << task.for_log() << "\n";
}

void tt_cmd_show(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_show)
{
    ID id = as<ID>(cmd_show.get_option(req_id)->as<u64>());
    std::cout << tt.get_task(id).for_show();
}

void tt_main(const CLI::App& app)
{
    if (auto* init = app.get_subcommand(subcmd_init); init != nullptr && init->parsed())
        return TaskTracker::cmd_init();

    TaskTracker tt;

    if (auto* cmd = app.get_subcommand(subcmd_new); *cmd)
        return tt_cmd_new(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_log); *cmd)
        return tt_cmd_log(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_show); *cmd)
        return tt_cmd_show(tt, *cmd);
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
    [[maybe_unused]] auto* cmd_init = app.add_subcommand(subcmd_init, "Initializes task tracker.");

    /**
     * New subcommand.
     */
    auto* cmd_new = app.add_subcommand(subcmd_new, "Creates new task.");
    cmd_new->add_option(opt_message, "Message that will be written to the task.");
    cmd_new->add_option(opt_type, "Task type (0 -> task, 1 -> bug, 2 -> feature).");

    /**
     * Log subcommand.
     */
    [[maybe_unused]] auto* cmd_log = app.add_subcommand(subcmd_log, "Logs all tasks.");

    /**
     * Show subcommand.
     */
    [[maybe_unused]] auto* cmd_show = app.add_subcommand(subcmd_show, "Shows single task.");
    cmd_show->add_option(req_id, "Task id.")->required(true);

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    try {
        tt_main(app);
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
        return 1;
    }

    return 0;
}

// NOLINTEND(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-avoid-return-with-void-value)
