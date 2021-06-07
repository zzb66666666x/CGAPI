# Log

#### 2021.5.26

一些问题：

- 编译出一个能用的dll？
- 接口长什么样
- 能否跑一个demo或者改装一下games101的作业3
- 怎么为指令集加速，多线程流出改造空间

基本任务：

- 简易图片渲染结果
- 改造mesa/GLFW的函数，换成自己的API

- 学习OpenGL的规范

- Cpp实现着色器？

#### 2021.6.3

目标：

- 希望模拟一个gl_context
- 实现一个类似glX的窗口，用opencv即可，利用gl_context避免拷贝frame_buffer的开销
- 矩阵栈glPushMatrix的加入用来管理坐标？需要么？
- 考虑纹理，代码框架需要支持渲染到纹理，支持glFramebufferTexture2D

关于实现gl_context:

- gl核心库的绘制依赖于struct gl_context，内部包含framebuffer，用户buffer，attributes，总之包含一切渲染依靠的数据结构
- gl核心库遵循如下函数调用逻辑：glSomeOperation(params) -> implementation(context, params)，gl需要主动获取当前context，然后在这个数据结构中进行操作
- 定义许多glObject以及其派生类，用glManager类管理，采用哈希表保存各种glObject，包括VBO，VAO，而所有的glManager都被保存在context中
- gl_context的创建参照mesa的实现，由一个额外的组件glX（gl + X window on Linux）提供类似glXCreateContext，glXMakeCurrent之类的API，用来在gl核心库中创建context，并定义一个全局指针变量指向context struct，然后glGenBuffers之类的核心API会按照这个指针在context里面分配各种东西
- glX包暂时打算用opencv来绘制窗口的话，那就取名glCV好了
- 未来可以多写一个类似GLFW的库，称其为GLF，即GL FrontEnd，可以包装glX的API，封装create context的一系列操作，改为提供create window之类的API，最终我们的包的使用者的代码会非常简洁

关于mesa源码：

- 阅读notes文件夹下的mesa gl_context.md，里面记录了看代码的辛酸历程。。。

关于着色器语言的支持：

- 好像还是没有什么很好的办法。。。

