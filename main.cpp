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
#include <stdexcept>
#include <string>
#include <util.hpp>

#include "cli11/CLI11.hpp"
#include "console.hpp"
#include "task.hpp"

// NOLINTBEGIN(hicpp-use-auto, modernize-use-auto, readability-static-accessed-through-instance,
// readability-avoid-return-with-void-value)

const std::string version = "0.0.1"; // NOLINT

const std::string subcmd_init = "init";         // NOLINT
const std::string subcmd_push = "push";         // NOLINT
const std::string subcmd_pop = "pop";           // NOLINT
const std::string subcmd_log = "log";           // NOLINT
const std::string subcmd_show = "show";         // NOLINT
const std::string subcmd_roll = "roll";         // NOLINT
const std::string subcmd_rollback = "rollback"; // NOLINT
const std::string subcmd_amend = "amend";       // NOLINT

// const std::string req_id = "id";

const std::string opt_offset = "offset";
const std::string opt_id = "--id";
const std::string opt_message = "-m,--message"; // NOLINT
const std::string opt_message_short = "-m";     // NOLINT
const std::string opt_type = "-t,--type";       // NOLINT
const std::string opt_type_short = "-t";        // NOLINT
const std::string opt_all = "-a,--all";         // NOLINT
const std::string opt_all_short = "-a";         // NOLINT

const std::string default_editor = "vim";
const std::string default_editor_message =
    "\n"
    "# Please enter task description. Lines starting with '#' will be ignored and \n"
    "# empty description aborts task creation.";

