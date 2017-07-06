#ifndef ATL_UTILS_FILE_HPP
#define ATL_UTILS_FILE_HPP

#include <stdio.h>

#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace atl {

bool file_exists(const std::string &name);
std::vector<std::string> path_split(const std::string path);
void paths_combine(const std::string path1,
                   const std::string path2,
                   std::string &out);

}  // end of atl namepsace
#endif
