if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
)
  check_include_files(stdint.h stdint)
  if(stdint)
    add_definitions(-DHAVE_STDINT_H)
  endif()

  # Make the tests
  include_directories(../../Algorithms/inc
					  ../../DataHandling/inc
                      ../../TestHelpers/inc
                      ../../Reflectometry/inc)

  # This variable is used within the cxxtest_add_test macro to build these
  # helper classes into the test executable. It will go out of scope at the end
  # of this file so doesn't need un-setting
  set(TESTHELPER_SRCS
      ../../TestHelpers/src/ReflectometryHelper.cpp)
  cxxtest_add_test(ReflectometryTest ${TEST_FILES})
  target_link_libraries(ReflectometryTest
                        PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        Algorithms
                        DataHandling
                        Reflectometry
                        Nexus
                        gmock
)
  add_dependencies(FrameworkTests ReflectometryTest)
  # Test data
  add_dependencies(ReflectometryTest StandardTestData)

  # Add to the 'ReflectometryTest' group in VS
  set_property(TARGET ReflectometryTest PROPERTY FOLDER "UnitTests")
endif()

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} Reflectometry.Python ${TEST_PY_FILES})