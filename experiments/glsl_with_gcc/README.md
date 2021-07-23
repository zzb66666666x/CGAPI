# Compile GLSL At Runtime After Simple Parser
If you are familiar with the GLSL and OpenGL, you will know that shader is compiled at run time, which is pretty hard to do if you are wishing to implement it. Now, let's suppose you want realize the similar function (the execution of code will be on CPU of course), and you don't feel like creating your compiler (which is soooo hard!), maybe you can consider first translate the GLSL code to C++, then adopt the method in this project to compile it. 

## What I did

To compile and link at runtime, you should use operating system API `popen` to open another process (not thread!) to execute some bash script. Thanks god, GCC supports taking input from `stdin`, so you can literally pass some string to the gcc to compile, converting the source code (in the form of string) to dll. Then, if you want to use the dll, you can use Windows API to load it and get the address of a function you wanna execute.

Of course the similar toolchain can be transferred to Linux, in which the `dlopen` API can be used.

## Support GLSL
GLSL is pretty much the same with C++, by `using namespace glm`, you get direct access to the types like `vec3`, `mat4`. All you have to do is to extract enough information (eg. the layout keyword for vertex shader) from the glsl code, add some extra string like `#include <glm/glm.hpp>` to it, then gcc can accept it and compile it!

## YACC Parser 

With the help of flex&bison, we can define some general grammar rules for shader language. They are pretty much the same with C, so you just have to do some extra job to keep record of the `in`, `out`, `uniform`, `layout`. These are no more than extra keywords. 

Check out the `outcome.txt` to see how the parser translates the glsl to C++ code. 

The `test.y` is the main body of parser, which defines the grammar rules and also interfaces to parse a string or a file. As you walk through the code, you will find that I am trying to build up the `io_profile` of the shader language. the `io_profile` is a map between `key: name` and `value: {data_type, in_or_out, layout_id}`. With that map, the user program can know what data should be given to the shader before calling the shader main function. The parser, will also use the map to define interface functions to pass/fetch values to shader inner variables. The interface functions are listed below.

```cpp
__declspec(dllexport) void glsl_main();
__declspec(dllexport) void input_port(std::map<std::string, data_t>& indata);
__declspec(dllexport) void output_port(std::map<std::string, data_t>& outdata);
__declspec(dllexport) void input_uniform_dispatch(int idx, data_t data);
__declspec(dllexport) data_t output_uniform_dispatch(int idx);
```

 Note that the `data_t` is a union of difference data types, like vectors, matrices or float. The definition of data_t is given below. 

```C++
typedef union{
    glm::vec2 vec2_var;
    glm::vec3 vec3_var;
    glm::vec4 vec4_var;
    glm::mat2 mat2_var;
    glm::mat3 mat3_var;
    glm::mat4 mat4_var;
}data_t;
```

## GLSL IO

The glsl has three basic IO types, they are `in`, `out`, `uniform`. From my perspective, the `in` and `out` variables are those people want to define/fetch all together in a batch. For the uniform, people want to read or write to it with more freedom. In OpenGL, there are APIs like `glGetUniformLocation`, so people really cares about controlling the `uniform` variables in a flexible way. 

For all the `in`, `out` variables, the `input_port`, and `output_port` interface will take in charge. However, for the uniform variables, each one of them will own two functions (one for input, one for output). Then we get a bunch of IO interfaces for those `uniform` functions. How to manage them? Use a jump table. When parsing the code, I maintained a map called `uniform_map` which maps a function's name to its index in the jump table. So, user can pass in the index according to the `uniform_map`, the shader will use that index to find the correct function to execute. Check out the code below for details. 

```Cpp
mat4 projection;    // uniform
mat4 view;   		// uniform
mat4 model;     	// uniform

void set_uniform_model(data_t data){
    model = data.mat4_var;
}

data_t get_uniform_model(){
    return (data_t){.mat4_var = model};
}

void set_uniform_projection(data_t data){
    projection = data.mat4_var;
}

data_t get_uniform_projection(){
    return (data_t){.mat4_var = projection};
}

void set_uniform_view(data_t data){
    view = data.mat4_var;
}

data_t get_uniform_view(){
    return (data_t){.mat4_var = view};
}

set_uniform input_uniform_fmap[] = {set_uniform_model,set_uniform_projection,set_uniform_view};

get_uniform output_uniform_fmap[] = {get_uniform_model,get_uniform_projection,get_uniform_view};

void input_uniform_dispatch(int idx, data_t data){
    input_uniform_fmap[idx](data);
}

data_t output_uniform_dispatch(int idx){
    return output_uniform_fmap[idx]();
}
```

Note that when parser reads through the file, it can automatically generate the code for `input_uniform_fmap` and `output_uniform_fmap`. because all the interfaces are gives the pattern of name, which is `data_t get_uniform_ + name()` and `void set_uniform + name(data_t data)`.

