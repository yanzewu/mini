#ifndef MINI_STREAM_H
#define MINI_STREAM_H

#include <string>
#include <iostream>

namespace mini {

    class OutputStream {
    public:

        unsigned buffer_size = 2048;
        std::string buffer;

        OutputStream() {}
        explicit OutputStream(unsigned buffer_size) : buffer_size(buffer_size){}


        OutputStream& operator<<(const std::string& obj) {
            buffer += obj;
            try_write_output();
            return *this;
        }

        OutputStream& operator<<(char obj) {
            buffer.push_back(obj);
            try_write_output();
            return *this;
        }

        OutputStream& operator<<(const char* obj) {
            return operator<<(std::string(obj));
        }

        OutputStream& operator<<(int obj) {
            return operator<<(static_cast<long>(obj));
        }

        OutputStream& operator<<(unsigned int obj) {
            return operator<<(static_cast<unsigned long>(obj));
        }

        OutputStream& operator<<(unsigned long obj) {
            buffer += std::to_string(obj);
            try_write_output();
            return *this;
        }

        OutputStream& operator<<(long obj) {
            buffer += std::to_string(obj);
            try_write_output();
            return *this;
        }

        OutputStream& operator<<(unsigned long long obj) {
            buffer += std::to_string(obj);
            try_write_output();
            return *this;
        }

        OutputStream& operator<<(float obj) {
            buffer += std::to_string(obj);
            try_write_output();
            return *this;
        }

        OutputStream& write_white(size_t n) {
            return operator<<(std::string(n, ' '));
        }

        OutputStream& write_lspace(const std::string& s, size_t width) {
            if (width > s.size()) write_white(width - s.size());
            return this->operator<<(s);
        }
        OutputStream& write_rspace(const std::string& s, size_t width) {
            operator<<(s);
            if (width > s.size()) write_white(width - s.size());
            return *this;
        }

        virtual void close() {
            flush();
        }

        virtual void flush() {}
        virtual ~OutputStream() {}

    protected:

        void try_write_output() {
            if (this->buffer.size() >= buffer_size) {
                this->flush();
                buffer.clear();
            }
        }
    };

    class StdoutOutputStream : public OutputStream {
    public:
        StdoutOutputStream() : OutputStream() {}

        explicit StdoutOutputStream(unsigned buffer_size) : OutputStream(buffer_size) {}

        void flush() {
            std::cout << buffer;
        }
        ~StdoutOutputStream() {
            flush();
        }
    };

    class StringOutputStream : public OutputStream {
    public:
        StringOutputStream() : OutputStream(UINT_MAX) {}

        const std::string& str()const {
            return this->buffer;
        }
    };

    // TODO implement open/close file

    class FileOutputStream : public OutputStream {
    public:
        std::ostream& output;

        FileOutputStream(std::ostream& output) : OutputStream(), output(output) {}

        void flush() {
            output << buffer;
            output.flush();
        }
        ~FileOutputStream() {
            flush();
        }
    };


    class StringAssembler {
    public:

        StringOutputStream ostream;

        StringAssembler() {}
        StringAssembler(const std::string& s) {
            ostream << s;
        }

        template<class T>
        StringAssembler& operator()(const T& obj) {
            ostream << obj;
            return *this;
        }
        const std::string& operator()() {
            return ostream.str();
        }
    };
}

#endif