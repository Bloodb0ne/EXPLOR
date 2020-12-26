#pragma once

#include "ConstFuse.h"
#include "StringParsers.h"

using namespace constfuse;
using namespace constfuse::string;


using ctxIter = ContextAwareIterator<std::string::iterator>;


std::array<std::string, 17> fnames = { "AXL","BAXL","BPXL","BXL","CAMERA","CHP","CHV","DO","GOTO","IF","MODE","PAT","PXL","SVP","WBT","XL","XLI" };

constexpr auto notopcode = [](bool& res, std::string& name)->std::string {
	bool hasFound = std::find(fnames.begin(), fnames.end(), name) != fnames.end();
	if (hasFound) {
		name.clear(); res = false;
	}
	else {
		res = true;
	};

	return name;
};

constexpr auto success_to_bool = [](bool& r, auto)->bool {
	return r;
};
constexpr auto success_to_false = [](bool& r, auto)->bool {
	return !r;
};

constexpr auto to_triplet = [](std::tuple<char, char, char> t) {
	return std::array<char, 3>{std::get<0>(t), std::get<1>(t), std::get<2>(t) };
};

constexpr auto to_bin_octal = [](std::string n) {
	Pattern res;
	std::for_each(n.begin(), n.end(), [&](char item) {
		uint8_t num = item - '0';
		res.push_back((num / 4) % 2);
		res.push_back((num / 2) % 2);
		res.push_back(num % 2);
		});
	return res;
};

constexpr auto mode1 = (ParseLit("WRP") | to_value(WrapMode::WRP)) || (ParseLit("PLN") | to_value(WrapMode::PLN));
constexpr auto mode2 = ParseLit("TST") | to_value(RenderMode::TST) || ParseLit("RUN") | to_value(RenderMode::RUN);
constexpr auto mode3 = ParseLit("SQR") | to_value(NeighbourhoodMode::SQR) || ParseLit("HEX") | to_value(NeighbourhoodMode::HEX);

constexpr auto compare_funcs = ParseLit("EQ") | to_value(Compares::EQ) ||
ParseLit("LT") | to_value(Compares::LT) ||
ParseLit("GT") | to_value(Compares::GT);

constexpr auto change_funcs = ParseLit("SET") | to_value(CHOp::SET) ||
ParseLit("SUB") | to_value(CHOp::SUB) ||
ParseLit("ADD") | to_value(CHOp::ADD) ||
ParseLit("MPY") | to_value(CHOp::MPY) ||
ParseLit("DIV") | to_value(CHOp::DIV);

constexpr auto tlit_options = ParseLit("NUMS") | to_value(CHLoc::NUMS) ||
ParseLit("DIRS") | to_value(CHLoc::DIRS) ||
ParseLit("CHST") | to_value(CHLoc::CHST) ||
ParseLit("XLIT") | to_value(CHLoc::XLIT) ||
ParseLit("WBTS") | to_value(CHLoc::WBTS) ||
ParseLit("TPLS") | to_value(CHLoc::TPLS);
constexpr auto dirs_sqr = symbs<'W', 'A', 'N', 'R', 'E', 'B', 'S', 'L'>();
//constexpr auto dirs_hex = Many(symbs<'W','N', 'R', 'E', 'S', 'L'>());
constexpr auto nums = Many("1..9"_range);

constexpr auto to_int = Converter<int>{};
constexpr auto to_probability = Converter<Probability>{};

constexpr auto chr2s = [](auto v) { return std::string(1, v); };

constexpr auto ws = Optional(monadic::Repeat(symbs<' ', '\t'>()));
constexpr auto nl = Optional(symbs<'\n', '\r', '\0'>());
constexpr auto brackets = compound::Wrapper('('_symb, ')'_symb);
constexpr auto octal_number = (ws << monadic::Many(AcceptString("0..7"_range)) >> ws);

constexpr auto alphanumeric = monadic::Many(AcceptString("a..z"_range || "A..Z"_range || "0..9"_range));
constexpr auto int_num = ("1..9"_range % chr2s && Optional(monadic::Many(AcceptString("0..9"_range))));
constexpr auto variable = alphanumeric % Converter<Parameter>{};
constexpr auto line_label = (alphanumeric >> ws) | notopcode;

constexpr auto wn = ("a..z"_range || "A..Z"_range || "0..9"_range || symbs<'(', ')', ',', '.', ' ', '\t'>());
constexpr auto comma = ','_symb;
constexpr auto x_ = (Optional(('X'_symb || 'x'_symb) | success_to_bool) >> Optional(comma));// | success_to_false; //FIX

constexpr auto prob_definition = (brackets(Seq(x_, (int_num >> comma) % to_int, x_, int_num% to_int)) >> ws) % Converter<Probability>{}; //is there a more readable way

