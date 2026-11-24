add_executable(mini_piano mini_piano.c)
if(NOT MSVC)
  target_link_libraries(mini_piano PRIVATE m)   # sin() and pow() live in libm
endif()
mini_project_session(mini_piano session)
mini_project_session(mini_piano ode)
mini_project_session(mini_piano eof)
