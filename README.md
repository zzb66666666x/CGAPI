# OpenGL on CPU

[toc]

## Intro

This project is a realization of OpenGL running on CPU (soft rendering pipeline) with its GUI support from OpenCV. The goal of this library is to provide a set of API to quickly test out your ideas in computer graphics, which is easy enough to use, compile and link. The whole system is not based on GPU, but with multi-core processors nowadays, the efficiency is still acceptable. The advantage of building a system on CPU is that it's simple enough, and totally flexible to customize and debug. I also believe that this project can also be great tool to get new comers to get familiarized with the OpenGL low level features. OpenGL and other graphics API are often realized by graphics card producers, and current open source OpenGL realizations are sometimes too complicated for people to read and learn (like mesa3D). So in this library, I only keep the core logic of a graphics pipeline, which should be easy to read. That's why I call this library `glite`, which is `gl` +  `lite`. 

The whole set of API provides nearly all the OpenGL functions, and a shader language parser is also supported, so you can pass in a string and let the library to compile it for you. Unlike the real OpenGL compiler, this compiler built inside is more like a front end (with the help of YACC), its function is to detect the grammar, and translate it to C++ code. Then the code is compiled into `.dll` file by `g++`. The `glite` can than extract function pointers from it to realize the programmable rendering pipeline. 

## Build

First, you should make sure you have GCC and CMake. We chose C++ 17 in this project, but C++11 can also work. The recommended platform for this library is Windows. I also tried to make it cross platform, but we do not have enough time to fully test it though. 

To make the GUI work, you should also have OpenCV installed. We only use it for GUI and window creation. For Windows users with MinGW toolchain, you are highly recommended to compile the OpenCV. My own version of OpenCV was released in the May of 2020. 

This project also relies on GLM as math package support. 

The Google benchmark are used for testing performance, which is totally unrelated with the library itself, you can comment out some executable targets. 

The VCPKG is a tool from Microsoft to manage the dependencies, If you don't use it, that's fine. You can comment it out and configure the dependency yourself (like adding paths to environment variables). 

The boost library is used, but not much, we only use its `<boost/numeric/interval.hpp>` for id generation and deletion. Since the boost is almost header only, its configuration should be simple. 

For multi-threading, we have two versions, one use `pthread`, the other one is `openMP`. They always com with the compiler, so it's not a big problem. 

Don't forget to change the `CMakelists.txt`  to redirect the `OpenCV_DIR`, and other path variables. 

Once you have the dependency resolved. you can compile the source code.

```
# at project workdir
mkdir build
cd build
cmake -G"MinGW Makefiles" ..
make -j4
make install
```

## File Structure

```
./experiments		# my own experiments about YACC
./include			# public headers
	/gl				# gl related API
	/glv			# display the result to window/file
./src				# source code
	/glv			# source code of GUI operation and store frame as bmp format
	/gl				# gl source code 
		/glsl		# glsl parser and inner support files
	/utests			# unit tests
	/utils			# some helper code
./scripts			# python scripts to help extract string code from files
./install			# after you make install, you will get this
./notes				# notes written during preparation  
./resources			# models, texture images
./reference			# reference files
./shader			# glsl shader code
./tests				# user testing code (using glite.dll)
```

## Design Choices

#### Basics

Everything related with the rendering tasks are kept in the object of `gl_context`. In this object, we have objects for vertex data, frame buffer, depth buffer, rendering payload, shader, pipeline and some bool flags. 

OpenGL supports `VBO`, `VAO`, `EBO` and other built in objects for storage. We also define them to be class `glStorage<class T>` in file `globj.h / globj.cpp`. We use class `glManager` to map key value to these storage objects. If you are familiar with OpenGL, you will also know that the GL system is more like a state machine, so current objects belong to `GL_ARRAY_BUFFER` or `GL_TEXTURE_2D` are those actually participating in the rendering process. So the class `glPipeline` is like a place to share data structures between different stages of rendering pipeline and the `glRenderPayload` keeps tracks of the current object binding to `GL_ARRAY_BUFFER`, `GL_FRAMEBUFFER`, and `GL_TEXTURE_2D` and things like that. 

In `src/gl/render.cpp`, we write many functions for rendering,  but few of them are really useful. It's because we are always testing different approaches to render the scene. So we really want our pipeline to be modularized. So we put data structures shared between rendering functions into `glPipeline` (like a list of triangles), and we forced a uniform function signature which is `void func(void)`. That way we can define a function pointer like below. 

```C
typedef void (*render_fp)();
```

When we want to test something out, we just put the function pointers we want inside `glPipeline::exec` (which is defined as `std::list<render_fp> exec`). As you can see, our final choices (maybe changed in the future) are `programmable_process_geometry_openmp` and `programmable_rasterize_with_shading_openmp`. 

