include (CodeCoverage)
set (CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
set (CMAKE_C_FLAGS "-fprofile-arcs -ftest-coverage")

# Test server for blackbox testing
find_package (PythonInterp)

if (PYTHONINTERP_FOUND)
    add_executable (test_app
        test_app.c)
    target_link_libraries (test_app
        http_server)
    get_property (test_app_exe TARGET test_app PROPERTY LOCATION)

    if (HTTP_SERVER_HAVE_SELECT)
        # Test select(2) event loop
        add_test (blackbox_select
            ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_blackbox.py)
        set_tests_properties (blackbox_select
            PROPERTIES ENVIRONMENT "EXECUTABLE=${test_app_exe};HTTP_SERVER_EVENT_LOOP=select")
    endif ()

    if (HTTP_SERVER_HAVE_KQUEUE)
        # Test kqueue(2) event loop
        add_test (blackbox_kqueue
            ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_blackbox.py)
        set_tests_properties (blackbox_kqueue
            PROPERTIES ENVIRONMENT "EXECUTABLE=${test_app_exe};HTTP_SERVER_EVENT_LOOP=kqueue")
    endif ()

    # Regenerate clar test suite
    add_custom_target (generate_clar
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate.py ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

# Test suite

add_executable (test_suite
    test_response.c
    test_errors.c
    test_http_server.c
    clar.c
    clar.h
    main.c)
target_link_libraries (test_suite
    http_server)

# Coverage stuff
get_property (test_suite_exe TARGET test_suite PROPERTY LOCATION)
SETUP_TARGET_FOR_COVERAGE (coverage
    ${test_suite_exe}
    coverage)
