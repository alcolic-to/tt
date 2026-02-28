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
#include <cctype>
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

const std::string version = "0.0.2";

const std::string subcmd_init = "init";
const std::string subcmd_config = "config";
const std::string subcmd_register = "register";
const std::string subcmd_whoami = "whoami";
const std::string subcmd_push = "push";
const std::string subcmd_pop = "pop";
const std::string subcmd_log = "log";
const std::string subcmd_show = "show";
const std::string subcmd_roll = "roll";
const std::string subcmd_rollb = "rollb";
const std::string subcmd_amend = "amend";
const std::string subcmd_take = "take";
const std::string subcmd_takeb = "takeb";
const std::string subcmd_assign = "assign";
const std::string subcmd_assignb = "assignb";

const std::string opt_username = "--username,-n";
const std::string opt_username_short = "-n";
const std::string opt_email = "--email,-m";
const std::string opt_email_short = "-m";
const std::string opt_vid_or_uid = "vuid";
const std::string opt_message = "-m,--message";
const std::string opt_message_short = "-m";
const std::string opt_type = "-t,--type";
const std::string opt_type_short = "-t";
const std::string opt_all = "-a,--all";
const std::string opt_all_short = "-a";
const std::string opt_global = "--global,-g";
const std::string opt_global_short = "-g";
const std::string opt_local = "--local,-l";
const std::string opt_local_short = "-l";
const std::string opt_worker = "--worker,-w";
const std::string opt_worker_short = "-w";

const std::string req_username = "username";

const std::string default_editor = "vim";
const std::string default_editor_message =
    "\n"
    "# Please enter task description. Lines starting with '#' will be ignored and \n"
    "# empty description aborts task creation.";

