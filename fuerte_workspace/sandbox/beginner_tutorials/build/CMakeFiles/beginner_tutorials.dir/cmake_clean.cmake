FILE(REMOVE_RECURSE
  "../src/beginner_tutorials/msg"
  "../src/beginner_tutorials/srv"
  "../msg_gen"
  "../srv_gen"
  "../msg_gen"
  "../srv_gen"
  "CMakeFiles/beginner_tutorials.dir/src/tester.o"
  "../bin/beginner_tutorials.pdb"
  "../bin/beginner_tutorials"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang CXX)
  INCLUDE(CMakeFiles/beginner_tutorials.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
