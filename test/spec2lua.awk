#!/usr/bin/awk -f

# Extract the test cases from CommonMark's spec.txt
# into a Lua file for unit testing the Markdown parser
#
# Simple state engine:
# state 0: ordinary text lines (initial state)
# state 1: Markdown lines, after /`{32} example/ in state 0
# state 2: HTML lines, after /^\./ in state 1
# state 0: back to text, after /`{32}/ in state 2
#
# Output generated:
# Test{
#   number = 123,
#   section = "Foo",
#   input = [==[ ... ]==],
#   result = [==[ ... ]==]
# }

BEGIN {
  state = 0
  number = 0
  heading = ""
}

/^#+ / && state == 0 {
  gsub(/#+ /, "")
  heading = $0
  print "-- " $0
  next
}

/^`{32} example/ && state == 0 {
  state = 1
  number += 1
  printf "Test{\n  number = %d,\n", number
  printf "  section = \"%s\",\n", heading
  printf "  input = [==["
  next
}

/^\.$/ && state == 1 {
  state = 2
  printf "]==],\n  result = [==["
  next
}

/^`{32}/ && state == 2 {
  state = 0
  printf "]==]\n}\n\n"
  next
}

state == 1 || state == 2 {
  gsub("→", "\t")
  print $0
}
