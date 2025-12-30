# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-src")
  file(MAKE_DIRECTORY "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-build"
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix"
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/tmp"
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/src/opus_cmake-populate-stamp"
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/src"
  "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/src/opus_cmake-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/src/opus_cmake-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Mitchell Aikens/source/repos/ResidualAudioEngine/build/_deps/opus_cmake-subbuild/opus_cmake-populate-prefix/src/opus_cmake-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
