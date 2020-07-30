#ifndef MINI_FRONTEND_H
#define MINI_FRONTEND_H

#include "fileloader.h"
#include "lexer.h"
#include "parser.h"
#include "attributor.h"
#include "ircodegen.h"
#include "dependency.h"
#include "builtin.h"

#include <ostream>

#include <filesystem>

namespace mini {

    class CompilerFrontEnd {
    public:

        CompilerFrontEnd() {}

        void initialize(const std::vector<std::string>& search_paths) {
            for (const auto& path : search_paths) {
                paths.push_back(std::filesystem::path(path));
            }
            paths.push_back(std::filesystem::current_path());
            symbol_table.initialize(&dependency_resolver);
            BuiltinSymbolGenerator::generate_builtin_types(symbol_table);
            BuiltinSymbolGenerator::generate_builtin_functions(symbol_table);
        }

        void process(IRProgram& ir_program) {
            parse();
            attribute();
            generate_ir(ir_program);
        }

        // load the string as command
        void load_string(const std::string& str) {
            input_from_file = false;
            filename_main = str;
        }

        void load_file(const std::string& filename) {
            input_from_file = true;
            filename_main = filename;
        }

        void parse() {
            parse_and_process_dependency(filename_main);
            dependency_resolver.spread_dependency();
        }

        void attribute() {
            attributor.process(nodes, symbol_table);
        }

        void generate_ir(IRProgram& ir_program) {
            ircodegenerator.process(nodes, symbol_table, ir_program, filenames);
        }

        // Parse and denendency resolve
        unsigned parse_and_process_dependency(const std::string& filename) {

            // The algorithm is that, once we find an 'import', we lookup the file. If
            // the file is not parsed, we parse it and copy all of its parsed ASTs.
            // Therefore the global AST buffer would contain each file exactly once,
            // according to their first import order.
            // To prevent references of undefined global variables, we keep a record of imports 
            // in DependencyChecker to determine which variables are 'seenable' at certain points
            // by chasing along its dependency chain, excluding itself.
            // Note: the duplicating definition in same/different file can always be found.


            std::string full_filename;
            bool need_to_pop_path = input_from_file;

            if (input_from_file) {
                check_file(filename, full_filename);            
                if (filename_backmap.count(full_filename) != 0) {
                    return filename_backmap[full_filename];
                }
                paths.push_back(std::filesystem::weakly_canonical(std::filesystem::path(filename).parent_path())); // add path for relative import.
            }
            else {
                full_filename = "<input>";
            }

            unsigned filename_id = filename_backmap.size();
            filename_backmap[full_filename] = filename_id;
            filenames.push_back(full_filename);
            dependency_resolver.insert(filename_id);

            std::vector<pAST> cur_ast_buffer;

            if (input_from_file) {
                parse_file_to_ast(full_filename, cur_ast_buffer, filename_id);  // parse the file
            }
            else {
                parse_string_to_ast(filename, cur_ast_buffer);
                input_from_file = true;     // the other sources come from import so must be files.
            }

            for (const pAST& node : cur_ast_buffer) {
                switch (node->get_type())
                {
                case AST::IMPORT:
                {
                    try {
                        unsigned other_fid = parse_and_process_dependency(
                            node->as<ImportNode>()->get_filename() + ".mini");
                        dependency_resolver.connect(filename_id, other_fid);    // record the import order
                    }
                    catch (const IOError& e) {
                        node->get_info().throw_exception(e.what());             // translate to parsing error, easier to locate.
                    }
                    break;
                }
                case AST::CLASS: {
                    auto m_node = ast_cast<ClassNode>(node);
                    symbol_table.insert_type(m_node->symbol, TypeMetaData::CUSTOM);
                    nodes.push_back(node);
                    break;
                };
                default: nodes.push_back(node); break;
                }

            }

            if (need_to_pop_path) paths.pop_back();

            return filename_id;
        }

        // check and return the absolute form of filename
        void check_file(const std::string& filename, std::string& canonical_name) {

            std::filesystem::path full_filename;

            bool found = false;

            for (auto p = paths.rbegin(); p != paths.rend(); p++) {
                try {
                    full_filename = std::filesystem::canonical(*p / filename);
                }
                catch (const std::filesystem::filesystem_error&) {
                    continue;
                }
                found = true;
                break;
            }
            if (!found) {
                throw IOError("Cannot open file " + filename);
            }
            canonical_name = full_filename.string();
        }

        void parse_file_to_ast(const std::string& filename, std::vector<Ptr<AST>>& ast_buffer, unsigned file_no) {

            std::string buffer;
            std::vector<Token> token_buffer;
            
            FileLoader::read_file(filename, buffer);
            lexer.tokenize(buffer, token_buffer, file_no);
            parser.parse(token_buffer, ast_buffer);
        }

        void parse_string_to_ast(const std::string& str, std::vector<Ptr<AST>>& ast_buffer) {
            std::vector<Token> token_buffer;

            lexer.tokenize(str, token_buffer, 0);
            parser.parse(token_buffer, ast_buffer);
        }

        // get the filename table
        const std::vector<std::string>& filename_table()const {
            return this->filenames;
        }

        const SymbolTable& sym_table()const {
            return this->symbol_table;
        }

    private:
        friend class FrontEndDisplayer;

        SymbolTable symbol_table;
        std::vector<pAST> nodes;

        DependencyChecker dependency_resolver;
        Lexer lexer;
        Parser parser;
        Attributor attributor;
        IRCodeGenerator ircodegenerator;

        bool input_from_file = true;    // false means input from string.
        std::string filename_main;

        std::vector<std::filesystem::path> paths;
        std::vector<std::string> filenames;
        std::unordered_map<std::string, unsigned> filename_backmap;
    };


    class FrontEndDisplayer {
    public:

        FileOutputStream output;
        const CompilerFrontEnd& front_end;
        
        FrontEndDisplayer(const CompilerFrontEnd& cf, std::ostream& output_) : 
            output(output_),
            front_end(cf) {

        }

        void display() {
            output << "-------- AST --------\n";
            for (const auto& ast : front_end.nodes) {
                output << *ast;
            }
            output << "\n-------- Symbol Table --------\n";
            output << front_end.symbol_table << "\n";
            output.close();
        }
    };

}

#endif // !FRONTEND_H
