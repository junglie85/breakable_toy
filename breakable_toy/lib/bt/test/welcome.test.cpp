#include <catch2/catch_test_macros.hpp>

#include <bt/welcome.hpp>

TEST_CASE("1 == 1") {
    REQUIRE("Welcome, Breakable Toy!" == bt::greet());
}
