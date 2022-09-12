add_executable(compressed_vector compressed_vector.cpp)
target_link_libraries(compressed_vector
  xsig
)

add_executable(base64_test base64_test.cpp)
target_link_libraries(base64_test
  xsig
)

add_executable(image_util_tests image_util_tests.cpp)
target_link_libraries(image_util_tests
  opencv_core
  opencv_imgcodecs
  opencv_imgproc
  opencv_highgui
  xsig
)

add_executable(signature_test signature_test.cpp)
target_link_libraries(signature_test
  xsig
  opencv_core
  opencv_imgcodecs
  opencv_imgproc
)

#add_executable(deduper_legacy deduper_legacy.cpp)
#target_link_libraries(deduper_legacy
#  ${OpenCV_LIBS}
#  ${CMAKE_THREAD_LIBS_INIT}
#  xsig
#)

add_executable(testdrive testdrive.cpp)
target_link_libraries(testdrive
  xsig
  opencv_core
  opencv_imgcodecs
  opencv_imgproc
  ${CMAKE_THREAD_LIBS_INIT}
)
if(WIN32)
    target_link_libraries(testdrive shell32 kernel32)
endif()

add_executable(testdrive_sqlite testdrive_sqlite.cpp)
target_link_libraries(testdrive_sqlite
  xsig
  opencv_core
  opencv_imgcodecs
  opencv_imgproc
  ${SQLite3_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)
if(WIN32)
    target_link_libraries(testdrive_sqlite shell32 kernel32)
endif()
