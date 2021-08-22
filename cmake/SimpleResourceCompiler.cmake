function(SRC_COMPILE_RESOURCES target directory name)
    message("src: Target: ${target}")
    message("src: ResourceDir: ${directory}")
    message("src: Output: ${name}")
    message("src: Generating ${name}.h")

    set(SRC_GENERATED_HEADER "${name}.h")

    # CMake will have to be rerun whenever files are added or removed
    file(GLOB_RECURSE SRC_FILE_RESOURCES 
        "${directory}/*")
    message("${SRC_FILE_RESOURCES}")

    add_custom_command(
                    OUTPUT "${CMAKE_BINARY_DIR}/${SRC_GENERATED_HEADER}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}"
                    COMMAND $<TARGET_FILE:src> ARGS "-v" "-t" ${directory} "-o" ${name} "-s" "${CMAKE_BINARY_DIR}/"
                    WORKING_DIRECTORY $<TARGET_FILE_DIR:src>
                    DEPENDS ${SRC_FILE_RESOURCES}
                    #BYPRODUCTS ${CMAKE_BINARY_DIR}/${SRC_GENERATED_HEADER}
                    COMMENT "Run SimpleResourceCompiler"
                    VERBATIM
    )
                    
    target_sources(${target}
        PUBLIC 
            "${CMAKE_BINARY_DIR}/${SRC_GENERATED_HEADER}"
    )  

    #set_property(TARGET ${target} APPEND PROPERTY OBJECT_DEPENDS ${SRC_GENERATED_HEADER})
    #set_property(SOURCE ${SRC_GENERATED_HEADER} APPEND PROPERTY OBJECT_DEPENDS target)
    message("================================")
endfunction(SRC_COMPILE_RESOURCES)