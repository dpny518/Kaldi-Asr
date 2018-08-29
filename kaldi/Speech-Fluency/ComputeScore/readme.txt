
部署需要注意如下：
1.修改CMakeList.txt中库，头文件依赖路径(这里是dep路径)，修改后进入build目录执行如下命令：
    1)cmake .
    2)make
    这里就可以编译出so
2.编译的so，和dep文件夹拷贝到docker/app即可作为打包工作目录
3.修改dep/model/ModelConfig/ 目录下：
    ivector_extractor.conf
    online.conf
    中配置文件路径为/app/dep/model/* 等

***************************************************************************************
开发者：
1.代码结构说明
ComputeScore为程序源码目录，同时我们程序依赖的第三方头文件，静态库，动态库以及模型文件，
在共享服务器存放。
如代码目录如下：ComputeScore为代码目录，dep为依赖目录
    /code
        --ComputeScore
        --dep
2.修改模型配置文件等路径
     dep/model/ModelConfig/ivector_extractor.conf
     dep/model/ModelConfig/online.conf
     配置文件中需要绝对路径，所以这里文件中路径需要改。


3.编译代码：
修改CMakeList.txt中库，头文件依赖路径(这里是dep路径)，修改后进入build目录执行如下命令：
    1)cmake .
    2)make
    这里就可以编译出so
4.运行：
将bin目录下config.ini文件中相关模型路径配置好即可