from zipfile import *
import os, sys, md5
"""
gensum.py: used by pybuild.bat, to generate checksums verification programs.
Part of stackless python.
"""

prog = """
import md5
expected = "%s"
fname = r"%s"
print "expected digest", expected
with file(fname, "rb") as f:
    data = f.read()
received = md5.md5(data).hexdigest()
print ("matched" if received == expected else "NOT MATCHED!!")
raw_input("press enter to continue")
"""

def GenerateChecksum(zname):
    if not os.path.exists(zname):
        print "File not existing:", zname
        sys.exit(1)

    with file(zname, "rb") as f:
        data = f.read()

    signame = zname+".md5.py"
    expected = md5.md5(data).hexdigest()
    shortname = os.path.split(zname)[-1]
    with file(signame, "w") as f:
        f.write(prog % (expected, shortname))

if __name__ == "__main__":
    if len(sys.argv) == 2:
        GenerateChecksum(sys.argv[1])
    else:
        print sys.argv[0], "<filename>"
