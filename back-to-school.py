import sys
import io
import re

################################################################################

EMPTY  = '.'
FILLED = '#'
MAX_ROUNDS = 100

# MAX_LINE_LEN gives some reasonable upper limit for the line length.
MAX_LINE_LEN = 1024*10

################################################################################

class GameState():
    def __init__(self, firstline):
        self.lines = []
        self.lines.append(firstline)

    def linesFilled(self):
        # How many lines have been filled so far?
        return len(self.lines)

    def fillNextLine(self):
        # "The filling of each square is defined by the square above it and 4
        # squares next to it (2 squares on each sides).
        #
        # Rule #1, the square above is blank: If there are 2 or 3 filled squares
        # in total next to it (taking into account 4 squares, 2 on each sides)
        # it will be filled. If not, it will be left blank.
        #
        # Rule #2, the square above is filled: If there are 2 or 4 squares
        # filled in total next to it (taking into account 4 squares, 2 on each
        # sides) it will be filled. If not, it will be left blank.
        lineAbove = self.lines[-1]
        # Python strings are immutable, therefore using an array when building
        # a new line
        currLineArr = [" "] * len(lineAbove)
        # startIdx is the index of the first potentially filled square.
        # Squares before it will be empty.
        startIdx = getFirstFilledIdx(lineAbove) - 1
        # stopIdx is the index of the last potentially filled square.
        # Squares after it will be empty.
        stopIdx = getLastFilledIdx(lineAbove) + 1

        for i in range(startIdx, stopIdx+1):
            block = getBlock(i, lineAbove)
            filled = block.count(FILLED)
            if lineAbove[i] == " ":
                # Rule #1
                if filled == 2 or filled == 3:
                    currLineArr[i] = FILLED
            else:
                # Rule #2,
                # Note: we count the filled square itself also, therefore
                # checking for 3 or 5, not 2 or 4.
                if filled == 3 or filled == 5:
                    currLineArr[i] = FILLED

        newline = ''.join(currLineArr)
        newline = padTrimLine(newline)
        #print("[+] newline:%s" % newline)
        self.lines.append(newline)

    def printPattern(self):
        # Print the pattern name and return true if the pattern can be
        # recognised based on the lines filled thus far.
        # Otherwise return false.
        lastline = self.lines[-1]
        lastlineStripped = lastline.strip()
        # Loop through all but last added line
        for line in self.lines[:-1]:
            if lastlineStripped == "":
                print("vanishing")
                return True
            if lastline == line:
                print("blinking")
                return True
            if lastlineStripped == line.strip():
                print("gliding")
                return True

        if self.linesFilled() >= MAX_ROUNDS:
            print("other")
            return True

        return False

################################################################################

def getFirstFilledIdx(line):
    # Return the index of the first filled square on the given line
    firstFilledIdx = len(line) - len(line.lstrip())
    return firstFilledIdx

def getLastFilledIdx(line):
    # Return the index of the last filled square on the given line
    lastFilledIdx = len(line.rstrip()) - 1
    return lastFilledIdx

def getBlock(pos, line):
    # Return a block of 5 squares: 2 on each side of line[pos]
    return line[pos-2:pos+1+2]

def padTrimLine(line):
    # Make sure there are at least 3 blanks on both ends of the line.
    # Remove trailing blanks in excess of 3.
    #
    # Why 3?
    # We need exactly 3 leading and trailing blanks so we can safely apply the
    # given rules on the line below.
    # Leading spaces are meaningful when determining if the pattern is
    # gliding or blinking, therefore, leading blanks are kept.
    # Trailing spaces in excess of 3 can be removed since we know they will not
    # produce any new filled squares on the lines below the current line.
    firstFilledIdx = getFirstFilledIdx(line)
    lfill = 3 - firstFilledIdx
    lastFilledIdx = getLastFilledIdx(line)
    rfill = 3 - (len(line) - lastFilledIdx - 1)
    # At least 3 spaces on both ends of the line
    line = " "*lfill + line + " "*rfill
    # Remove trailing spaces if there are more than 3
    if rfill < 0:
        line = line[:rfill]
    return line

################################################################################

def play(infile):
    with io.open(infile) as infile:
        # Read top lines from the infile
        for line in infile:
            # Ignore empty lines
            if bool(line.rstrip() == ''):
                continue
            if len(line) > MAX_LINE_LEN:
                sys.stderr.write(
                    "ERROR: file contains lines longer than %d characters\n" %
                    MAX_LINE_LEN)
                sys.exit(1)
            # Remove newline from the end of the line
            line = line.rstrip('\n')
            # Check that the line contains only expected characters
            if not bool(re.match("^[%s%s]+$" % (EMPTY, FILLED), line )):
                sys.stderr.write("ERROR: unexpected characters: \"%s\"\n"%line)
                sys.exit(1)
            # Replace EMPTY markers from the input line with whitespaces.
            # This is just for convenience.
            line = line.replace(EMPTY, " ")
            line = padTrimLine(line)
            #print("[+] first  :%s" % line)
            game = GameState(line)
            while game.linesFilled() < MAX_ROUNDS:
                game.fillNextLine()
                if game.printPattern():
                    #continue
                    break

################################################################################

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: %s <textfile>" % __file__
        sys.exit(0)

    infile = sys.argv[1]
    play(infile)

################################################################################
