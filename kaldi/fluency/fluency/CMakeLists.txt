cmake_minimum_required (VERSION 2.6)
set(CMAKE_BUILD_TYPE "Debug")  
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

if(CONAN_LIBCXX STREQUAL "libstdc++11")
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
elseif(CONAN_LIBCXX STREQUAL "libstdc++")
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

project (kaldi-gop)

# Kaldi's path
set(KALDI_ROOT /home/zhangnian/kaldi)  # please edit this line

find_package(PythonLibs)
include_directories (${PYTHON_INCLUDE_DIRS})

find_package(Boost 1.66.0 COMPONENTS python)
include_directories (${Boost_INCLUDE_DIRS}) 

# Settings
add_compile_options(-std=c++11)
add_definitions(-DHAVE_ATLAS)
add_definitions(-Wno-deprecated-declarations)
include_directories("${KALDI_ROOT}/src")
include_directories("${KALDI_ROOT}/tools/openfst/include")
include_directories("${KALDI_ROOT}/tools/ATLAS/include")
include_directories("/usr/local/cuda/include")
include_directories("/home/zhangnian/jsoncpp/include")
#include_directories("/home/zhangnian/boost_1_66/local/include")
#link_directories("/home/zhangnian/boost_1_66/local/lib")


# fluency
include_directories(".")
file(GLOB FLUENCY fluency/*.cc)
#add_library(fluency SHARED ${FLUENCY})
add_library(fluency ${FLUENCY})

set_target_properties(fluency PROPERTIES PREFIX "")

target_link_libraries(fluency ${KALDI_ROOT}/src/gmm/kaldi-gmm.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/transform/kaldi-transform.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/feat/kaldi-feat.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/fstext/kaldi-fstext.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/nnet2/kaldi-nnet2.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/decoder/kaldi-decoder.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/lat/kaldi-lat.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/hmm/kaldi-hmm.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/tree/kaldi-tree.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/util/kaldi-util.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/base/kaldi-base.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/matrix/kaldi-matrix.a)
target_link_libraries(fluency ${KALDI_ROOT}/src/cudamatrix/kaldi-cudamatrix.a)

target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfst.so.7.0.0)
target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfstfar.so.7.0.0)
target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfstfarscript.so.7.0.0)
target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfstngram.so.7.0.0)
target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfstscript.so.7.0.0)

target_link_libraries(fluency /home/zhangnian/jsoncpp/libs/libjson.so)
target_link_libraries(fluency dl.so)
target_link_libraries(fluency /usr/lib/libatlas.so.3)
target_link_libraries(fluency /usr/lib/libf77blas.so.3)
target_link_libraries(fluency /usr/lib/libcblas.so.3)
target_link_libraries(fluency /usr/lib/liblapack_atlas.so.3)
target_link_libraries(fluency /usr/local/cuda/lib64/libcublas.so)
target_link_libraries(fluency /usr/local/cuda/lib64/libcublas.so.8.0)
target_link_libraries(fluency /usr/local/cuda/lib64/libcublas.so.8.0.61)
target_link_libraries(fluency /usr/local/cuda/lib64/libcudart.so)
target_link_libraries(fluency /usr/local/cuda/lib64/libcudart.so.8.0)
target_link_libraries(fluency /usr/local/cuda/lib64/libcudart.so.8.0.61)
target_link_libraries(fluency /usr/local/cuda/lib64/libcurand.so)
target_link_libraries(fluency /usr/local/cuda/lib64/libcurand.so.8.0)
target_link_libraries(fluency /usr/local/cuda/lib64/libcurand.so.8.0.61)
target_link_libraries(fluency ${KALDI_ROOT}/tools/openfst/lib/libfst.so)


target_link_libraries (fluency
    boost_python
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
)

# compute-fluency-score
add_executable(compute-fluency-score fluencybin/compute-fluency-score.cc)
#add_library(fluency-score SHARED mfccbin/compute-fluency-score.cc)
target_link_libraries(compute-fluency-score fluency)
find_package(Threads)
target_link_libraries(compute-fluency-score ${CMAKE_THREAD_LIBS_INIT})

#find_package(Threads)
#target_link_libraries(fluency ${CMAKE_THREAD_LIBS_INIT})