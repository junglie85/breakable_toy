add_executable(WelcomeTest welcome.test.cpp)
target_link_libraries(WelcomeTest PRIVATE Catch2::Catch2WithMain bt)
add_test(NAME WelcomeTest COMMAND WelcomeTest)
