import ctypes
import os
import sys

# Path to DLL — adjust if not in current directory
dll_name = "leo.dll"
dll_path = os.path.abspath(dll_name)

if not os.path.exists(dll_path):
        sys.exit(f"Could not find {dll_path}")

# Load the DLL
leo = ctypes.CDLL(dll_path)

# Declare function signature: int leo_sum(int, int)
leo.leo_sum.argtypes = (ctypes.c_int, ctypes.c_int)
leo.leo_sum.restype = ctypes.c_int

# Call the function
a, b = 5, 7
result = leo.leo_sum(a, b)

print(f"leo_sum({a}, {b}) = {result}")
