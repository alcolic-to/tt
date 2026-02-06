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
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <util.hpp>

#include "cli11/CLI11.hpp"
#include "task.hpp"

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-avoid-return-with-void-value)

const std::string version = "0.0.1"; // NOLINT

const std::string subcmd_init = "init";         // NOLINT
const std::string subcmd_new = "new";           // NOLINT
const std::string subcmd_log = "log";           // NOLINT
const std::string subcmd_show = "show";         // NOLINT
const std::string subcmd_roll = "roll";         // NOLINT
const std::string subcmd_rollback = "rollback"; // NOLINT

const std::string req_id = "id";
const std::string opt_message = "-m,--message"; // NOLINT
const std::string opt_message_short = "-m";     // NOLINT
const std::string opt_type = "-t,--type";       // NOLINT
const std::string opt_type_short = "-t";        // NOLINT

const std::string default_editor = "vim";
const std::string default_desc_message =
    "\n"
    "# Please enter task description. Lines starting with '#' will be ignored and \n"
    "# empty description aborts task creation.";

namespace {

int test_main() // NOLINT
{
    try {
        TaskTracker tt;

        // for (u64 i = 0; i < 1000; ++i)
        //     tt.new_task(Type::task, std::format("This is {} task.", i));

        auto all = tt.all_tasks();
        for (const auto& task : all)
            std::cout << task.for_log() << "\n";
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
        return 1;
    }

    return 0;
}

bool spaces_only(const std::string& s)
{
    return std::ranges::all_of(s, [](u8 c) { return std::isspace(c); });
}

std::string desc_from_editor()
{
    std::ofstream{msg_file, std::ios::trunc} << default_desc_message;
    std::system(std::string{default_editor + " " + msg_file}.c_str());

    std::stringstream ss;
    std::string line;

    for (std::ifstream in{msg_file}; std::getline(in, line);)
        if (!line.starts_with("#"))
            ss << line << '\n';

    std::filesystem::remove(msg_file);
    return ss.str();
}

void tt_cmd_new(TaskTracker& tt, CLI::App& cmd_new)
{
    std::string desc;
    Type type{Type::task};

    if (CLI::Option* opt = cmd_new.get_option(opt_message_short); *opt)
        desc = opt->as<std::string>();

    if (CLI::Option* opt = cmd_new.get_option(opt_type_short); *opt)
        type = as<Type>(opt->as<u64>());

    if (desc.empty())
        desc = desc_from_editor();

    trim_left(desc), trim_right(desc);

    if (desc.empty() || spaces_only(desc))
        throw std::runtime_error{"Empty message. Aborting creation."};

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

void tt_cmd_roll(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_show)
{
    ID id = as<ID>(cmd_show.get_option(req_id)->as<u64>());
    tt.roll(id);
}

void tt_cmd_rollback(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_show)
{
    ID id = as<ID>(cmd_show.get_option(req_id)->as<u64>());
    tt.rollback(id);
}

void tt_main(const CLI::App& app)
{
    if (auto* cmd = app.get_subcommand(subcmd_init); *cmd)
        return TaskTracker::cmd_init();

    TaskTracker tt;

    if (auto* cmd = app.get_subcommand(subcmd_new); *cmd)
        return tt_cmd_new(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_log); *cmd)
        return tt_cmd_log(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_show); *cmd)
        return tt_cmd_show(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_roll); *cmd)
        return tt_cmd_roll(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_rollback); *cmd)
        return tt_cmd_rollback(tt, *cmd);
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
    auto* cmd_show = app.add_subcommand(subcmd_show, "Shows single task.");
    cmd_show->add_option(req_id, "Task id.")->required(true);

    /**
     * Roll subcommand.
     */
    auto* cmd_roll = app.add_subcommand(subcmd_roll, "Rolls state by 1.");
    cmd_roll->add_option(req_id, "Task id.")->required(true);

    /**
     * Rollback subcommand.
     */
    auto* cmd_rollback = app.add_subcommand(subcmd_rollback, "Rolls back state by 1.");
    cmd_rollback->add_option(req_id, "Task id.")->required(true);

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
