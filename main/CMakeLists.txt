idf_component_register(SRCS "hackingtool.c"
                    REQUIRES htool_web htool_api htool_wifi
                    )
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)



