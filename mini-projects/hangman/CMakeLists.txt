add_executable(hangman hangman.c)
mini_project_session(hangman win  ARGS "--word PROGRAMMER")
mini_project_session(hangman lose ARGS "--word syntax")
mini_project_session(hangman eof  ARGS "--word POINTER")
