# Compiles SOURCE with the project's compiler and succeeds only if the
# compilation fails with a message that matches EXPECT.
if(MSVC)
  set(cmd ${CC} /nologo /c /std:c${STD} /W4 /WX /Fo${OBJ} ${SOURCE})
else()
  set(cmd ${CC} -std=c${STD} -Wall -Wextra -Werror -c ${SOURCE} -o ${OBJ})
endif()
execute_process(COMMAND ${cmd} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0)
  message(FATAL_ERROR "${SOURCE} compiled, but it must not")
endif()
if(NOT "${out}${err}" MATCHES "${EXPECT}")
  message(FATAL_ERROR "${SOURCE} failed for another reason:\n${out}${err}")
endif()
