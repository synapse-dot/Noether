#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

static ProgramNode parseSource(const std::string& src) {
    Lexer lexer(src);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    return parser.parse();
}

int main() {
    int failures = 0;

    // valid v2-style directives parse
    try {
        const std::string valid = R"(
            system spring_mass {
              physical {}
              math {}
              simulate {
                integrator rk4
                timestep 0.01 [T]
                duration 10.0 [T]
                export "spring_mass.csv"
              }
              visualize {
                scene "spring_mass_oscillator"
                plot x vs t
                plot v vs t
              }
            }
        )";
        ProgramNode p = parseSource(valid);
        if (p.systems.size() != 1 || !p.systems[0].simulate || !p.systems[0].visualize) {
            std::cerr << "[FAIL] valid v2 parse: missing simulate/visualize blocks\n";
            failures++;
        }
    } catch (const ParseError& e) {
        std::cerr << "[FAIL] valid v2 parse threw ParseError: " << e.what() << "\n";
        failures++;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] valid v2 parse threw exception: " << e.what() << "\n";
        failures++;
    }

    // unknown directive in simulate should fail clearly
    try {
        const std::string badDirective = R"(
            system spring_mass {
              simulate {
                wrongthing nope
              }
            }
        )";
        (void)parseSource(badDirective);
        std::cerr << "[FAIL] unknown simulate directive did not fail\n";
        failures++;
    } catch (const ParseError& e) {
        if (std::string(e.what()).find("Unknown directive") == std::string::npos) {
            std::cerr << "[FAIL] unknown directive error not clear: " << e.what() << "\n";
            failures++;
        }
    }

    // malformed visualize plot should fail clearly
    try {
        const std::string malformed = R"(
            system spring_mass {
              visualize {
                plot x t
              }
            }
        )";
        (void)parseSource(malformed);
        std::cerr << "[FAIL] malformed visualize block did not fail\n";
        failures++;
    } catch (const ParseError& e) {
        if (std::string(e.what()).find("'vs'") == std::string::npos) {
            std::cerr << "[FAIL] malformed visualize error not clear: " << e.what() << "\n";
            failures++;
        }
    }

    if (failures > 0) {
        std::cerr << "Parser tests failed: " << failures << "\n";
        return 1;
    }

    std::cout << "All parser tests passed.\n";
    return 0;
}
