add_executable(compressed_vector compressed_vector.cpp)
target_link_libraries(compressed_vector
  ${OpenCV_LIBS}
  xsig
)

add_executable(image_util_tests image_util_tests.cpp)
target_link_libraries(image_util_tests
  ${OpenCV_LIBS}
  xsig
)

add_executable(signature_test signature_test.cpp)
target_link_libraries(signature_test
  ${OpenCV_LIBS}
  xsig
)

#add_executable(deduper_legacy deduper_legacy.cpp)
#target_link_libraries(deduper_legacy
#  ${OpenCV_LIBS}
#  ${CMAKE_THREAD_LIBS_INIT}
#  xsig
#)

add_executable(testdrive testdrive.cpp)
target_link_libraries(testdrive
  ${OpenCV_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  xsig
)
if(WIN32)
    target_link_libraries(testdrive shell32 kernel32)
endif()
