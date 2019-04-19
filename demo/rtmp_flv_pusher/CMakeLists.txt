CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(rtmp_flv_pusher_demo)

AUX_SOURCE_DIRECTORY(../../src/pusher pusher_source)
ADD_EXECUTABLE(rtmp_flv_pusher_demo rtmp_flv_pusher_demo.cpp ${pusher_source})
TARGET_LINK_LIBRARIES(rtmp_flv_pusher_demo avformat avcodec avutil avfilter avdevice swresample swscale pthread)
MESSAGE(status ${PROJECT_DIR})
INSTALL(TARGETS rtmp_flv_pusher_demo RUNTIME DESTINATION ${PROJECT_DIR}/bin)