#### GLSL Parser By YACC

The basic logic behind the YACC is the *Backus-Naur form*, by which we can define the grammar rules recursively. The basic components of a parser consists of a `.y` file for grammar rules and a `.l` file to define words and tokens. The tool to process `.l` file is called the `lexer`, we use `flex` from GNU to compile our lexer, the output is by default called `lex.yy.c`. For the `.y` file, we use `bison` (also from GNU) to generate grammar rules written in C code, and they are by default `y.tab.c / y.tab.h`.  We use Makefile to control this process.

```makefile
parser: grammar.y lexer.l
	bison --yacc -dv grammar.y
	flex lexer.l

clean:
	rm *.tab.c
	rm *.tab.h
	rm *.yy.c
	rm *.output
```

With those files generated, we can add them to the source list of our graphics library project. Then the rest of the problem becomes how can we use that parser inside our project. Since `bison` enable us to define `C code actions` when some specific grammar patterns are matched, so we can keep track of which variables for the glsl code are `in / out`, which are `uniform`, and which should be initialized by pipeline (by the `layout` keyword). For each of the shader inner variables, the parser will automatically generate its own `set / get` functions. Our pipeline can control the shader by those interface functions. 

To implement IO functionalities with shader inner variables, we can wrap everything defined in the shader in a class called `GLSLShader` which is a derived class of `ShaderInterface`. 

```C++
union data_t{
    glm::vec2 vec2_var;
    glm::vec3 vec3_var;
    glm::vec4 vec4_var;
    glm::mat2 mat2_var;
    glm::mat3 mat3_var;
    glm::mat4 mat4_var;
    union{
        int sampler2D_var;
        int int_var;
    };
};

class ShaderInterface{
    public:
    virtual ~ShaderInterface(){}
    virtual void glsl_main() = 0;
    virtual void input_port(std::map<std::string, data_t>& indata) = 0; 
    virtual void output_port(std::map<std::string, data_t>& outdata) = 0;
    virtual void input_uniform_dispatch(int idx, data_t& data) = 0; 
    virtual data_t output_uniform_dispatch(int idx) = 0; 
    virtual void set_inner_variable(int variable, data_t& data) = 0;
    virtual void get_inner_variable(int variable, data_t& data) = 0;
    virtual void set_sampler2D_callback(get_sampler2D_data_fptr func) = 0;
};
```

Then you can see that, we pass value to shader variables by either a `data_t` or a `std::map` of it. That way, we can pass different types of data packed in an `union`. Also note that the names of members in the union are encoded with a specific pattern `type_var`. With this trick, our parser can easily generate the `set / get` code by concatenating the string for some variable type with the `"_var"`. Let's say our shader defines `uniform vec3 lightpos`, our parser can then automatically generate code like below.

```C++
void set_uniform_lightpos(data_t data){
    lightpos = data.vec3_var;
}
data_t get_uniform_lightpos(){
    return (data_t){.vec3_var = lightpos};
}
```

After defining those classes, we can define the final interface function between the shader and the pipeline to be one `create` functions to return an object of `GLSLShader`, and one `destroy` function to free the allocated space. 

```C++
ShaderInterface* create_shader_inst(){
    ShaderInterface* ret = new GLSLShader;
    return ret;
}
void destroy_shader_inst(ShaderInterface* inst){
    delete inst;
}
```

One advantage of wrapping shader functions into a class is that it's perfect for multi-threading. Since different threads will define there own input variables to shader, then execute the shader main function, then fetch the output, it would be great that each thread can has its own shader instance (no share variables between threads), so that we won't worry about thread safety and locking, and the performance can be guaranteed. 

There are other implementation details about inner variables like `gl_Position` and `gl_FragColor`, I won't cover them all in this readme. 

## Off Screen Rendering

The off screen rendering is another important feature of OpenGL. Let's take the shadow mapping as an example. By changing the framebuffer and redirecting the renderer output to texture, we can get a depth map from the perspective of light source. Then in the second pass of rendering pipeline, we can know which region are in shadow. 

After small modification of code from [Learn OpenGL, extensive tutorial resource for learning Modern OpenGL](https://learnopengl.com/), we realized the shadow mapping. Check this example out: [zzb66666666x/shadow-mapping-with-self-written-OpenGL: Example of using my OpenGL on CPU project (github.com)](https://github.com/zzb66666666x/shadow-mapping-with-self-written-OpenGL) .

Currently, we only allow rendering to z-buffer, future updates will make this feature complete. 

## Contact Us

Author#1: Zhu Zhongbo from ZJU-UIUC Institute

```
Email: Zhongbo.18@intl.zju.edu.cn
```

Author#2: Zheng Juncheng from ZJU, graduate school of software engineering

```
Email: 
```

