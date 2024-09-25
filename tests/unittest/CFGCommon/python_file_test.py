def print_something(msg) :
  print(msg)

def func1(arg0, arg1, arg2, arg3, arg4, arg5) :
  print_something("Start of func1")
  assert isinstance(arg0, bool)
  assert arg0 == True
  assert isinstance(arg1, int)
  assert arg1 == -1
  assert isinstance(arg2, str)
  assert arg2 == "abc"
  assert isinstance(arg3, bytearray)
  assert len(arg3) == 3
  assert arg3[0] == 0
  assert arg3[1] == 1
  assert arg3[2] == 2
  assert isinstance(arg4, list)
  assert len(arg4) == 2
  assert arg4[0] == 10
  assert arg4[1] == -10
  assert isinstance(arg5, list)
  assert len(arg5) == 3
  assert arg5[0] == "X"
  assert arg5[1] == "y"
  assert arg5[2] == "Z"
  print_something("End of func1")
  return [not arg0, arg1 - 10, arg2.upper(), bytearray([a + 10 for a in arg3]), arg4 + [-5, 5], [a.lower() for a in arg5], None]
  
def func2() :
  print_something("Start of func2")

abc = 101