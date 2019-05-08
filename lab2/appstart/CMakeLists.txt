project(appstart)

add_executable(${PROJECT_NAME} src/main.c src/app_list.c)
target_include_directories(${PROJECT_NAME} PUBLIC inc)

find_package(Curses REQUIRED)
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
