add_library (tap
    tap/tap.c
    tap/tap.h)
include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/tap/)

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
endif ()

# Test cases

add_executable (test_http_server
    test_http_server.c)
target_link_libraries (test_http_server
    http_server
    tap)
add_test (http_server
    test_http_server)

add_executable (test_errors
    test_errors.c)
target_link_libraries (test_errors
    http_server
    tap)
add_test (errors
    test_errors)

add_executable (test_response
    test_response.c)
target_link_libraries (test_response
    http_server
    tap)
add_test (response
    test_response)
