
#ifndef MINI_FILELOADER_H
#define MINI_FILELOADER_H

#include "errors.h"

#include <fstream>
#include <string>
#include <sstream>

namespace mini {

    class FileLoader {
    public:

        static void read_file(const std::string& filename, std::string& buffer) {
            std::ifstream fp(filename);
            if (!fp.is_open()) {
                throw IOError("Cannot open file: " + filename);
            }
            std::stringstream sbuffer;
            sbuffer << fp.rdbuf();
            buffer = sbuffer.str();
            /*
            fp.seekg(0, std::ios::end);
            size_t fp_size = fp.tellg();

            buffer.resize(fp_size);
            fp.seekg(0);

            fp.read(&buffer[0], fp_size);
            std::cout << buffer;*/
        }

    };

}

#endif