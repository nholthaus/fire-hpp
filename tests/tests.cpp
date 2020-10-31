
/*
    Copyright Kristjan Kongas 2020

    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <gtest/gtest.h>
#include "fire-hpp/fire.hpp"

#define EXPECT_EXIT_SUCCESS(statement) EXPECT_EXIT(statement, ::testing::ExitedWithCode(0), "")
#define EXPECT_EXIT_FAIL(statement) EXPECT_EXIT(statement, ::testing::ExitedWithCode(_failure_code), "")

using namespace std;
using namespace fire;

void init_args(const vector<string> &args, bool strict, int named_calls = 1000000) {
    const char ** argv = new const char *[args.size()];
    for(size_t i = 0; i < args.size(); ++i)
        argv[i] = args[i].c_str();

    _::logger = _arg_logger();
    _::matcher = _matcher((int) args.size(), argv, named_calls, strict);

    delete [] argv;
}

void init_args(const vector<string> &args) {
    init_args(args, false, 0);
}

void init_args_strict(const vector<string> &args, int named_calls) {
    init_args(args, true, named_calls);
}

// Making this macro a function is unfortunately impossible, because fired_main must preserve it's default arguments
#define CALL_WITH_INTROSPECTION(fired_main, arguments) \
{\
    int argc = arguments.size();\
    vector<const char *> ptrs(arguments.size());\
    for(size_t i = 0; i < arguments.size(); ++i)\
        ptrs[i] = arguments[i].c_str();\
    const char ** argv = ptrs.data();\
    \
    PREPARE_FIRE_(fired_main, argc, argv);\
    fired_main();\
}



TEST(optional, value) {
    fire::optional<int> no_value;
    EXPECT_FALSE((bool) no_value);
    EXPECT_FALSE(no_value.has_value());
    EXPECT_EQ(no_value.value_or(3), 3);
    EXPECT_EXIT_FAIL(no_value.value());

    fire::optional<int> value(1);
    EXPECT_TRUE((bool) value);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value_or(3), 1);
    EXPECT_EQ(value.value(), 1);
}

TEST(optional, assignment) {
    fire::optional<int> opt1, opt2 = 3, opt3(3);
    EXPECT_FALSE(opt1.has_value());
    EXPECT_TRUE(opt2.has_value());
    EXPECT_TRUE(opt3.has_value());
    opt1 = opt3;
    EXPECT_TRUE(opt1.has_value());
}

TEST(optional, no_value) {
    fire::optional<int> opt;
    EXPECT_FALSE((bool) opt);
    EXPECT_FALSE(opt.has_value());
    EXPECT_EQ(opt.value_or(3), 3);
}


TEST(identifier, prepend_hyphens) {
    EXPECT_EQ(identifier::prepend_hyphens(""), "");
    EXPECT_EQ(identifier::prepend_hyphens("a"), "-a");
    EXPECT_EQ(identifier::prepend_hyphens("ab"), "--ab");
    EXPECT_EQ(identifier::prepend_hyphens("abc"), "--abc");
}

TEST(identifier, constructor) {
    fire::optional<int> empty;

    identifier(vector<string>{"-l", "--long"}, empty);
    identifier(vector<string>{"--long", "-l"}, empty);
    identifier(vector<string>{"-l"}, empty);
    identifier(vector<string>{"--long"}, empty);
    identifier(vector<string>{"--long", ""}, empty);
    identifier(vector<string>{"description"}, 0);
    identifier(vector<string>{}, 0);

    EXPECT_EXIT_FAIL(identifier(vector<string>{"-l", "-l"}, empty));
    EXPECT_EXIT_FAIL(identifier(vector<string>{"--long", "--long"}, empty));
    EXPECT_EXIT_FAIL(identifier(vector<string>{"-l"}, 0));
    EXPECT_EXIT_FAIL(identifier(vector<string>{"--long"}, 0));
    EXPECT_EXIT_FAIL(identifier(vector<string>{}, empty));

    EXPECT_EXIT_FAIL(identifier(vector<string>{"-long"}, empty));
    EXPECT_EXIT_FAIL(identifier(vector<string>{"--l"}, empty));
}

TEST(identifier, overlap) {
    fire::optional<int> empty;

    identifier long0(vector<string>{"-l"}, empty);
    identifier long1(vector<string>{"-l", "--long"}, empty);
    identifier long2(vector<string>{"--long"}, empty);
    identifier short0(vector<string>{"-s"}, empty);
    identifier short1(vector<string>{"-s", "--short"}, empty);
    identifier short2(vector<string>{"--short"}, empty);
    identifier pos0(vector<string>{}, 0);
    identifier pos1(vector<string>{}, 1);

    EXPECT_TRUE(long0.overlaps(long1));
    EXPECT_TRUE(long1.overlaps(long2));
    EXPECT_FALSE(long2.overlaps(long0));

    EXPECT_FALSE(long0.overlaps(short0));
    EXPECT_FALSE(long1.overlaps(short1));
    EXPECT_FALSE(long2.overlaps(short2));

    EXPECT_FALSE(long0.overlaps(pos0));
    EXPECT_FALSE(long1.overlaps(pos0));
    EXPECT_FALSE(long2.overlaps(pos0));

    EXPECT_FALSE(pos0.overlaps(long0));
    EXPECT_FALSE(pos0.overlaps(long2));
    EXPECT_TRUE(pos0.overlaps(pos0));
    EXPECT_FALSE(pos0.overlaps(pos1));
}

TEST(identifier, contains) {
    fire::optional<int> empty;

    identifier long0(vector<string>{"-l"}, empty);
    identifier long1(vector<string>{"-l", "--long"}, empty);
    identifier long2(vector<string>{"--long"}, empty);
    identifier pos(vector<string>{}, 0);

    EXPECT_FALSE(long0.contains("--long"));
    EXPECT_TRUE(long1.contains("--long"));
    EXPECT_TRUE(long2.contains("--long"));

    EXPECT_TRUE(long0.contains("-l"));
    EXPECT_TRUE(long1.contains("-l"));
    EXPECT_FALSE(long2.contains("-l"));

    EXPECT_FALSE(long0.contains(0));
    EXPECT_FALSE(long2.contains(0));

    EXPECT_TRUE(pos.contains(0));
    EXPECT_FALSE(pos.contains("--zeroth"));
    EXPECT_FALSE(pos.contains(1));
}

TEST(identifier, help) {
    fire::optional<int> empty;

    EXPECT_EQ(identifier(vector<string>{"-l"}, empty).help(), "-l");
    EXPECT_EQ(identifier(vector<string>{"-l", "--long"}, empty).help(), "-l|--long");
    EXPECT_EQ(identifier(vector<string>{"--long"}, empty).help(), "--long");

    EXPECT_EQ(identifier(vector<string>{"description"}, 0).help(), "<0>");
    EXPECT_EQ(identifier(vector<string>{"<name>", "description"}, 0).help(), "<name>");
    EXPECT_EQ(identifier(vector<string>{"<name>"}, 0).help(), "<name>");
    EXPECT_EQ(identifier().help(), "...");
}

TEST(identifier, longer) {
    fire::optional<int> empty;

    EXPECT_EQ(identifier(vector<string>{"-l"}, empty).longer(), "-l");
    EXPECT_EQ(identifier(vector<string>{"-l", "--long"}, empty).longer(), "--long");
    EXPECT_EQ(identifier(vector<string>{"--long"}, empty).longer(), "--long");

    EXPECT_EQ(identifier(vector<string>{"zeroth"}, 0).longer(), "<0>");
    EXPECT_EQ(identifier(vector<string>{"<name>"}, 0).longer(), "<name>");
    EXPECT_EQ(identifier().longer(), "...");
}

TEST(identifier, less) {
    fire::optional<int> empty;

    EXPECT_TRUE(identifier(vector<string>{"-a"}, empty) < identifier(vector<string>{"-z"}, empty));
    EXPECT_FALSE(identifier(vector<string>{"-z"}, empty) < identifier(vector<string>{"-a"}, empty));

    EXPECT_TRUE(identifier(vector<string>{"--abc"}, empty) < identifier(vector<string>{"--zyx"}, empty));
    EXPECT_FALSE(identifier(vector<string>{"--zyx"}, empty) < identifier(vector<string>{"--abc"}, empty));

    EXPECT_TRUE(identifier(vector<string>{"-z", "--aa"}, empty) < identifier(vector<string>{"-a", "--az"}, empty));
    EXPECT_FALSE(identifier(vector<string>{"-a", "--az"}, empty) < identifier(vector<string>{"-z", "--aa"}, empty));

    EXPECT_TRUE(identifier(vector<string>{}, 0) < identifier(vector<string>{}, 1));
    EXPECT_FALSE(identifier(vector<string>{}, 1) < identifier(vector<string>{}, 0));

    EXPECT_TRUE(identifier(vector<string>{}, 0) < identifier(vector<string>{"-a"}, empty));
    EXPECT_FALSE(identifier(vector<string>{"-a"}, empty) < identifier(vector<string>{}, 0));

    EXPECT_TRUE(identifier(vector<string>{}, 0) < identifier(vector<string>{"first"}, 1) &&
        identifier(vector<string>{"first"}, 1) < identifier(vector<string>{}, 2));
    EXPECT_TRUE(identifier(vector<string>{"-a"}, empty) < identifier(vector<string>{"-B"}, empty) &&
        identifier(vector<string>{"-B"}, empty) < identifier(vector<string>{"-c"}, empty));

    identifier opt_arg(vector<string>{"-a"}, empty);
    opt_arg.set_optional(true);
    identifier req_arg(vector<string>{"-c"}, empty);
    EXPECT_TRUE(req_arg < opt_arg);

    identifier pos_arg(vector<string>{}, 0);
    pos_arg.set_optional(true);
    identifier named_arg(vector<string>{"-a"}, empty);
    EXPECT_TRUE(pos_arg < named_arg);
}


TEST(matcher, invalid_input) {
    EXPECT_EXIT_FAIL(init_args({"./run_tests", "--i"}));
    EXPECT_EXIT_FAIL(init_args({"./run_tests", "-ab=0"}));
    EXPECT_EXIT_FAIL(init_args({"./run_tests", "-aa"}));
    EXPECT_EXIT_FAIL(init_args({"./run_tests", "-x=0", "-x", "0"}));
    EXPECT_EXIT_FAIL(init_args({"./run_tests", "--flag", "--flag"}));
}

TEST(matcher, boolean_flags) {
    init_args({"./run_tests", "-a", "-bcd"});
    EXPECT_TRUE((bool) arg("-a"));
    EXPECT_TRUE((bool) arg("-b"));
    EXPECT_TRUE((bool) arg("-c"));
    EXPECT_TRUE((bool) arg("-d"));
}

TEST(matcher, equations) {
    init_args({"./run_tests", "-a=b", "--abc=xy", "-x=y=z"});
    EXPECT_EQ((string) arg("-a"), "b");
    EXPECT_EQ((string) arg("--abc"), "xy");
    EXPECT_EQ((string) arg("-x"), "y=z"); // quotation marks are omitted from command line

    init_args({"./run_tests", "-a=b", "123"});
}

TEST(matcher, no_space_assignment) {
    init_args({"./run_tests"});
    init_args({"./run_tests", "0"});
    init_args({"./run_tests", "0", "0"});
    init_args({"./run_tests", "-x", "0"}); // there's no equals sign, so "0" is positional
    EXPECT_EXIT_FAIL((void) (int) arg("-x"));
}


TEST(help, help_invocation) {
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "-h"}, 0));
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "--help"}, 0));
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "--help", "1"}, 0));

    init_args_strict({"./run_tests", "-h"}, 1);
    EXPECT_EXIT_SUCCESS((void) (int) arg("-i"));

    init_args_strict({"./run_tests", "-h", "-i", "1"}, 1);
    EXPECT_EXIT_SUCCESS((void) (int) arg("-i"));

    init_args_strict({"./run_tests", "-h"}, 3);
    (void) (int) arg("--i1");
    (void) (int) arg("--i2");
    EXPECT_EXIT_SUCCESS((void) (int) arg("--i3"));

}

TEST(help, no_space_assignment_help_invocation) {
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "-h"}, 0));
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "--help"}, 0));
    EXPECT_EXIT_SUCCESS(init_args_strict({"./run_tests", "--help", "1"}, 0));

    init_args_strict({"./run_tests", "-h"}, 1);
    EXPECT_EXIT_SUCCESS((void) (int) arg(0));

    init_args_strict({"./run_tests", "-h", "1"}, 1);
    EXPECT_EXIT_SUCCESS((void) (int) arg(0));

    init_args_strict({"./run_tests", "-h"}, 3);
    (void) (int) arg(0);
    (void) (int) arg(1);
    EXPECT_EXIT_SUCCESS((void) (int) arg(2));

    init_args_strict({"./run_tests", "-h"}, 1);
    EXPECT_EXIT_SUCCESS(vector<string> v_undef = arg(variadic()));
}


TEST(arg, argument_naming) {
    init_args({"./run_tests"});

    EXPECT_EXIT_FAIL(arg("s"));
    EXPECT_EXIT_FAIL(arg("--s", 0));
    EXPECT_EXIT_FAIL(arg("-long", 0.0));
    EXPECT_EXIT_FAIL(arg("---cmon", "test"));

    EXPECT_EXIT_FAIL(arg("-1", 0));
    (void) arg("--1e3", 0);
    (void) arg("--a3");
}

TEST(arg, defaults) {
    init_args({"./run_tests"});

    EXPECT_EQ((int) arg("-i", 1), 1);
    EXPECT_NEAR((double) arg("-f", 2), 2, 1e-5);
    EXPECT_NEAR((double) arg("-f", 2.0), 2.0, 1e-5);
    EXPECT_EQ((string) arg("-s", "test"), "test");

    EXPECT_EXIT_FAIL((void) (int) arg("-i", 1.0));
    EXPECT_EXIT_FAIL((void) (int) arg("-i", "test"));
    EXPECT_EXIT_FAIL((void) (double) arg("-f", "test"));
    EXPECT_EXIT_FAIL((void) (string) arg("-s", 1));
    EXPECT_EXIT_FAIL((void) (string) arg("-s", 1.0));

    EXPECT_EXIT_FAIL((void) (bool) arg("-b", 1));
}

TEST(arg, correct_parsing) {
    init_args({"./run_tests", "--bool1", "-i=1", "-f=2.0", "-s=test", "1"});

    EXPECT_TRUE((bool) arg("--bool1"));
    EXPECT_FALSE((bool) arg("--bool2"));

    EXPECT_EQ((int) arg("-i", 2), 1);

    EXPECT_EQ((int) arg("-i"), 1);
    EXPECT_NEAR((double) arg("-i"), 1.0, 1e-5);
    EXPECT_NEAR((double) arg("-f"), 2.0, 1e-5);
    EXPECT_EQ((string) arg("-s"), "test");
    EXPECT_EQ((int) arg(0), 1);

    EXPECT_EQ((string) arg({"-s", "--string"}), "test");
    EXPECT_EQ((string) arg({"--string", "-s", "description"}), "test");
}

TEST(arg, incorrect_parsing) {
    init_args({"./run_tests", "-i=1", "-f=2.0", "-s=test"});
    EXPECT_EXIT_FAIL((void) (bool) arg("-i"));

    EXPECT_EXIT_FAIL((void) (int) arg("-f"));
    EXPECT_EXIT_FAIL((void) (int) arg("-s"));
    EXPECT_EXIT_FAIL((void) (double) arg("-s"));

    EXPECT_EXIT_FAIL((void) (int) arg("-x"));

    init_args({"./run_tests"});
    EXPECT_EXIT_FAIL((void) (int) arg(0));

    EXPECT_EXIT_FAIL(init_args({"./run_tests", "---x"}));
}

TEST(arg, positional_parsing) {
    init_args({"./run_tests", "0", "1"});
    EXPECT_EQ((int) arg(0), 0);
    EXPECT_EQ((int) arg(1), 1);
    EXPECT_EQ((int) arg({0, "zeroth"}), 0);
    EXPECT_EQ((int) arg({"first", 1}), 1);
    EXPECT_EXIT_FAIL((void) (int) arg(2));

    init_args({"./run_tests", "0", "-x=3", "1"});
    EXPECT_EQ((int) arg("-x"), 3);
    EXPECT_EXIT_FAIL((void) (int) arg({"-x", 3}));
    EXPECT_EQ((int) arg(0), 0);
    EXPECT_EQ((int) arg(1), 1);
    EXPECT_EQ((int) arg(2, -1), -1);
    fire::optional<int> opt = arg(2);
    EXPECT_FALSE(opt.has_value());

    vector<int> all1 = arg(variadic());
    EXPECT_EQ(all1, vector<int>({0, 1}));
}

TEST(arg, all_positional_parsing) {
    init_args({"./run_tests"});
    vector<int> all0 = arg(variadic());
    EXPECT_EQ(all0, vector<int>({}));

    init_args({"./run_tests", "0", "1"});
    vector<int> all1 = arg(variadic(), "description");
    EXPECT_EQ(all1, vector<int>({0, 1}));

    init_args({"./run_tests", "text"});
    vector<string> all2 = arg(variadic());
    EXPECT_EQ(all2, vector<string>({"text"}));
}

TEST(arg, double_dash_separator) {
    init_args({"./run_tests", "--"});
    vector<string> all0 = arg(variadic());
    EXPECT_EQ(all0, vector<string>({}));

    init_args({"./run_tests", "--flag1", "name0", "--", "name1", "-name2", "--name3", "---name4"});
    vector<string> all1 = arg(variadic());
    EXPECT_EQ(all1, vector<string>({"name0", "name1", "-name2", "--name3", "---name4"}));
}

TEST(arg, precision) {
    init_args({
        "./run_tests",
        "--65535=65535",
        "--65536=65536",
        "--permitted=1000000000000",
        "--overflow=100000000000000000000000000000000000000"
    });

    EXPECT_EXIT_FAIL((void) (unsigned) arg("-a", "-1"));

    (void) (uint16_t) arg("--65535");
    EXPECT_EXIT_FAIL((void) (uint16_t) arg("--65536"));

    (void) (int32_t) arg("-a", (1LL << 31) - 1);
    EXPECT_EXIT_FAIL((void) (int32_t) arg("-a", 1LL << 31));

    (void) (uint32_t) arg("-a", (1LL << 32) - 1);
    EXPECT_EXIT_FAIL((void) (uint32_t) arg("-a", 1LL << 32));

    (void) (int64_t) arg("-a", 1LL << 62);
    (void) (int64_t) arg("--permitted");
    EXPECT_EXIT_FAIL((void) (int64_t) arg("--overflow"));

    (void) (double) arg("-a", 1e100);
    EXPECT_EXIT_FAIL((void) (float) arg("-a", 1e100));
}

bool dashed_values_inside = false;

int dashed_values_main(int x = arg("-x"), int y = arg("-y"), string z = arg("-z"),
        string w = arg("-w"), string q = arg("-q")) {
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, -1);
    EXPECT_EQ(z, "-name");
    EXPECT_EQ(w, "--name");
    EXPECT_EQ(q, "---name");

    dashed_values_inside = true;
    return 0;
}

TEST(arg, dashed_values) {
    vector<string> args = {"./run_tests", "-x", "-1", "-y=-1", "-z=-name", "-w=--name", "-q=---name"};
    CALL_WITH_INTROSPECTION(dashed_values_main, args);
    EXPECT_TRUE(dashed_values_inside);
}

TEST(arg, strict_query) {
    init_args_strict({"./run_tests"}, 0);

    init_args_strict({"./run_tests", "-x=0"}, 1);
    (void) (int) arg("-x");

    EXPECT_EXIT_FAIL(init_args_strict({"./run_tests", "-i=1"}, 0));

    init_args_strict({"./run_tests", "-i=1"}, 2);
    (void) (int) arg("-i");
    EXPECT_EXIT_FAIL((void) (int) arg("-x"));

    init_args_strict({"./run_tests", "-i=1"}, 1);
    EXPECT_EXIT_FAIL((void) (int) arg("-x"));

    init_args_strict({"./run_tests", "-i=1"}, 1);
    EXPECT_EXIT_FAIL((void) (int) arg("-x", 0));
}

TEST(arg, strict_query_positional) {
    init_args_strict({"./run_tests", "0", "1"}, 1);
    EXPECT_EXIT_FAIL((void) (int) arg(0)); // Invalid 2-nd argument

    init_args_strict({"./run_tests", "1"}, 2);
    fire::optional<int> x0 = arg(0), x1 = arg(1);
    EXPECT_EQ(x0.value(), 1);
    EXPECT_FALSE(x1.has_value());

    init_args_strict({"./run_tests", "0", "1"}, 1);
    EXPECT_EXIT_FAIL((void) (int) arg(0));

    init_args_strict({"./run_tests", "0", "1"}, 1);
    vector<int> all0 = arg(variadic());
}

TEST(arg, strict_query_all_positional) {
    init_args_strict({"./run_tests", "0", "1"}, 2);
    vector<int> all1 = arg(variadic());
    EXPECT_EXIT_FAIL((void) (int) arg(0));

    init_args_strict({"./run_tests", "0", "1"}, 2);
    (void) (int) arg(0);
    EXPECT_EXIT_FAIL(vector<int> all2 = arg(variadic()));
}

TEST(arg, optional_arguments) {
    init_args({"./run_tests", "-i=1", "-f=1.0", "-s=test"});

    fire::optional<int> i_undef = arg("--undefined");
    fire::optional<int> i = arg("-i");
    EXPECT_FALSE(i_undef.has_value());
    EXPECT_EQ(i.value(), 1);

    fire::optional<long double> f_undef = arg("--undefined");
    fire::optional<long double> f = arg("-f");
    EXPECT_FALSE(f_undef.has_value());
    EXPECT_NEAR((double) f.value(), 1.0, 1e-5);

    fire::optional<string> s_undef = arg("--undefined");
    fire::optional<string> s = arg("-s");
    EXPECT_FALSE(s_undef.has_value());
    EXPECT_EQ(s.value(), "test");
}

TEST(arg, optional_and_default) {
    init_args({"./run_tests", "-i=0"});

    EXPECT_EXIT_FAIL({ fire::optional<int> x = arg("--undefined", 0); (void) x; });
    EXPECT_EXIT_FAIL({ fire::optional<int> x = arg("-i", 0); (void) x; });
}

TEST(arg, duplicate_parameter) {
    init_args_strict({"./run_tests"}, 2);

    (void) (int) arg("--undefined", 0);
    EXPECT_EXIT_FAIL((void) (int) arg("--undefined", 0));
}

TEST(arg, negative) {
    init_args({"./run_tests", "-a=-1"});
    EXPECT_EQ((int) arg("-a"), -1);

    init_args({"./run_tests", "-1", "-a=-2"});
    EXPECT_EQ((int) arg(0), -1);
    EXPECT_EQ((int) arg("-a"), -2);

    init_args({"./run_tests", "-10", "-a=-20"});
    EXPECT_EQ((int) arg(0), -10);
    EXPECT_EQ((int) arg("-a"), -20);
}

TEST(logger, assignement_arguments) {
    init_args_strict({"./run_tests"}, 100);
    (void) (int) arg({"-i", "--int"});
    (void) (string) arg("-s");
    (void) (float) arg("--float");
    (void) (bool) arg("--bool");

    vector<string> args = _::logger.get_assignment_arguments();
    EXPECT_NE(find(args.begin(), args.end(), "-i"), args.end());
    EXPECT_NE(find(args.begin(), args.end(), "--int"), args.end());
    EXPECT_NE(find(args.begin(), args.end(), "-s"), args.end());
    EXPECT_NE(find(args.begin(), args.end(), "--float"), args.end());
    EXPECT_EQ(find(args.begin(), args.end(), "--bool"), args.end());
}

bool ambiguous_args_inside1 = false;

int ambiguous_args_main1(int x = arg("-x")) {
    EXPECT_EQ(x, 1);

    ambiguous_args_inside1 = true;
    return 0;
}

bool ambiguous_args_inside2 = false;

int ambiguous_args_main2(bool x = arg("-x"), int pos = arg(0)) {
    EXPECT_TRUE(x);
    EXPECT_EQ(pos, 1);

    ambiguous_args_inside2 = true;
    return 0;
}

TEST(introspection, ambiguous_args) {
    vector<string> args = {"./run_tests", "-x", "1"};

    CALL_WITH_INTROSPECTION(ambiguous_args_main1, args);
    EXPECT_TRUE(ambiguous_args_inside1);

    CALL_WITH_INTROSPECTION(ambiguous_args_main2, args);
    EXPECT_TRUE(ambiguous_args_inside2);
}
