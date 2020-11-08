import sys

lineno = 0

sys.stdout.write(str(lineno))

with open("./mgba.log", "r") as mGBA:
    with open("./DSHBA.log", "r") as DSHBA:

        while True:
            lineno += 1
            sys.stdout.write(f"\r{lineno}")

            mGBA_line  = mGBA.readline().strip().lower()
            DSHBA_line = DSHBA.readline().strip().lower()

            if not DSHBA_line:
                print("\rNo difference found, DSHBA log ran out")
                quit(0)

            if not mGBA_line:
                print("\rNo difference found, mGBA log ran out")
                quit(0)

            if not mGBA_line.startswith(DSHBA_line):
                print("Difference in line", lineno)
                print("   mGBA: ", mGBA_line)
                print("   DSHBA:", DSHBA_line)
                print("press any key to continue...")
                sys.stdin.read(1)
                print("ok")