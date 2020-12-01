#include <clipp.h>
#include <cstdlib>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iostream>
#include <poac/cmd.hpp>
#include <poac/core/except.hpp>
#include <poac/io/term.hpp>
#include <string>
#include <optional>

enum class subcommand {
    nothing,
    init,
    _new,
    help,
    version,
};

enum class option {
    nothing,
    verbose,
    quiet,
};

[[nodiscard]] int
no_such_command(const int& argc, char* argv[], const clipp::group& cli) {
    fmt::print(
        std::cerr,
        "{}: no such command: `{}`\n\n{}\n",
        poac::io::term::error,
        fmt::join(argv + 1, argv + argc," "),
        clipp::usage_lines(cli, "poac")
    );
    return EXIT_FAILURE;
}

template <typename T>
int
optional_to_int(const std::optional<T>& opt) {
    if (opt.has_value()) {
        fmt::print(
            std::cerr,
            "{}: {}\n",
            poac::io::term::error, opt->what()
        );
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

int
main(const int argc, char* argv[]) {
    subcommand subcmd = subcommand::nothing;
    option opt = option::nothing;

    auto init_opts = poac::cmd::init::Options {
        poac::cmd::_new::ProjectType::Bin
    };
    const clipp::group init_cmd =
        ( clipp::command("init")
             .set(subcmd, subcommand::init)
             .doc("Create a new poac package in an existing directory")
        , ( clipp::option("--bin", "-b")
                .doc("Use a binary (application) template [default]")
          | clipp::option("--lib", "-l")
                .set(init_opts.type, poac::cmd::_new::ProjectType::Lib)
                .doc("Use a library template")
          )
        );

    auto new_opts = poac::cmd::_new::Options {
        poac::cmd::_new::ProjectType::Bin,
        ""
    };
    const clipp::group new_cmd =
        ( clipp::command("new")
            .set(subcmd, subcommand::_new)
            .doc("Create a new poac package at <path>")
        , clipp::word("path", new_opts.package_name)
        , ( clipp::option("--bin", "-b")
                .doc("Use a binary (application) template [default]")
          | clipp::option("--lib", "-l")
              .set(new_opts.type, poac::cmd::_new::ProjectType::Lib)
              .doc("Use a library template")
          )
        );

    const clipp::parameter help_cmd =
        clipp::command("help")
            .set(subcmd, subcommand::help)
            .doc("Print this message");

    const clipp::parameter version_cmd =
        clipp::command("version")
            .set(subcmd, subcommand::version)
            .doc("Show the current poac version");

    const clipp::group cli = (
        ( clipp::option("--help", "-h")
            .set(subcmd, subcommand::help)
            .doc("Print this message or the help of the given subcommand(s)")
        , clipp::option("--version", "-V")
            .set(subcmd, subcommand::version)
            .doc("Show the current poac version")
        , clipp::option("--verbose", "-v")
            .set(opt, option::verbose)
            .doc("Use verbose output")
        , clipp::option("--quiet", "-q")
             .set(opt, option::quiet)
             .doc("No output printed to stdout")
        ) |
        ( init_cmd
        | new_cmd
        | help_cmd
        | version_cmd
        )
    );

    if (argc == 1) {
        std::cout << clipp::usage_lines(cli, "poac") << std::endl;
        return EXIT_SUCCESS;
    } else if (clipp::parse(argc, argv, cli)) {
        switch (subcmd) {
            case subcommand::nothing:
                return no_such_command(argc, argv, cli);
            case subcommand::init:
                return optional_to_int(poac::cmd::init::exec(std::move(init_opts)));
            case subcommand::_new:
                return optional_to_int(poac::cmd::_new::exec(std::move(new_opts)));
            case subcommand::help:
                std::cout << clipp::make_man_page(cli, "poac");
                return EXIT_SUCCESS;
            case subcommand::version:
                return optional_to_int(poac::cmd::version::exec());
        }
    } else {
        return no_such_command(argc, argv, cli);
    }
}