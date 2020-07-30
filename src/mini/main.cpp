
#include "mini.h"

int main(int argc, const char** argv) {

    mini::Mini m;
    m.init(argc, argv);
    return m.exec();
}