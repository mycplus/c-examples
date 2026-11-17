# Builds one target that must not compile, and checks two things: the build
# fails, and the compiler's message mentions the expected name. A build that
# fails for some unrelated reason does not count as a pass.
execute_process(COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --target ${TARGET} --config ${CONFIG}
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
if(rc EQUAL 0)
  message(FATAL_ERROR "${TARGET} compiled, but it must not")
endif()
if(NOT "${out}${err}" MATCHES "${EXPECT}")
  message(FATAL_ERROR "${TARGET} failed without mentioning '${EXPECT}':\n${out}${err}")
endif()
