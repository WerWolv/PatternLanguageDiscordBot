#include <dpp.h>
#include <pl.hpp>

#include <string>

#include <pl/patterns/pattern.hpp>
#include <wolv/utils/string.hpp>

using namespace std::literals::chrono_literals;

static std::vector<pl::u8> parseByteString(const std::string &string) {
    auto byteString = std::string(string);
    std::erase(byteString, ' ');

    if ((byteString.length() % 2) != 0) return {};

    std::vector<pl::u8> result;
    for (pl::u32 i = 0; i < byteString.length(); i += 2) {
        if (!std::isxdigit(byteString[i]) || !std::isxdigit(byteString[i + 1]))
            return {};

        result.push_back(std::strtoul(byteString.substr(i, 2).c_str(), nullptr, 16));
    }

    return result;
}

int main() {
    dpp::cluster bot(std::getenv("DISCORD_TOKEN"));
    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            /* Register the command */
            bot.guild_command_create(
                    dpp::slashcommand()
                            .set_type(dpp::ctxm_message)
                            .set_name("Run PL Code")
                            .set_application_id(bot.me.id),
                    789833418631675954
            );
        }
    });

    bot.on_message_context_menu([&](const dpp::message_context_menu_t &event) {
        if (event.command.get_command_name() == "Run PL Code") {
            event.thinking();

            pl::PatternLanguage runtime;
            runtime.setDangerousFunctionCallHandler([] { return false; });
            runtime.setIncludePaths({ std::getenv("INCLUDE_DIR") });

            runtime.removePragma("eval_depth");
            runtime.removePragma("array_limit");
            runtime.removePragma("pattern_limit");
            runtime.removePragma("loop_limit");

            runtime.addPragma("example", [](const pl::PatternLanguage &runtime, const std::string &value) {
                auto data = parseByteString(value);
                runtime.setDataSource(0, data.size(), [data](pl::u64 address, pl::u8 *buffer, pl::u64 size) {
                    std::memcpy(buffer, data.data() + address, size);
                });

                return true;
            });

            runtime.setDataSource(0, 0, [](pl::u64 address, pl::u8 *buffer, pl::u64 size) { });

            auto message = event.get_message().content;

            auto parts = wolv::util::splitString(message, "```");
            if (parts.size() != 3) {
                event.edit_original_response(dpp::message("Source code must be wrapped in a code block."));
                return;
            }


            if (parts[1].starts_with("pl\n")) parts[1].erase(0, 3);
            else if (parts[1].starts_with("rust\n")) parts[1].erase(0, 5);
            else if (parts[1].starts_with("cpp\n")) parts[1].erase(0, 4);
            else {
                event.edit_original_response(dpp::message("Source code must be wrapped in a `pl`, `rust` or `cpp` code block."));
                return;
            }

            runtime.addDefine("__PL_BOT__");

            bool success = runtime.executeString(parts[1], "<Source Code>");

            std::string consoleLog;
            runtime.setLogCallback([&consoleLog](auto level, auto message) {
                for (auto line : wolv::util::splitString(message, "\n")) {
                    if (!wolv::util::trim(line).empty()) {
                        switch (level) {
                            using enum pl::core::LogConsole::Level;

                            case Debug:     line = fmt::format("D: {}", line); break;
                            case Info:      line = fmt::format("I: {}", line); break;
                            case Warning:   line = fmt::format("W: {}", line); break;
                            case Error:     line = fmt::format("E: {}", line); break;
                            default: break;
                        }
                    }

                    consoleLog += line;
                    consoleLog += '\n';
                }
            });

            if (consoleLog.size() > 1000)
                consoleLog = "..." + consoleLog.substr(consoleLog.size() - 1000, consoleLog.size());

            std::string patternOutput;
            for (const auto &pattern : runtime.getPatterns()) {
                patternOutput += fmt::format("{} {} @ {} = {}\n", pattern->getTypeName(), pattern->getVariableName(), pattern->getOffset(), pattern->toString());
            }
            if (patternOutput.size() > 1000)
                patternOutput = "..." + patternOutput.substr(consoleLog.size() - 1000, patternOutput.size());

            std::string result;
            result += success ? "**Result**: `Success`" : "`Failure`";
            result += fmt::format("\n\n**Console Log**\n```\n{}\n```", consoleLog);
            if (!patternOutput.empty()) {
                result += fmt::format("\n\n**Patterns**\n```\n{}\n```", patternOutput);
            }

            event.edit_original_response(dpp::message(result));
        }
    });

    /* Start bot */
    bot.start(dpp::st_wait);

    return 0;
}
