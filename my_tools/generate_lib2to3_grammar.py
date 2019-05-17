
import sys
import os
sys.path.insert(0, os.path.normpath(os.path.join(os.path.abspath(__file__), "..", "..", "Lib")))

import lib2to3
import lib2to3.pygram as pygram

# load_grammar in driver.py generates pickle files.