namespace {

int test_main() // NOLINT
{
    try {
        // TaskTracker tt;

        // std::cout << esc << "38;5;2m" << "This should be green\n"
        //           << esc << "39m"
        //           << "And this should be default\n";

        // println<green>("Some green text");
        // println<red>("Some red text");
        // println<yellow>("Some yellow text");
        // println("Some normal text");

        // for (u64 i = 0; i < 1000; ++i)
        //     tt.new_task(Type::task, std::format("This is {} task.", i));
        // auto pred = [](const Task& t) { return true; };

        // auto all = tt.all_tasks();
        // for (const auto& task : all)
        //     std::cout << task.for_log() << "\n";
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

/**
 * Creates new file for message editing.
 * It spawns editor and waits for user to exit. After that, message is read from file.
 * You can provide additional message that will be writen before default editor message.
 */
std::string desc_from_editor(const std::string& msg = "")
{
    std::ofstream{msg_file, std::ios::trunc} << msg << default_editor_message;
    std::system(std::string{default_editor + " " + msg_file}.c_str());

    std::stringstream ss;
    std::string line;

    for (std::ifstream in{msg_file}; std::getline(in, line);)
        if (!line.starts_with("#"))
            ss << line << '\n';

    std::filesystem::remove(msg_file);
    return ss.str();
}

void tt_cmd_push(TaskTracker& tt, CLI::App& cmd_push)
{
    std::string desc;
    Type type{Type::task};

    if (CLI::Option* opt = cmd_push.get_option(opt_message_short); *opt)
        desc = opt->as<std::string>();

    if (CLI::Option* opt = cmd_push.get_option(opt_type_short); *opt)
        type = as<Type>(opt->as<u64>());

    if (desc.empty())
        desc = desc_from_editor();

    trim_left(desc), trim_right(desc);

    if (desc.empty() || spaces_only(desc))
        throw std::runtime_error{"Empty message. Aborting creation."};

    tt.new_task(type, std::move(desc));
}

void tt_cmd_pop(TaskTracker& tt, CLI::App& cmd_pop)
{
    if (CLI::Option* opt = cmd_pop.get_option(opt_id); *opt) {
        tt.resolve_task(as<ID>(opt->as<u64>()));
        return;
    }

    std::vector<Task> tasks{tt.all_tasks_not_done()};
    if (tasks.empty())
        throw std::runtime_error{"No non-resolved tasks."};

    u64 offset = 0;

    if (CLI::Option* opt = cmd_pop.get_option(opt_offset); *opt)
        offset = opt->as<u64>();

    if (offset >= tasks.size())
        throw std::runtime_error{"Offset to large."};

    Task& t = tasks[offset];
    t.set_status(Status::done);
    tt.save_task(t);
}

void log_task(const Task& task, u64 vid = -1)
{
    if (vid != -1)
        print<yellow>("{:<3} ", vid);

    print<yellow>("{} ", task.for_log_id());
    print<high_blue>("{} ", task.for_log_type());

    switch (task.status()) { // clang-format off
    case Status::not_started: print<high_gray>("{} ", task.for_log_status()); break;
    case Status::in_progress: print<yellow>("{} ", task.for_log_status()); break;
    case Status::done:        print<green>("{} ", task.for_log_status()); break;
    default: throw std::runtime_error{"Invalid task status."};
    } // clang-format on

    println("{}", task.for_log_desc());
}

void tt_cmd_log(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_log)
{
    bool opt_all = false;
    if (CLI::Option* opt = cmd_log.get_option(opt_all_short); *opt)
        opt_all = true;

    auto pred = [&]() -> std::function<bool(const Task&)> {
        if (opt_all)
            return [](const Task&) { return true; };

        return [](const Task& t) { return t.status() != Status::done; };
    }();

    u64 vid = 0;
    for (const Task& task : tt.all_tasks_where(pred))
        if (opt_all)
            log_task(task);
        else
            log_task(task, vid++);
}

void show_task(const Task& task)
{
    println<yellow>("{}", task.for_show_id());
    println<high_blue>("{}", task.for_show_type());

    switch (task.status()) { // clang-format off
    case Status::not_started: println<high_gray>("{}", task.for_show_status()); break;
    case Status::in_progress: println<yellow>("{}", task.for_show_status()); break;
    case Status::done:        println<green>("{}", task.for_show_status()); break;
    default: throw std::runtime_error{"Invalid task status."};
    } // clang-format on

    println("\n{}", task.for_show_desc());
}

void tt_cmd_show(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_show)
{
    if (CLI::Option* opt = cmd_show.get_option(opt_id); *opt) {
        show_task(tt.get_task(as<ID>(opt->as<u64>())));
        return;
    }

    u64 offset = 0;
    if (CLI::Option* opt = cmd_show.get_option(opt_offset); *opt)
        offset = opt->as<u64>();

    show_task(tt.get_task(as<Offset>(offset)));
}

void tt_cmd_roll(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_roll)
{
    if (CLI::Option* opt = cmd_roll.get_option(opt_id); *opt) {
        tt.roll(as<ID>(cmd_roll.get_option(opt_id)->as<u64>()));
        return;
    }

    u64 offset = 0;
    if (CLI::Option* opt = cmd_roll.get_option(opt_offset); *opt)
        offset = opt->as<u64>();

    Task t{tt.get_task(as<Offset>(offset))};
    tt.roll(t);
}

void tt_cmd_rollback(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_rollback)
{
    if (CLI::Option* opt = cmd_rollback.get_option(opt_id); *opt) {
        tt.rollback(as<ID>(cmd_rollback.get_option(opt_id)->as<u64>()));
        return;
    }

    u64 offset = 0;
    if (CLI::Option* opt = cmd_rollback.get_option(opt_offset); *opt)
        offset = opt->as<u64>();

    Task t{tt.get_task(as<Offset>(offset))};
    tt.rollback(t);
}

void tt_cmd_amend(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_amend)
{
    Task task;

    if (CLI::Option* opt = cmd_amend.get_option(opt_id); *opt) {
        task = tt.get_task(as<ID>(cmd_amend.get_option(opt_id)->as<u64>()));
    }
    else {
        u64 offset = 0;
        if (CLI::Option* opt = cmd_amend.get_option(opt_offset); *opt)
            offset = opt->as<u64>();

        task = tt.get_task(as<Offset>(offset));
    }

    if (CLI::Option* opt = cmd_amend.get_option(opt_type_short); *opt)
        task.set_type(as<Type>(opt->as<u64>()));

    std::string desc;

    if (CLI::Option* opt = cmd_amend.get_option(opt_message_short); *opt)
        desc = opt->as<std::string>();

    if (desc.empty())
        desc = desc_from_editor(task.desc());

    trim_left(desc), trim_right(desc);

    if (desc.empty() || spaces_only(desc))
        throw std::runtime_error{"Empty message. Aborting amend."};

    task.set_desc(std::move(desc));
    tt.save_task(task);
}

void tt_main(const CLI::App& app)
{
    if (auto* cmd = app.get_subcommand(subcmd_init); *cmd)
        return TaskTracker::cmd_init();

    TaskTracker tt;

    if (auto* cmd = app.get_subcommand(subcmd_push); *cmd)
        return tt_cmd_push(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_pop); *cmd)
        return tt_cmd_pop(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_log); *cmd)
        return tt_cmd_log(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_show); *cmd)
        return tt_cmd_show(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_roll); *cmd)
        return tt_cmd_roll(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_rollback); *cmd)
        return tt_cmd_rollback(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_amend); *cmd)
        return tt_cmd_amend(tt, *cmd);
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
     * Push subcommand.
     */
    auto* cmd_push = app.add_subcommand(subcmd_push, "Creates new task.")->alias("new");
    cmd_push->add_option(opt_message, "Message that will be written to the task.");
    cmd_push->add_option(opt_type, "Task type (0 -> task, 1 -> bug, 2 -> feature).");

    /**
     * Pop subcommand.
     */
    auto* cmd_pop = app.add_subcommand(subcmd_pop, "Resolves task.")->alias("resolve");
    cmd_pop->add_option(opt_offset, "Offset (zero based) from top of a non-resolved tasks.");
    cmd_pop->add_option(opt_id, "Task id to pop (resolve).");

    /**
     * Log subcommand.
     */
    auto* cmd_log = app.add_subcommand(subcmd_log, "Logs all unresolved tasks.");
    cmd_log->add_flag(opt_all, "Logs all tasks.");

    /**
     * Show subcommand.
     */
    auto* cmd_show = app.add_subcommand(subcmd_show, "Shows single task.");
    cmd_show->add_option(opt_offset, "Offset (zero based) from top of a non-resolved tasks.");
    cmd_show->add_option(opt_id, "Task id.");

    /**
     * Roll subcommand.
     */
    auto* cmd_roll = app.add_subcommand(subcmd_roll, "Rolls state by 1.");
    cmd_roll->add_option(opt_offset, "Offset (zero based) from top of a non-resolved tasks.");
    cmd_roll->add_option(opt_id, "Task id.");

    /**
     * Rollback subcommand.
     */
    auto* cmd_rollback = app.add_subcommand(subcmd_rollback, "Rolls back state by 1.");
    cmd_rollback->add_option(opt_offset, "Offset (zero based) from top of a non-resolved tasks.");
    cmd_rollback->add_option(opt_id, "Task id.");

    /**
     * Amend subcommand.
     */
    auto* cmd_amend = app.add_subcommand(subcmd_amend, "Replaces tasks message.");
    cmd_amend->add_option(opt_offset, "Offset (zero based) from top of a non-resolved tasks.");
    cmd_amend->add_option(opt_id, "Task id.");
    cmd_amend->add_option(opt_message, "Message that will be written to the task.");
    cmd_amend->add_option(opt_type, "Task type (0 -> task, 1 -> bug, 2 -> feature).");

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
