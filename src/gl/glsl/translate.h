#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include <string>
#include <map>

#ifdef __cplusplus
extern "C"{
#endif

void cpp_code_generate(std::string& src, std::string& dest);

#ifdef __cplusplus
}
#endif

#endif