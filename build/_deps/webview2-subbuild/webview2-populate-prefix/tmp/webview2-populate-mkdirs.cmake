# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/Drmd/LooperVstproject/build/webview2/Microsoft.Web.WebView2.1.0.2903.40")
  file(MAKE_DIRECTORY "C:/Users/Drmd/LooperVstproject/build/webview2/Microsoft.Web.WebView2.1.0.2903.40")
endif()
file(MAKE_DIRECTORY
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-build"
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix"
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/tmp"
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/src/webview2-populate-stamp"
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/src"
  "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/src/webview2-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/src/webview2-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Drmd/LooperVstproject/build/_deps/webview2-subbuild/webview2-populate-prefix/src/webview2-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