namespace {

int test_main() // NOLINT
{
    try {
        // std::ofstream{cfg_file, std::ios::trunc} << "Some new file content";

        // std::ofstream{cfg_file, std::ios::trunc} << "alcolic-to";

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

/**
 * Creates new file for message editing.
 * It spawns editor and waits for user to exit. After that, message is read from file.
 * You can provide additional message that will be writen before default editor message.
 */
std::string desc_from_editor(const std::string& msg = "")
{
    std::ofstream{msg_file, std::ios::trunc} << msg << default_editor_message;
    std::system(std::string{default_editor + " " + msg_file.string()}.c_str());

    std::stringstream ss;
    std::string line;

    for (std::ifstream in{msg_file}; std::getline(in, line);)
        if (!line.starts_with("#"))
            ss << line << '\n';

    std::filesystem::remove(msg_file);
    return ss.str();
}

std::string desc_from_opt_or_editor(CLI::App& cmd, const std::string& inital_desc = "")
{
    std::string desc;

    if (CLI::Option* opt = cmd.get_option(opt_message_short); *opt)
        desc = opt->as<std::string>();

    if (desc.empty())
        desc = desc_from_editor(inital_desc);

    trim_left(desc), trim_right(desc);

    if (desc.empty() || spaces_only(desc))
        throw std::runtime_error{"Empty message. Aborting creation."};

    return desc;
}

/**
 * Returns task based on command line input (VID or UID).
 */
Task task_from_vuid(TaskTracker& tt, CLI::App& cmd)
{
    u64 vid = 0;

    if (CLI::Option* opt = cmd.get_option(opt_vid_or_uid); *opt) {
        std::string vid_or_uid{opt->as<std::string>()};
        if (UID::valid_uid(vid_or_uid))
            return tt.get_task(UID{vid_or_uid});

        if (!digits_only(vid_or_uid))
            throw std::runtime_error{"Invalid VID or UID."};

        vid = opt->as<u64>();
    }

    return tt.get_task(as<VID>(vid));
}

void tt_cmd_config(CLI::App& cmd_config)
{
    std::string username;
    if (CLI::Option* opt = cmd_config.get_option(opt_username_short); *opt)
        username = opt->as<std::string>();

    std::string email;
    if (CLI::Option* opt = cmd_config.get_option(opt_email_short); *opt)
        email = opt->as<std::string>();

    Config config{TaskTracker::cmd_config(username, email)};
    println("{} <{}>", config.username(), config.email());
}

void tt_cmd_init(CLI::App& cmd_init)
{
    TaskTracker::cmd_init();
}

void tt_cmd_register(CLI::App& cmd_register)
{
    std::string username;
    if (CLI::Option* opt = cmd_register.get_option(opt_username_short); *opt)
        username = opt->as<std::string>();

    std::string email;
    if (CLI::Option* opt = cmd_register.get_option(opt_email_short); *opt)
        email = opt->as<std::string>();

    TaskTracker::cmd_register(std::move(username), std::move(email));
}

void tt_cmd_whoami(TaskTracker& tt, CLI::App& cmd_init)
{
    println("{} <{}>", tt.username(), tt.email());
}

void tt_cmd_push(TaskTracker& tt, CLI::App& cmd_push)
{
    Type type{Type::task};
    if (CLI::Option* opt = cmd_push.get_option(opt_type_short); *opt)
        type = as<Type>(opt->as<u64>());

    std::string desc{desc_from_opt_or_editor(cmd_push)};

    Scope scope = Scope::local;
    if (CLI::Option* opt = cmd_push.get_option(opt_global_short); *opt)
        scope = Scope::global;

    std::string worker;
    if (CLI::Option* opt = cmd_push.get_option(opt_worker_short); *opt)
        worker = opt->as<std::string>();

    tt.new_task(scope, type, worker, desc);
}

void tt_cmd_pop(TaskTracker& tt, CLI::App& cmd_pop)
{
    Task task{task_from_vuid(tt, cmd_pop)};
    tt.resolve_task(task);
}

void log_task(const Task& task)
{
    if (task.global())
        print<high_blue>("{}", task.for_log_scope());
    else
        print<yellow>("{}", task.for_log_scope());

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

void log_task(const Task& task, u64 vid)
{
    print<yellow>("{:<3} ", vid);
    log_task(task);
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

    if (CLI::Option* opt = cmd_log.get_option(opt_global_short); *opt) {
        for (const Task& task : tt.all_tasks_where<Scope::global>(pred))
            log_task(task);

        if (opt = cmd_log.get_option(opt_local_short); !*opt)
            return;
    }

    u64 vid = 0;
    for (const Task& task : tt.all_tasks_where<Scope::local>(pred))
        if (opt_all)
            log_task(task);
        else
            log_task(task, vid++);
}

void show_task(const Task& task)
{
    println<yellow>("{}", task.for_show_id());
    if (task.worker() != default_worker())
        println("{}\n", task.worker());

    println<high_blue>("{}", task.for_show_scope());
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
    Task task{task_from_vuid(tt, cmd_show)};
    show_task(task);
}

void tt_cmd_roll(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_roll)
{
    Task task{task_from_vuid(tt, cmd_roll)};
    tt.roll(task);
}

void tt_cmd_rollb(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_rollback)
{
    Task task{task_from_vuid(tt, cmd_rollback)};
    tt.rollb(task);
}

void tt_cmd_amend(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_amend)
{
    Task task{task_from_vuid(tt, cmd_amend)};

    Type type = task.type();
    if (CLI::Option* opt = cmd_amend.get_option(opt_type_short); *opt)
        type = as<Type>(opt->as<u64>());

    std::string worker = task.worker();
    if (CLI::Option* opt = cmd_amend.get_option(opt_worker_short); *opt)
        worker = opt->as<std::string>();

    std::string desc{desc_from_opt_or_editor(cmd_amend, task.desc())};

    task.set_type(type);
    task.set_worker(std::move(worker));
    task.set_desc(std::move(desc));
    tt.save_task(task);
}

void tt_cmd_take(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_take)
{
    Task task{task_from_vuid(tt, cmd_take)};

    if (task.scope() == Scope::local)
        throw std::runtime_error{"Task already assigned to user."};

    tt.add_task_ref(task);

    task.set_worker(tt.username());
    tt.save_task(task);
}

void tt_cmd_takeb(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_untake)
{
    Task task{task_from_vuid(tt, cmd_untake)};

    if (task.scope() == Scope::local)
        throw std::runtime_error{"Can not take back local task."};

    tt.remove_task_ref(task);

    task.unset_worker();
    tt.save_task(task);
}

void tt_cmd_assign(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_assign)
{
    Task task{task_from_vuid(tt, cmd_assign)};

    if (task.scope() == Scope::local)
        throw std::runtime_error{"Can not assign local task."};

    if (CLI::Option* opt = cmd_assign.get_option(req_username); *opt)
        tt.switch_context({opt->as<std::string>(), ""});

    tt.add_task_ref(task);

    task.set_worker(tt.username());
    tt.save_task(task);
}

void tt_cmd_assignb(TaskTracker& tt, [[maybe_unused]] CLI::App& cmd_assignb)
{
    Task task{task_from_vuid(tt, cmd_assignb)};

    if (task.scope() == Scope::local)
        throw std::runtime_error{"Can not assign back local task."};

    if (task.worker().empty())
        throw std::runtime_error{"Task not assigned."};

    if (tt.username() != task.worker())
        tt.switch_context({task.worker(), ""});

    tt.remove_task_ref(task);

    task.unset_worker();
    tt.save_task(task);
}

void tt_main(const CLI::App& app)
{
    if (auto* cmd = app.get_subcommand(subcmd_config); *cmd)
        return tt_cmd_config(*cmd);

    if (auto* cmd = app.get_subcommand(subcmd_init); *cmd)
        return tt_cmd_init(*cmd);

    if (auto* cmd = app.get_subcommand(subcmd_register); *cmd)
        return tt_cmd_register(*cmd);

    TaskTracker tt;

    if (auto* cmd = app.get_subcommand(subcmd_whoami); *cmd)
        return tt_cmd_whoami(tt, *cmd);

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

    if (auto* cmd = app.get_subcommand(subcmd_rollb); *cmd)
        return tt_cmd_rollb(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_amend); *cmd)
        return tt_cmd_amend(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_take); *cmd)
        return tt_cmd_take(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_takeb); *cmd)
        return tt_cmd_takeb(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_assign); *cmd)
        return tt_cmd_assign(tt, *cmd);

