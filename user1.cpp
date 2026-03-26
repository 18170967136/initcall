#include <iostream>
#include "auto_init.h"   // 引入注册框架

void user_init() {
    std::cout << "  -> user_init initialized\n";
}
REGISTER_MODULE(user_init, "user_init", 5)