#include <gtest/gtest.h>
#include <stdlib.h>
#include <fcntl.h>

using namespace std;

// shell to run tests on
//#define SHELL "../build/shell -t"
#define SHELL "/bin/sh"

// declarations of methods you want to test (should match exactly)
std::vector<std::vector<std::string>> parseArguments(const std::vector<std::string>&);

namespace {

void Execute(std::string command, std::string expectedOutput);
void Execute(std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent);

TEST(Shell, ParseArguments) {
    std::vector<std::string> expected;

    expected = {};
    EXPECT_EQ(expected, parseArguments(""));
    EXPECT_EQ(expected, parseArguments(" "));
    EXPECT_EQ(expected, parseArguments("  "));

    expected = {"foo"};
    EXPECT_EQ(expected, parseArguments("foo"));
    EXPECT_EQ(expected, parseArguments(" foo"));
    EXPECT_EQ(expected, parseArguments("foo "));
    EXPECT_EQ(expected, parseArguments(" foo "));
    EXPECT_EQ(expected, parseArguments("  foo  "));

    expected = {"foo", "bar"};
    EXPECT_EQ(expected, parseArguments("foo bar"));
    EXPECT_EQ(expected, parseArguments(" foo  bar"));
    EXPECT_EQ(expected, parseArguments("  foo   bar  "));

    expected = {"cmd1", "arg1", "<", "inputfile", "|", "cmd2", "arg2", ">", "outputfile"};
    EXPECT_EQ(expected, parseArguments("cmd1 arg1 < inputfile | cmd2 arg2 > outputfile"));
}

TEST(Shell, ReadFromFile) {
    Execute("cat < 1", "line 1\nline 2\nline 3\nline 4");
}

TEST(Shell, ReadFromAndWriteToFile) {
    Execute("cat < 1 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\nline 4");
}

TEST(Shell, ReadFromAndWriteToFileChained) {
    Execute("cat < 1 | head -n 3 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\n");
    Execute("cat < 1 | head -n 3 | tail -n 1 > ../foobar", "", "../foobar", "line 3\n");
}

TEST(Shell, WriteToFile) {
    Execute("ls -1 > ../foobar", "", "../foobar", "1\n2\n3\n4\n");
}

TEST(Shell, Execute) {
    Execute("uname", "Linux\n");
    Execute("ls", "1\n2\n3\n4\n");
    Execute("ls -1", "1\n2\n3\n4\n");
}

TEST(Shell, ExecuteChained) {
    Execute("ls -1 | head -n 2", "1\n2\n");
    Execute("ls -1 | head -n 2 | tail -n 1", "2\n");
}


//////////////// HELPERS

std::string filecontents(const std::string& str) {
  std::string retval;
  int fd = open(str.c_str(), O_RDONLY);
  struct stat st;
  if (fd >= 0 && fstat(fd, &st) == 0) {
    long long size = st.st_size;
    retval.resize(size);
    char *current = (char*)retval.c_str();
    ssize_t left = size;
    while (left > 0) {
      ssize_t bytes = read(fd, current, left);
      if (bytes == 0 || (bytes < 0 && errno != EINTR))
        break;
      if (bytes > 0) {
        current += bytes;
        left -= bytes;
      }
    }
  }
  if (fd >= 0)
    close(fd);
  return retval;
}

void filewrite(const std::string& str, std::string content) {
  int fd = open(str.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd < 0)
    return;
  while (content.size() > 0) {
    int written = write(fd, content.c_str(), content.size());
    if (written == -1 && errno != EINTR) {
      std::cout << "error writing file '" << str << "': error " << errno << std::endl;
      break;
    }
    content = content.substr(written);
  }
  close(fd);
}

void Execute(std::string command, std::string expectedOutput) {
    filewrite("input", command);
    system("cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> /dev/null");
    std::string got = filecontents("output");
    EXPECT_EQ(expectedOutput, got);
}

void Execute(std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent) {
    std::string expectedOutputLocation = "../test-dir/" + expectedOutputFile;
    unlink(expectedOutputLocation.c_str());
    filewrite("input", command);
    int rc = system("cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> /dev/null");
    EXPECT_EQ(0, rc);
    std::string got = filecontents("output");
    EXPECT_EQ(expectedOutput, got) << command;
    std::string gotOutputFileContents = filecontents(expectedOutputLocation);
    EXPECT_EQ(expectedOutputFileContent, gotOutputFileContents) << command;
    unlink(expectedOutputLocation.c_str());
}

}
