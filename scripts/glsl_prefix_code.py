# this file is to extract the prefix code for glsl translation

# headers.push_back(string("../src/gl/glsl/os.h"));
# headers.push_back(string("../src/gl/formats.h"));
# headers.push_back(string("../include/gl/common.h"));
# headers.push_back(string("../src/gl/glsl/vec_math.h"));
# headers.push_back(string("../src/gl/glsl/inner_variable.h"));
# headers.push_back(string("../src/gl/glsl/inner_support.h"));

files = ["../src/gl/glsl/os.h", "../src/gl/formats.h", "../include/gl/common.h", \
         "../src/gl/glsl/vec_math.h", "../src/gl/glsl/inner_variable.h", "../src/gl/glsl/inner_support.h"]

code = []

for file in files:  
    fp = open(file, "r")
    for line in fp.readlines():
        tmp = line.strip(" ")
        if tmp == "\n":
            continue
        code.append(tmp)
    fp.close()
    code.append("\n")

output = open("output.txt", "w")
# print("".join(code))
output.write("".join(code))
output.close()

