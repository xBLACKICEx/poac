#include <boost/ut.hpp>

#include <poac/core/builder/ninja/syntax.hpp>

#include <string>
#include <sstream>
#include <vector>

static const std::string LONGWORD = std::string(10, 'a');
static const std::string LONGWORDWITHSPACES = std::string(5, 'a') + "$ " + std::string(5, 'a');
static const std::string INDENT = "    ";

int main() {
    using namespace std::literals::string_literals;
    using namespace boost::ut;
    using namespace boost::ut::spec;

    namespace ninja = poac::core::builder::ninja;
    using boost::algorithm::join;
    using vec = std::vector<std::string>;

    describe("test line word wrap") = [] {
        it("test single long word") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer._line(LONGWORD);
            expect(eq(LONGWORD + '\n',writer.get_value()));
        };

        it("test few long words") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer._line(join(vec{"x"s, LONGWORD, "y"s}, " "));
            expect(eq(
                join(
                    vec{
                        "x"s,
                        INDENT + LONGWORD,
                        INDENT + "y"
                    },
                    " $\n"
                ) + '\n',
                writer.get_value()
            ));
        };

        it("test comment wrap") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer.comment("Hello /usr/local/build-tools/bin");
            expect(eq(
                "# Hello\n# /usr/local/build-tools/bin\n"s,
                writer.get_value()
            )) << "Filenames should not be wrapped";
        };

        it("test short words indented") = [] {
            // Test that indent is taking into account when breaking subsequent lines.
            // The second line should not be '    to tree', as that's longer than the
            // test layout width of 8.
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer._line("line_one to tree");
            expect(eq(
                "line_one $\n"
                "    to $\n"
                "    tree\n"s,
                writer.get_value()
            ));
        };

        it("test few long words indented") = [] {
            // Check wrapping in the presence of indenting.
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer._line(join(vec{"x"s, LONGWORD, "y"s}, " "), 1);
            expect(eq(
                join(
                    vec{
                        "  "s + "x",
                        "  " + INDENT + LONGWORD,
                        "  " + INDENT + "y"
                    },
                    " $\n"
                ) + '\n',
                writer.get_value()
            ));
        };

        it("test escaped spaces") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 8 };
            writer._line(join(vec{"x"s, LONGWORDWITHSPACES, "y"s}, " "));
            expect(eq(
                join(
                    vec{
                        "x"s,
                        INDENT + LONGWORDWITHSPACES,
                        INDENT + "y"
                    },
                    " $\n"
                    ) + '\n',
                writer.get_value()
            ));
        };

        it("test fit many words") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 78 };
            writer._line("command = cd ../../chrome; python ../tools/grit/grit/format/repack.py ../out/Debug/obj/chrome/chrome_dll.gen/repack/theme_resources_large.pak ../out/Debug/gen/chrome/theme_resources_large.pak", 1);
            expect(eq(
                "  command = cd ../../chrome; python ../tools/grit/grit/format/repack.py $\n"
                "      ../out/Debug/obj/chrome/chrome_dll.gen/repack/theme_resources_large.pak $\n"
                "      ../out/Debug/gen/chrome/theme_resources_large.pak\n"s,
                writer.get_value()
            ));
        };

        it("test leading space") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 14 };
            writer.variable("foo", vec{"", "-bar", "-somethinglong"}, 0);
            expect(eq(
                "foo = -bar $\n"
                "    -somethinglong\n"s,
                writer.get_value()
            ));
        };

        it("test embedded dollar dollar") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 15 };
            writer.variable("foo", vec{"a$$b", "-somethinglong"}, 0);
            expect(eq(
                "foo = a$$b $\n"
                "    -somethinglong\n"s,
                writer.get_value()
            ));
        };

        it("test two embedded dollar dollars") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 17 };
            writer.variable("foo", vec{"a$$b", "-somethinglong"}, 0);
            expect(eq(
                "foo = a$$b $\n"
                "    -somethinglong\n"s,
                writer.get_value()
            ));
        };

        it("test leading dollar dollar") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 14 };
            writer.variable("foo", vec{"$$b", "-somethinglong"}, 0);
            expect(eq(
                "foo = $$b $\n"
                "    -somethinglong\n"s,
                writer.get_value()
            ));
        };

        it("test trailing dollar dollar") = [] {
            ninja::syntax::writer writer{ std::ostringstream(), 14 };
            writer.variable("foo", vec{"a$$", "-somethinglong"}, 0);
            expect(eq(
                "foo = a$$ $\n"
                "    -somethinglong\n"s,
                writer.get_value()
            ));
        };
    };

    describe("test build") = [] {
        it("test variables dict") = [] {
            ninja::syntax::writer writer{ std::ostringstream() };
            writer.build(
                {"out"},
                "cc",
                ninja::syntax::build_set_t{
                    .inputs = std::vector{"in"s},
                    .variables = std::unordered_map<std::string, std::string>{
                        {"name", "value"}
                    }
                }
            );

            expect(eq(
                "build out: cc in\n"
                "  name = value\n"s,
                writer.get_value()
            ));
        };

        it("test implicit outputs") = [] {
            ninja::syntax::writer writer{ std::ostringstream() };
            writer.build(
                {"o"},
                "cc",
                ninja::syntax::build_set_t{
                    .inputs = std::vector{"i"s},
                    .implicit_outputs = "io",
                }
            );

            expect(eq(
                "build o | io: cc i\n"s,
                writer.get_value()
            ));
        };
    };

    describe("test expand") = [] {
        it("test basic") = [] {
            const ninja::syntax::variables_t vars{{"x", "X"}};
            expect(eq("foo"s, ninja::syntax::expand("foo", vars)));
        };

        it("test var") = [] {
            const ninja::syntax::variables_t vars{{"xyz", "XYZ"}};
            expect(eq("fooXYZ"s, ninja::syntax::expand("foo$xyz", vars)));
        };

        it("test vars") = [] {
            const ninja::syntax::variables_t vars{{"x", "X"}, {"y", "YYY"}};
            expect(eq("XYYY"s, ninja::syntax::expand("$x$y", vars)));
        };

        it("test space") = [] {
            const ninja::syntax::variables_t vars{};
            expect(eq("x y z"s, ninja::syntax::expand("x$ y$ z", vars)));
        };

        it("test locals") = [] {
            const ninja::syntax::variables_t vars{{"x", "a"}};
            const ninja::syntax::variables_t local_vars{{"x", "b"}};
            expect(eq("a"s, ninja::syntax::expand("$x", vars)));
            expect(eq("b"s, ninja::syntax::expand("$x", vars, local_vars)));
        };

        it("test double") = [] {
            expect(eq("a b$c"s, ninja::syntax::expand("a$ b$$c", {})));
        };
    };
}