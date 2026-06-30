add_test( ManifoldTest.STLTest /workspace/build/test/test_manifoldplus [==[--gtest_filter=ManifoldTest.STLTest]==] --gtest_also_run_disabled_tests)
set_tests_properties( ManifoldTest.STLTest PROPERTIES WORKING_DIRECTORY /workspace/build/test SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set( test_manifoldplus_TESTS ManifoldTest.STLTest)
