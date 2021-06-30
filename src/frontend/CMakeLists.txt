add_executable(main main.cpp)
target_link_libraries(main smolboy fmt::fmt SDL2-static SDL2main)

# Gets rid of an annoying warning
if(WIN32)
    target_link_options(main PRIVATE -Xlinker /subsystem:console)
endif()