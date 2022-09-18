set(xsig_SOURCES
    src/base64.cpp
    src/imageutil.cpp
    src/signature.cpp
    src/subslice_signature.cpp
    src/signature_db.cpp
)

add_library(xsig ${xsig_SOURCES})

target_include_directories(xsig PRIVATE ./src)
target_include_directories(xsig PUBLIC ./include)

target_link_libraries(xsig PRIVATE
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
    ${SQLite3_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_options(xsig PRIVATE -Werror=return-type)
