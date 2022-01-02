bool passed_all = true;

#define IS_TRUE(x) {if (!(x)) {std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; passed_all = false;}}
#define IS_FALSE(x) {IS_TRUE(!x);}
#define IS_EQUAL(expected, received) {if (expected != received) {std::cout << __FUNCTION__ << " failed on line " << __LINE__ << ". Expected " << expected << ", received " << received << std::endl; passed_all = false;}}

// test if two doubles are approximately equal
bool approx(double a, double b) {return a - 1e-9 < b && a + 1e-9 > b;}