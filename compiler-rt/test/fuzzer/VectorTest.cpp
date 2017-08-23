#include<vector>

#include<stdlib.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* buffer, size_t length) {
    std::vector<uint8_t> bb(100 * 10);
    return 0;
}