    if (auto* cmd = app.get_subcommand(subcmd_assignb); *cmd)
        return tt_cmd_assignb(tt, *cmd);
}

} // namespace

int main(int argc, char* argv[])
{
    if constexpr (dev)
        return test_main();

    CLI::App app{"Task tracker."};
    argv = app.ensure_utf8(argv);

    app.set_version_flag("-v,--version", version, "Task tracker version.");

    /* clang-format off */ 

    /**
     * Init subcommand.
     */
    [[maybe_unused]] auto* cmd_init = app.add_subcommand(subcmd_init, "Initializes task tracker.");

    /**
     * Config subcommand.
     */
    auto* cmd_config = app.add_subcommand(subcmd_config, "Configures user info.");
    cmd_config->add_option(opt_username, "username for new user (default is read from env).");
    cmd_config->add_option(opt_email, "email for new user (default is 'none').");

    /**
     * Register subcommand.
     */
    auto* cmd_register = app.add_subcommand(subcmd_register, "Registers new user to an existing TT repo.");
    cmd_register->add_option(opt_username, "username for new user (default is read from config).");
    cmd_register->add_option(opt_email, "email for new user (default is read from config).");

    /**
     * Whoami subcommand.
     */
    [[maybe_unused]] auto* cmd_whoami = app.add_subcommand(subcmd_whoami, "Prints user info.");

    /**
     * Push subcommand.
     */
    auto* cmd_push = app.add_subcommand(subcmd_push, "Creates new task.")->alias("new");
    cmd_push->add_option(opt_message, "Message that will be written to the task. If not provided, editor will be opened.");
    cmd_push->add_option(opt_type, "Task type (0 -> task, 1 -> bug, 2 -> feature).");
    cmd_push->add_option(opt_worker, "Task worker.");
    cmd_push->add_flag(opt_global, "Creates global task.");
    cmd_push->add_flag(opt_local, "Creates local task (default).");

    /**
     * Pop subcommand.
     */
    auto* cmd_pop = app.add_subcommand(subcmd_pop, "Resolves task.")->alias("resolve");
    cmd_pop->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Log subcommand.
     */
    auto* cmd_log = app.add_subcommand(subcmd_log, "Logs (unresolved by default) tasks.");
    cmd_log->add_flag(opt_all, "Logs all tasks.");
    cmd_log->add_flag(opt_global, "Logs global tasks.");
    cmd_log->add_flag(opt_local, "Logs local tasks.");

    /**
     * Show subcommand.
     */
    auto* cmd_show = app.add_subcommand(subcmd_show, "Shows single task.");
    cmd_show->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Roll subcommand.
     */
    auto* cmd_roll = app.add_subcommand(subcmd_roll, "Rolls state by 1.");
    cmd_roll->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Rollb subcommand.
     */
    auto* cmd_rollback = app.add_subcommand(subcmd_rollb, "Rolls back state by 1.");
    cmd_rollback->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Amend subcommand.
     */
    auto* cmd_amend = app.add_subcommand(subcmd_amend, "Replaces tasks message.");
    cmd_amend->add_option(opt_vid_or_uid, "Task VID or UID.");
    cmd_amend->add_option(opt_message, "Message that will be written to the task.");
    cmd_amend->add_option(opt_type, "Task type (0 -> task, 1 -> bug, 2 -> feature).");
    cmd_amend->add_option(opt_worker, "Task worker.");

    /**
     * Take subcommand.
     */
    auto* cmd_take = app.add_subcommand(subcmd_take, "Takes (assigns) task to current user.");
    cmd_take->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Takeb subcommand.
     */
    auto* cmd_untake = app.add_subcommand(subcmd_takeb, "Takes back (unassigns) task.");
    cmd_untake->add_option(opt_vid_or_uid, "Task VID or UID.");

    /**
     * Assign subcommand.
     */
    auto* cmd_assign = app.add_subcommand(subcmd_assign, "Assigns task to worker.");
    cmd_assign->add_option(opt_vid_or_uid, "Task VID or UID.");
    cmd_assign->add_option(req_username, "username (current user by default).")->required(true);

    /**
     * Assignb subcommand.
     */
    auto* cmd_assignb = app.add_subcommand(subcmd_assignb, "Assigns back (unassigns) task from user.");
    cmd_assignb->add_option(opt_vid_or_uid, "Task VID or UID.");

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    /* clang-format on */

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
