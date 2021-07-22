cmake_minimum_required(VERSION 3.1)
project (spamaker)

add_compile_options(-std=c++17)
add_compile_options(-Wall -Werror -Wno-noexcept-type)

add_executable (spamake main.cpp builder.cpp)
