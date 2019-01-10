FILE(REMOVE_RECURSE
  "../src/beginner_tutorials/msg"
  "../src/beginner_tutorials/srv"
  "../msg_gen"
  "../srv_gen"
  "../msg_gen"
  "../srv_gen"
  "CMakeFiles/tester.dir/src/tester.o"
  "../bin/tester.pdb"
  "../bin/tester"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang CXX)
  INCLUDE(CMakeFiles/tester.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
