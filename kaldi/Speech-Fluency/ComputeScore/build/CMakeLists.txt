cmake_minimum_required (VERSION 2.6)
set(CMAKE_BUILD_TYPE "Debug")  

project (computer-score)

#depend libs path
set(LIB_ROOT /home/dinghj/testcode/code/dep)


# Settings
add_compile_options(-std=c++11 -fPIC -lstdc++ -pthread)
add_definitions(-DHAVE_ATLAS)
add_definitions(-Wno-deprecated-declarations)
include_directories("${LIB_ROOT}/lib/include")
include_directories("${LIB_ROOT}/lib/include/python2.7")
include_directories("${LIB_ROOT}/lib/include/ATLAS")
include_directories("../src")

# lib
file(GLOB center "../src/*.cpp" "../src/common/config/*.cpp" "../src/fluency/*.cc" "../src/PhonesGop/*.cpp")
add_library(compute SHARED ${center})


target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-fstext.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-nnet2.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-nnet3.a)


target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-online2.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-ivector.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-gmm.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-transform.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-feat.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-decoder.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-lat.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-hmm.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-tree.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-util.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-base.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-matrix.a)
target_link_libraries(compute ${LIB_ROOT}/lib/static/kaldi-cudamatrix.a)

target_link_libraries(compute ${LIB_ROOT}/lib/so/libjson.so)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libfst.so.7)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libatlas.so.3)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libf77blas.so.3)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libcblas.so.3)
target_link_libraries(compute ${LIB_ROOT}/lib/so/liblapack_atlas.so.3)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libcublas.so.8.0)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libcudart.so.8.0)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libcurand.so.8.0)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libpython2.7.so)
target_link_libraries(compute ${LIB_ROOT}/lib/so/libboost_python.so)


# compute-fluency-score
#add_executable(compute_test ../src/testbin/compute-test.cpp)
#target_link_libraries(compute_test compute)

#find_package(Threads)
#target_link_libraries(compute_test ${CMAKE_THREAD_LIBS_INIT})
