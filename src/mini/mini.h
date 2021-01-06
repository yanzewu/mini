
#ifndef MINI_H
#define MINI_H

#include "frontend.h"
#include "vm.h"

#include <cstdio>
#include <string>

namespace mini {

    class Mini {
    public :
        enum class Mode {
            COMPILE = 0,
            EXEC = 1,
            COMPILE_EXEC = 2
        };
        
        std::string arg;
        bool execute_from_file = true;
        Mode mode = Mode::COMPILE_EXEC;

        CompilerFrontEnd frontend;
        VM vm;
        IRProgram irprog;
        
        Mini() {

        }

        void init(int argc, const char** argv) {

            std::vector<std::string> search_paths;
            search_paths.push_back((std::filesystem::path(argv[0]).parent_path() / "lib").string());

            char* minipath = std::getenv("MINIPATH");
            if (minipath) {
                char* b = minipath, *e = minipath + 1;
                while (*e != 0) {
                    if (*e == ':') {
                        search_paths.push_back(std::string(b, e));
                        b = e + 1;
                    }
                    e++;
                }
            }

            if (argc < 2 || strcmp(argv[1], "-h") == 0) {
                std::cout << "Usage: mini [option] ... [-e command | filename]\n";
                std::cout << "Avaiable options are:\n";
                std::cout << "  -c        : Compile the program to bytecodes instead of evaluating it.\n";
                std::cout << "  -e command: Execute command directly.\n";
                std::cout << "  -p        : Run from bytecodes.\n";
                std::cout << "  -v        : Verbose.\n";
                std::cout << std::endl;
                exit(0);
            }
            else {
                for (int i = 1; i < argc; i++) {
                    if (strcmp(argv[i], "-e") == 0) {
                        execute_from_file = false;
                    }
                    else if (strcmp(argv[i], "-c") == 0) {
                        mode = Mode::COMPILE;
                    }
                    else if (strcmp(argv[i], "-p") == 0) {
                        mode = Mode::EXEC;
                    }
                    else {
                        arg = argv[i];
                    }
                }
            }

            frontend.initialize(search_paths);
        }

        int exec() {

            if (mode == Mode::COMPILE || mode == Mode::COMPILE_EXEC) {
                int ret;
                try {
                    if (execute_from_file) {
                        frontend.load_file(arg);
                    }
                    else {
                        frontend.load_string(arg);
                    }
                    ret = frontend.process(irprog);
                }
                catch (const IOError& e) {
                    StdoutOutputStream output;
                    e.print(output) << '\n';
                    return 1;
                }
                catch (const ParsingError& e) {
                    StdoutOutputStream output;
                    e.print(frontend.filename_table(), output) << '\n';
                    return 1;
                }
                if (ret != 0) return 1;
                if (mode == Mode::COMPILE) {
                    // dump the ir (should be binary, but here I use text for debugging)
                    StdoutOutputStream output;
                    irprog.print_full(output);
                }
            }
            else if (mode == Mode::EXEC) {
                // load the ir
            }

            if (mode == Mode::COMPILE_EXEC || mode == Mode::EXEC) {
                vm.load(irprog);
                vm.run();
                return vm.error_flag;
            }
            
            
            return 0;
        }
    };

}

#endif // !MINI_H
