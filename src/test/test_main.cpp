#include "../mini/mini.h"
#include "../external/cxxopts.hpp"

#include <fstream>

using namespace mini;

void test_parser(const std::string& filename, std::ostream& output) {

    std::vector<Ptr<AST>> ast_buffer;
    std::string buffer;
    std::vector<Token> token_buffer;

    mini::FileLoader::read_file(filename, buffer);
    mini::Lexer lexer;
    mini::Parser parser;

    lexer.tokenize(buffer, token_buffer, 0);
    parser.parse(token_buffer, ast_buffer);

    FileOutputStream os(output);

    for (const auto& a : ast_buffer) {
        os << *a << "\n";
    }
    os.close();
}

void test_semantic(const std::string& filename, std::ostream& output) {

    CompilerFrontEnd front_end;
    front_end.initialize({});
    front_end.load_file(filename);
    front_end.parse();
    front_end.attribute();

    FrontEndDisplayer displayer(front_end, output);
    displayer.display();
}


void test_frontend(const std::string& filename, std::ostream& output) {
    CompilerFrontEnd front_end;
    IRProgram irprog;
    front_end.initialize({});
    front_end.load_file(filename);
    front_end.process(irprog);

    FileOutputStream os(output);
    irprog.print_full(os);
    os << '\n';
}

int main(int argc, char** argv) {

    cxxopts::Options options("test", "Integrated test for mini");

    options.add_options()
        ("m,mode", "Test Mode", cxxopts::value<std::string>()->default_value("parse"))
        ("o,output", "Output", cxxopts::value<std::string>()->default_value(""))
        ("i,input", "Input file", cxxopts::value<std::string>());

    auto opts = options.parse(argc, argv);
    
    std::ofstream outfile;
    bool has_outfile = !opts["output"].as<std::string>().empty();
    if (has_outfile) {
        outfile.open(opts["output"].as<std::string>());
    }

    if (opts["mode"].as<std::string>() == "parse") {
        test_parser(opts["input"].as<std::string>(), has_outfile ? outfile : std::cout);
    }
    else if (opts["mode"].as<std::string>() == "semantic") {
        test_semantic(opts["input"].as<std::string>(), has_outfile ? outfile : std::cout);
    }
    else if (opts["mode"].as<std::string>() == "frontend") {
        test_frontend(opts["input"].as<std::string>(), has_outfile ? outfile : std::cout);
    }

    if (outfile.is_open()) outfile.close();

    return 0;

}