constexpr auto pxl_value = "A..Z"_range || "0..9"_range;
constexpr auto pxl_rplace = AcceptString(pxl_value) && AcceptString(pxl_value);
constexpr auto pxl_list = monadic::Many(AcceptString(pxl_value));
constexpr auto xlit = brackets(
(SepBy(pxl_rplace, comma) >> Not(pxl_value)) % Converter<XLIT>{} ||
(Seq(pxl_list, ParseLit("...") | success_to_bool)) % Converter<XLIT>{} ||
(Seq(pxl_list, Success() | to_value(false))) % Converter<XLIT>{});

constexpr auto pxlit = brackets(SepBy(Seq(pxl_value, pxl_value, pxl_value) % to_triplet, comma) % Converter<PXLIT>{});
constexpr auto trans_filter = brackets(RepeatN(variable >> Optional(comma), 8));
constexpr auto pat_name = int_num; // Should also catch explicit pattern name
constexpr auto prob_function = [](auto ...parguments) { return ws << Seq(prob_definition, parguments...); };


constexpr auto WBT_ = brackets(Seq(pxl_list >> Optional(comma), pxl_list >> Optional(comma), pxl_list)) % Converter<WBT>{};
constexpr auto MODE_ = brackets(Seq(mode1 >> comma, mode2 >> comma, mode3)) % Converter<MODE>{};
constexpr auto CAMERA = int_num % to_int % Converter<CAM>{};

constexpr auto XL_ = Seq(int_num % to_int, xlit) % Converter<XL>{};
constexpr auto AXL_ = Seq(nums >> comma, Many(dirs_sqr) >> comma, Many(pxl_value) >> comma, variable, xlit) % Converter<AXL>{};
constexpr auto PXL_ = Seq(dirs_sqr >> comma, int_num% to_int, pxlit) % Converter<PXL>{};
constexpr auto BXL_ = Seq(variable, trans_filter, XL_) % Converter<BXL>{};
constexpr auto BPXL_ = Seq(variable, trans_filter, PXL_) % Converter<BPXL>{};
constexpr auto BAXL_ = Seq(variable, trans_filter, AXL_) % Converter<BAXL>{};


constexpr auto GOTO_ = alphanumeric % Converter<GOTO>{};
constexpr auto IF_ = Seq('('_symb << variable, compare_funcs, variable >> ')'_symb) % Converter<IF>{};
constexpr auto DO_ = alphanumeric % Converter<DO>{};


constexpr auto SVP_ = Seq(line_label, variable, variable, variable, variable) % Converter<SVP>{};
constexpr auto CHV_ = Seq(variable >> comma, change_funcs >> comma, variable, Optional(comma << variable)) % Converter<CHV>{};
constexpr auto CHP_ = Seq(alphanumeric >> comma, alphanumeric) % Converter<CHP>{};
constexpr auto XLI_ = Seq(alphanumeric >> comma, tlit_options >> comma, int_num% to_int, xlit) % Converter<XLI>{}; // Later implementation because of incosistencies


	//constexpr auto generic_args = prob_function(monadic::Many(AcceptString(wn)));

constexpr auto cond_goto = Optional(Optional(comma) << alphanumeric);


//use a complex container for variables / detect what is a number and what a variable /

constexpr auto opcodes = Any(ParseLit("PAT") << octal_number % to_bin_octal,
	ParseLit("WBT") << prob_function(WBT_, cond_goto) % Converter<Command>{},
	ParseLit("MODE") << prob_function(MODE_, cond_goto) % Converter<Command>{},
	ParseLit("CAMERA") << prob_function(CAMERA, cond_goto) % Converter<Command>{},
	ParseLit("XL") << prob_function(XL_, cond_goto) % Converter<Command>{},
	ParseLit("AXL") << prob_function(AXL_, cond_goto) % Converter<Command>{},
	ParseLit("PXL") << prob_function(PXL_, cond_goto) % Converter<Command>{},
	ParseLit("BXL") << prob_function(BXL_, cond_goto) % Converter<Command>{},
	ParseLit("BAXL") << prob_function(BAXL_, cond_goto) % Converter<Command>{},
	ParseLit("BPXL") << prob_function(BPXL_, cond_goto) % Converter<Command>{},
	ParseLit("GOTO") << prob_function(GOTO_) % Converter<Command>{},
	ParseLit("IF") << prob_function(IF_, alphanumeric) % Converter<Command>{},
	ParseLit("DO") << prob_function(DO_, cond_goto) % Converter<Command>{},
	ParseLit("SVP") << prob_function(SVP_, cond_goto) % Converter<Command>{},
	ParseLit("CHV") << prob_function(CHV_, cond_goto) % Converter<Command>{},
	ParseLit("CHP") << prob_function(CHP_, cond_goto) % Converter<Command>{},
	ParseLit("XLI") << prob_function(XLI_, cond_goto) % Converter<Command>{});

constexpr auto pattern = Repeat(Seq(Optional(line_label), opcodes) >> nl);

