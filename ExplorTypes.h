#pragma once
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <functional>
#include <bitset>
#include <set>
#include <numeric>
#include <fstream>
#include <iterator>
#include <string>
#include <array>
#include <variant>
#include <chrono>

enum class WrapMode {
	WRP,
	PLN
};

enum class RenderMode {
	TST,
	RUN
};

enum class NeighbourhoodMode {
	SQR,
	HEX
};

enum class Compares {
	EQ,
	LT,
	GT
};

enum class CHOp {
	SET,
	ADD,
	SUB,
	MPY,
	DIV
};

enum class CHLoc {
	NUMS,
	DIRS,
	CHST,
	XLIT,
	WBTS,
	TPLS
};

struct Parameter {
	std::variant<std::monostate, int, std::string > value;
	bool is_variable = false;
	bool is_valid = false;
	Parameter() :value(std::monostate{}) {};
	Parameter(std::string v) {
		if (v[0] >= 'A' && v[0] <= 'Z') {
			is_variable = true;
			value = v;
		}
		else {
			value = std::stoi(v);
			is_variable = false;
		}
		is_valid = true;
	}

};

struct Probability {
	int n, p;
	bool xn, xp;
	std::mt19937 gen;
	std::uniform_real_distribution<> dis;

	Probability()
		:n(1), p(1), xn(0), xp(0) {
	};

	Probability(bool xn_, int n_, bool xp_, int p_)
		:n(n_), p(p_), xn(xn_), xp(xp_) {

		auto seed = (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count() ^ std::random_device()();
		gen = std::mt19937(seed); //Generate seed 
		dis = std::uniform_real_distribution<double>(0, 1); // Select Distribution
	}

	bool check(int execution_count) {

		//(n,p)
		if (!xn && !xp) { 
			if (execution_count % n == 0) return dis(gen) < 1.0 / p;
		}
		//(X,n,p)
		if (xn && !xp) {
			if (execution_count % n != 0) return dis(gen) < 1.0 / p;
		}
		//(n,X,p)
		if (!xn && xp) {
			if (execution_count % n == 0) return (dis(gen) < 1.0 - (1.0 / p));
		}
		//(X,n,X,p)
		if (xn && xp) {
			if (execution_count % n != 0) return (dis(gen) < 1.0 - (1.0 / p));
		}

		//How can we reformat to remove this?
		return false;
	}
};

std::size_t pxl_to_index(const char c){
	if (c >= 48 && c <= 57) return c - '0';
	if (c >= 65 && c <= 90) return 10 + (c - 'A');

	//Should not occur
	throw std::exception{ "Unknown pixel index has been issued" };
	
	return 0;
}

struct XLIT {
	std::variant<std::string, std::vector<std::string>> replacements;

	XLIT() {};
	XLIT(std::string values, bool full) {
		replacements = values;

		char lastReplacement = values.back();
		//Fill the remaining values with the last known
		if (full)
			for (std::size_t i = 0; i < 36 - values.length(); i++) {
				std::get<0>(replacements).push_back(lastReplacement);
			}
	};
	XLIT(std::vector < std::string > pairs)
		:replacements(pairs) {};

	char transform(char value) const {
		//Transform with string
		if (replacements.index() == 0) {
			if(std::get<0>(replacements).size() <= pxl_to_index(value))
				return value;
			return std::get<0>(replacements).at(pxl_to_index(value));
		}
		//Transform With Vectors
		for (auto s : std::get<1>(replacements)) {
			if (s[0] == value) {
				return s[1];
			}
		}
		//No Transformation Occurs
		return value;
	}
};

struct PXLIT {
	std::vector<std::array<char, 3>> translations;

	char transform(char atDir, char current) const {
		for (auto t : translations) {
			if (t[0] == current && t[1] == atDir) {
				return t[2];
			}
		}
		return current;
	}
};

struct WBT {
	std::string white;
	std::string black;
	std::string twinkle;
};

struct MODE {
	WrapMode wrap;
	RenderMode render;
	NeighbourhoodMode neighbourhood;
};

struct CAM {
	int frames;
};

struct XL {
	int prob{1};
	XLIT translation;
};

struct AXL {
	std::vector<char> numbers;
	std::vector<char> directions;
	std::vector<char> values;
	Parameter prob;
	XLIT translation;
};

struct PXL {
	char dir{'A'};
	int prob{1};
	PXLIT translation;
};

struct BXL {
	Parameter pattern;
	std::vector<Parameter> rectangle;
	XL transform;
};

struct BAXL {
	Parameter pattern;
	std::vector<Parameter> rectangle;

	AXL transform;
};

struct BPXL {
	Parameter pattern;
	std::vector<Parameter> rectangle;

	PXL transform;
};

struct GOTO {
	std::string label;
};

struct IF {
	Parameter lhs;
	Compares cmp;
	Parameter rhs;
};

struct DO {
	std::string label;
};

struct SVP {
	std::string label;
	Parameter x;
	Parameter y;
	Parameter width;
	Parameter height;
};

struct CHP {
	std::string instance; // the label of BXL,BAXL,BPXL 
	std::string newLabel;
};

struct CHV {
	Parameter location;
	CHOp operation;
	Parameter value1;
	Parameter value2;
};

struct XLI {
	std::string label;
	CHLoc location = CHLoc::CHST;
	int prob{1};
	XLIT transform;
};

struct Rectangle {
	std::size_t x;
	std::size_t y;
	std::size_t xM;
	std::size_t yM;
};

struct Rectangles {
	std::size_t x, y;
	std::size_t w, t;
	std::size_t h, v;
	std::size_t c, r;
	Rectangles(std::vector<std::size_t> const& v)
		:x(v[0]), y(v[1]),
		w(v[2]), t(v[3]),
		h(v[4]), v(v[5]),
		c(v[6]), r(v[7]) {
		//Bounds check lel
	}
};


using CommandType = std::variant<WBT, MODE, CAM, XL, AXL, PXL, BXL, BPXL, BAXL, GOTO, IF, DO, SVP, CHV, CHP, XLI>;

struct Command {
	Probability prob;
	CommandType cmd;
	std::string goto_;
};

using Pattern = std::vector<bool>;
using Commands = std::variant<Pattern, Command>;

struct PatternContainer {
	std::vector<Pattern> data;
	std::size_t rows{ 0 };
	std::size_t cols{ 0 };

	PatternContainer() = default;
	PatternContainer(std::size_t r, std::size_t c) :rows(r), cols(c) {
		for (size_t i = 0; i < r; i++)
		{
			Pattern col;
			col.resize(c);
			data.emplace_back(std::move(col));
		}
	}
};


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>; // not needed as of C++20



template<typename T>
constexpr bool in_cmd_group_b = std::is_same_v<T, BXL> || std::is_same_v<T, BAXL> || std::is_same_v<T, BPXL>;

template<typename T>
constexpr bool in_transform_group = std::is_same_v<T, XL> || std::is_same_v<T, AXL> || std::is_same_v<T, PXL>;