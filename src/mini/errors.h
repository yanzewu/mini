#pragma once

#ifndef MINI_ERRORS_H
#define MINI_ERRORS_H

#include <stdexcept>
#include <vector>

#include "stream.h"

namespace mini {

    class IOError : public std::runtime_error {
    public:

        IOError(const std::string& msg) : runtime_error(msg) {}

        OutputStream& print(OutputStream& os)const {
            os << "IOError: " << this->what();
            return os;
        }
    };

    class ParsingError : public std::runtime_error {
    public:

        unsigned srcno = 0;
        unsigned lineno = 0;
        unsigned colno = 0;

        explicit ParsingError(const std::string& msg) : runtime_error(msg) {}

        explicit ParsingError(const std::string& msg, unsigned fileno, unsigned lineno, unsigned colno) : runtime_error(msg),
            srcno(fileno), lineno(lineno), colno(colno) {}

        OutputStream& print(const std::vector<std::string>& filenames, OutputStream& os)const {
            os << "File \"" << filenames[srcno] << "\", line " << lineno + 1 << ", column " << colno + 1 << '\n';
            os << "ParsingError: " << this->what();
            return os;
        }

        OutputStream& print_as_warning(const std::vector<std::string>& filenames, OutputStream& os)const {
            os << filenames[srcno] << ':' << lineno + 1 << ": Warning: " << this->what();
            return os;
        }
    };

    class RuntimeError : public std::runtime_error {
    public:
        bool is_custom;
        unsigned int data = 0;      // the address at constant pool

        explicit RuntimeError(const std::string& msg) : runtime_error(msg), is_custom(false) {}
        explicit RuntimeError(unsigned int data) : runtime_error(""), is_custom(true), data(data) {}
    };

    // Just a host of warnings
    class Warning {
    public:

        static std::vector<ParsingError> error_list;

        static void warn(const std::string& msg, unsigned fileno, unsigned lineno, unsigned colno) {
            error_list.emplace_back(msg, fileno, lineno, colno);
        }

        static void warn(const ParsingError& error) {
            error_list.push_back(error);
        }

        // print and clear warning list.
        static void print_all_warnings(const std::vector<std::string>& filename_table, OutputStream& os) {
            for (const auto& e : error_list) {
                e.print_as_warning(filename_table, os) << '\n';
            }
        }
    };


    class ErrorManager {
    public:

        int error_uplimit;
        int error_hold = 0;
        std::vector<std::string>* filenames;
        StdoutOutputStream output;

        void reset() { error_hold = 0; }
        bool has_error()const { return error_hold > 0; }
        bool has_enough_errors()const { return error_hold > error_uplimit; }
        void count_and_print_error(const ParsingError& e) { 
            e.print(*filenames, output);
            output << '\n';
            error_hold++;
            output.flush();
        }
    };

}

#endif