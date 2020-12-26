#pragma once

#include "ExplorTypes.h"




template<std::size_t W = 340,std::size_t H= 240>
class EXPLOR {
	

	static constexpr std::size_t Width = W;
	static constexpr std::size_t Height = H;

	using ImageBuffer = std::array<std::array<char, Width>, Height>;
	using ImageBitmap = std::array<std::array<bool, Width>, Height>;


	std::vector<std::size_t> executeCounter;
	std::map<std::string, int> namedMap;

	char tTable[36] = { 0 };
	std::size_t pc = 0;
	int nextCounter = 0;
	int after_coroutine = -1;

	std::map<std::string, PatternContainer> patterns;
	

	WrapMode wrap_mode;
	RenderMode render_mode;
	NeighbourhoodMode neighbourhood_mode;

	//Randomizer components
	std::mt19937 gen;
	std::uniform_real_distribution<> dis;

public:

	std::vector<Command> commands;
	std::vector<ImageBitmap> frames;
	std::map<std::string, int> variables;
	std::string lastPattern;
	ImageBuffer imageBuffer;

	EXPLOR() {
		for (size_t i = 0; i < Height; i++)
		{
			std::fill(std::begin(imageBuffer[i]), std::end(imageBuffer[i]), '0');
		}

		auto seed = (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count() ^ std::random_device()();
		gen = std::mt19937(seed); //Generate seed 
		dis = std::uniform_real_distribution<double>(0, 1); // Select Distribution
	};

	bool hasEventOccured(unsigned int prob) {
		return prob == 1 || dis(gen) <= 1.0 / prob;
	}

	bool outOfBound(std::pair<int, int> coords) {
		
		if ((coords.first < 0 || coords.first > Height) &&
			(coords.second < 0 || coords.second > Width)) {
			return true;
		}

		return false;
	}

	bool validateCommand(const Command& cmd){
		//Additional validation of commands
		//Check for infinite looop ( occurs always and goto self )
		//Maybe the types should throw exceptions that do the validation
	}

	void addLine(std::string label, Commands cmd) {
		std::visit(overloaded{
			[&label,this](Pattern& p) {
				if (label.length() == 0) {
					patterns.find(lastPattern)->second.data.push_back(p);
					patterns.find(lastPattern)->second.rows++;
					//Throw if last pattern is invalid
				}
				else {
					PatternContainer temp;
					temp.data.push_back(p);
					temp.rows = 1;
					temp.cols = p.size();
					patterns.emplace(std::make_pair(label,std::move(temp)));
					lastPattern = label;
				}
			},
			[&label,this](Command& c) {
				commands.push_back(c);
				executeCounter.push_back(1);
				if (!label.empty())
					namedMap.emplace(std::make_pair(label,commands.size() - 1));
			},

			}, cmd);

	};


	std::pair<int, int> getNeighbour(char dir, std::size_t offset, std::size_t x, std::size_t y, bool overlap = false) {
		int xN = 0, yN = 0;
		switch (dir)
		{
		case 'W': {
			xN = x - offset; yN = y - offset; break;
		}
		case 'A': {
			xN = x; yN = y - offset; break;
		}
		case 'N': {
			xN = x + offset; yN = y - offset; break;
		}
		case 'R': {
			xN = x + offset; yN = y; break;
		}
		case 'E': {
			xN = x + offset; yN = y + offset; break;
		}
		case 'B': {
			xN = x; yN = y + offset; break;
		}
		case 'S': {
			xN = x - offset; yN = y + offset; break;
		}
		case 'L': {
			xN = x - offset; yN = y; break;
		}

		default:
			break;
		}
		
		if (overlap) {
			return { xN % Height, yN % Width };
		}
		
		return { xN,yN };
		
	}

	
	std::size_t countNeighbours(std::size_t x, std::size_t y, std::vector<char> dirs, std::size_t offset, char value) {
		std::size_t count = 0;
		for (const char& d : dirs) {
			auto [xn, yn] = getNeighbour(d, 1, x, y, this->wrap_mode == WrapMode::WRP);
			if (!outOfBound({ xn, yn })) {
				count += (imageBuffer.at(xn).at(yn) == value);
			}
		}

		return count;
	}

	bool inRegion(std::size_t x, std::size_t y, std::vector<char> dirs, std::vector<char> nums, std::vector<char> pxls) {
		std::vector<std::size_t> n;
		//This should not be really needed ( should be numbers beforehand in the parsing/init of the language )
		std::transform(nums.cbegin(), nums.cend(), std::back_inserter(n), [](const char& n) ->std::size_t { return n - '0'; });
		std::sort(n.begin(), n.end());

		
		for (const char& p : pxls) {
			//only because it returns t/f and looks cleaner
			if (std::binary_search(n.begin(), n.end(), countNeighbours(x, y, dirs, 1, p))) {
				return true;
			}
		}
		return false;
		
	}

	void forEachPixel(std::function<void(std::size_t, std::size_t, char)> transform) {
		for (size_t i = 0; i < Height; i++)
		{
			for (size_t j = 0; j < Width; j++)
			{
				transform(i, j, imageBuffer[i][j]);
			}
		}
	}

	void forEachPixelIn(std::function<void(std::size_t, std::size_t, char)> transform, Rectangle rect) {
		for (size_t i = rect.x; i < rect.xM; i++)
		{
			for (size_t j = rect.y; j < rect.yM; j++)
			{
				//if (i % Height != i || j % Width != j) continue;
				auto [oX, oY] = std::pair( i % Height, j % Width );	
				transform(oX, oY, imageBuffer[oX][oY]);
			}
		}

	}

	template<typename T>
	void translation(int x, int y, T const& t) { };

	template<>
	void translation<XL>(int x, int y, XL const& t) {

		if (hasEventOccured(t.prob))
			imageBuffer[x][y] = t.translation.transform(imageBuffer[x][y]);
	};

	template<>
	void translation<AXL>(int x, int y, AXL const& t) {
		// Test the event first so we reduce the amount of "heavy" compute in inRegion
		auto prob = resolveVariable(t.prob);
		if (prob) {
			if (hasEventOccured(prob.value()) && inRegion(x, y, t.directions, t.numbers, t.values)) {
				imageBuffer[x][y] = t.translation.transform(imageBuffer[x][y]);
			}
		}
	};

	template<>
	void translation<PXL>(int x, int y, PXL const& t) {
		auto [xn, yn] = getNeighbour(t.dir, 1, x, y, true);

		if (hasEventOccured(t.prob) && 
			!outOfBound({ xn, yn })) {

			imageBuffer[x][y] = t.translation.transform(imageBuffer[xn][yn], imageBuffer[x][y]);
		}
	};

	template<typename TForm,
		typename = std::enable_if_t<in_transform_group<TForm>>>
		void applyPattern(Rectangles const& rect, PatternContainer const& pat, TForm const& tform) {
		for (std::size_t c = 0; c < rect.c; c++)
		{
			for (std::size_t r = 0; r < rect.r; r++)
			{
				if (pat.data[r][c]) {
					auto [ax,ay] = 
						std::pair{ rect.x + r * rect.h, 
								   rect.y + c * rect.v };
					
					forEachPixelIn([&tform, this](int x, int y, char val) {
						translation(x, y, tform);
						}, Rectangle{ ax - rect.w/2,
							ay - rect.t/2,
							ax + rect.w/2,
							ay + rect.t/2 });

				}
			}
		}
	}

	template<typename TForm, typename = std::enable_if_t<in_transform_group<TForm>>>
	void applyBoxes(Rectangles const& rect, int const& prob, TForm const& tform) {
		for (std::size_t c = 0; c < rect.c; c++)
		{
			for (std::size_t r = 0; r < rect.r; r++)
			{
				if (hasEventOccured(prob)) {
					auto [x, y] = 
						std::pair{ rect.x + r * rect.h,
								   rect.y + c * rect.v };
					
					forEachPixelIn([&tform, this](std::size_t x, std::size_t y, char val) {
						translation(x, y, tform);
						}, Rectangle{ x - rect.w/2,
							y - rect.t/2,
							x + rect.w/2,
							y + rect.t/2 });
				}
			}
		}
	}

	std::variant<int, PatternContainer> resolvePattern(Parameter& p) {
		if (p.is_variable) {
			//Get pattern
			auto res = patterns.find(std::get<std::string>(p.value));
			if (res != patterns.end()) {
				return res->second;
			}
			else {

				throw std::exception{ "Unknown pattern name mentioned" };
			}
		}
		return std::get<int>(p.value);
	}

	std::optional<int> resolveVariable(Parameter const& p) {
		//Parameter can be missing
		if (!p.is_valid) return std::nullopt;

		if (p.is_variable) {
			std::string name = std::get<std::string>(p.value);
			auto res = variables.find(name);
			if (res == variables.end()) {
				//Add initial variable value
				variables.emplace(std::make_pair(name, 0));
				return 0;
			}
			return res->second;
		}
		return std::get<int>(p.value);
	}

	void setVariable(Parameter& p, int newValue) {
		//Throw exeption if its a int
		std::string name = std::get<std::string>(p.value);
		auto res = variables.find(name);
		if (res == variables.end()) {
			variables.emplace(std::make_pair(name, newValue));
		}
		else {
			res->second = newValue;
		}

	}

	void modifyVariable(Parameter& var, CHOp operation, Parameter& from, Parameter& to) {
		auto to_ = resolveVariable(to);
		auto from_ = resolveVariable(from);
		auto var_ = resolveVariable(var);
		int newValue = 0;
		if (to_) {
			//Choose a random value in the range
			newValue = std::trunc(dis(gen) * abs(to_.value() - from_.value())) + from_.value();
		}
		else {
			newValue = from_.value();
		}

		switch (operation)
		{
		case CHOp::SET:
			setVariable(var, newValue);
			break;
		case CHOp::ADD:
			setVariable(var, var_.value() + newValue);
			break;
		case CHOp::SUB:
			setVariable(var, var_.value() - newValue);
			break;
		case CHOp::MPY:
			setVariable(var, var_.value() * newValue);
			break;
		case CHOp::DIV:
			setVariable(var, var_.value() / newValue);
			break;
		default:
			throw std::exception{ "Unknown CHV operation specified" };
			break;
		}
	}

	void transformXLIT(CommandType& cmd, int prob, XLIT transform) {
		std::visit(overloaded{
			[&transform](BXL& comnd) {
				if (std::holds_alternative<std::string>(comnd.transform.translation.replacements)) {

					for (char& c : std::get<std::string>(comnd.transform.translation.replacements))
						c = transform.transform(c);
				}else {

				   for (std::string& cset : std::get<std::vector<std::string>>(comnd.transform.translation.replacements))
					   for (char& c : cset)
						   c = transform.transform(c);
				}
				},
				[](auto&&) {
					//Throw error
					throw std::exception{"Illegal operation label for XLI(transforming XLIT), must be only BXL"};
				}
			}, cmd);
	}

	void transformNUMS(CommandType& cmd, int prob, XLIT transform) {

		std::visit(overloaded{
			[&transform,&prob,this](AXL& comnd) {

				for (char& c : comnd.numbers) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[&transform,&prob,this](BAXL& comnd) {
				for (char& c : comnd.transform.numbers) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[](auto&&) {
				//Throw error
				throw std::exception{"Illegal operation label for XLI(transforming NUMS), must be only [AXL,BAXL]"};
			}
			}, cmd);
	}

	void transformCHST(CommandType& cmd, int prob, XLIT transform) {

		std::visit(overloaded{
			[&transform,&prob,this](AXL& comnd) {
				for (char& c : comnd.values) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[&transform,&prob,this](BAXL& comnd) {
				for (char& c : comnd.transform.values) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[](auto&&) {
				//Throw error
				throw std::exception{"Illegal operation label for XLI(transforming CHST), must be only [AXL,BAXL]"};
			}
			}, cmd);
	}

	void transformTPLS(CommandType& cmd, int prob, XLIT transform) {

		std::visit(overloaded{
			[&transform,&prob,this](PXL& comnd) {
				for (std::array<char,3>& trpl : comnd.translation.translations) {
					for (char& c : trpl) {
						if (hasEventOccured(prob)) {
							c = transform.transform(c);
						}
					}
				}
			},
			[&transform,&prob,this](BPXL& comnd) {
				for (std::array<char,3>& trpl : comnd.transform.translation.translations) {
					for (char& c : trpl) {
						if (hasEventOccured(prob)) {
							c = transform.transform(c);
						}
					}
				}
			},
			[](auto&&) {
				//Throw error
				throw std::exception{"Illegal operation label for XLI(transforming TPLS), must be only [PXL,BPXL]"};
			}
			}, cmd);
	}

	void transformWBTS(CommandType& cmd, int prob, XLIT transform) {

		std::visit(overloaded{
			[&transform,&prob,this](WBT& comnd) {
				for (char& c : comnd.white) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
				for (char& c : comnd.black) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
				for (char& c : comnd.twinkle) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[](auto&&) {
				//Throw error
				throw std::exception{"Illegal operation label for XLI(transforming WBTS), must be only WBT"};
			}
			}, cmd);
	}

	void transformDIRS(CommandType& cmd, int prob, XLIT transform) {

		std::visit(overloaded{
			[&transform,&prob,this](AXL& c) {
				for (char& c : c.directions) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[&transform,&prob,this](BAXL& c) {
				for (char& c : c.transform.directions) {
					if (hasEventOccured(prob)) {
						c = transform.transform(c);
					}
				}
			},
			[&transform, &prob, this](PXL& c) {
				if (hasEventOccured(prob)) {
					c.dir = transform.transform(c.dir);
				}
			},
			[&transform,&prob,this](BPXL& c) {
				if (hasEventOccured(prob)) {
					c.transform.dir = transform.transform(c.transform.dir);
				}
			},
			[](auto&&) {
				//Throw error
				throw std::exception{"Illegal operation label for XLI(transforming DIRS), must be only [AXL,BAXL,PXL,BPXL]"};
			}
			}, cmd);
	}


	template<typename TFormCmd, typename = std::enable_if_t<in_cmd_group_b<TFormCmd>>>
	void patternTransform(TFormCmd& command) {
		auto pat = resolvePattern(command.pattern);

		//if i could traverse structures easy this could be nicer
		std::vector<std::size_t> rect = { 0,0,0,0,0,0,0,0 };
		std::transform(std::cbegin(command.rectangle), std::cend(command.rectangle), std::begin(rect),
			[this](auto item) { return resolveVariable(item).value();  });

		Rectangles r = rect;
		
		//Use the boxes applicator
		std::visit(overloaded{
			[&command,&r,this](int& prob) {
				applyBoxes(r, prob, command.transform);
			},
			[&command,&r,this](PatternContainer& pat) {
				applyPattern(r, pat, command.transform);
			}
			}, pat);
	}

	bool valueToPixel(char c) {

	}

	void execute() {
		std::vector<int> stack_counter;
		//Maybe a more sophisticated aproach to handling the program counter		
		after_coroutine = -1;
		
		while (pc < commands.size()) {
			nextCounter = -1;

			Probability& prob = commands.at(pc).prob;
			CommandType& cmd = commands.at(pc).cmd;
			std::string_view next = commands.at(pc).goto_;
			//std::cout << "Executing Line # " << pc << " + " << nextCounter << '\n';
			if (prob.check(executeCounter[pc])) {

				//Execute appropriate code
				std::visit(overloaded{
					[this](WBT& command) {
						for (char c : command.white) {
							tTable[pxl_to_index(c)] = 0;
						}
						for (char c : command.black) {
							tTable[pxl_to_index(c)] = 1;
						}
						for (char c : command.twinkle) {
							tTable[pxl_to_index(c)] = 2;
						}
					},
					[this](MODE& command) {
						neighbourhood_mode = command.neighbourhood;
						render_mode = command.render;
						wrap_mode = command.wrap;
					},
					[this](CAM& command) {
						for (size_t frame = 0; frame < (size_t)command.frames; frame++)
						{
							//For each frame create a bitmap
							auto temp = new ImageBitmap;

							forEachPixel([&temp,this](int x, int y, char value) {
								char newValue = tTable[pxl_to_index(value)];
								if (newValue == 2) {
									newValue = dis(gen) <= 0.5;
								}
								
								(*temp)[x][y] = newValue;
							});

							frames.push_back(std::forward<ImageBitmap>(*temp));
							delete temp;
						}
					},
					[this](XL& command) {
						forEachPixel([&command,this](int x, int y, char value) {
							translation(x,y,command);
						});

					},
					[this](AXL& command) {
						forEachPixel([&command,this](int x, int y, char value) {
							translation(x,y,command);
						});

					},
					[this](PXL& command) {
						forEachPixel([&command,this](int x,int y,char value) {
							translation(x,y,command);
						});

					},
					[this](BXL& command) {
						patternTransform(command);
					},
					[this](BPXL& command) {
						patternTransform(command);
					},
					[this](BAXL& command) {
						patternTransform(command);
					},
					[this](GOTO& command) {
						
						if (command.label == "DONE") {
							//Special instruction to return to prev point
							//of execution before starting the DO statement
							nextCounter = after_coroutine;
							after_coroutine = -1;
						}else {
							auto labeledLine = namedMap.find(command.label);
							if (labeledLine != namedMap.end()) {
								nextCounter = labeledLine->second;
							}
						}
						
					},
					[this,&next](IF& command) {
						
						int lhs = resolveVariable(command.lhs).value();
						int rhs = resolveVariable(command.rhs).value();
						bool comparison = false;

						switch (command.cmp)
						{
						case Compares::EQ:
							comparison = lhs == rhs;
							break;
						case Compares::LT:
							comparison = lhs < rhs;
							break;
						case Compares::GT:
							comparison = lhs > rhs;
							break;
						default:
							break;

						}

						//Transfer execution if test succeds
						if (comparison) {
							auto labeledLine = namedMap.find(next.data());
							if (labeledLine != namedMap.end()) {
								nextCounter = labeledLine->second;
							}
						}

					},
					[this,&next](DO& command) {
						
						//Set execution counter resume point
						if (next == "") {
							//Execution continues at a certain label
							auto labeledLine = namedMap.find(next.data());
							if (labeledLine != namedMap.end()) {
								after_coroutine = labeledLine->second;
							}
						}
						else {
							//Continue after the invocation of the coroutine
							after_coroutine = ++pc;
						}

						//Transfer execution
						auto labeledLine = namedMap.find(command.label);
						if (labeledLine != namedMap.end()) {
							nextCounter = labeledLine->second;
						}

					},
					[this](SVP& command) {
						std::size_t x = resolveVariable(command.x).value();
						std::size_t y = resolveVariable(command.y).value();
						std::size_t w = resolveVariable(command.width).value();
						std::size_t h = resolveVariable(command.height).value();

						if (x + w > 340 || y + h > 240) {
							throw std::exception{ "[SVP] Pattern out of bounds." };
						}
						else {
							PatternContainer newPattern(w, h);
							forEachPixelIn([this, &newPattern, &x, &y, &w, &h](int xC, int yC, char value) {
								char newValue = tTable[pxl_to_index(value)];
								if (newValue == 2) {
									newValue = dis(gen) <= 0.5;
								}
								newPattern.data[xC - x][yC - y] = newValue;
								}, Rectangle{ x, y, x + w, y + h });

							//Push pattern
							patterns.emplace(std::make_pair(command.label, newPattern));
						}
					},
					[this](CHV& command) {
						modifyVariable(command.location, command.operation, command.value1, command.value2);
					},
					[this](CHP& command) {
						auto labeledLine = namedMap.find(command.instance);
						int cmdIndex = -1;
						if (labeledLine != namedMap.end()) {
							cmdIndex = labeledLine->second;
						}
						//Throw if not found
						std::visit(overloaded{
							[&command](BXL& c) {
								c.pattern = command.newLabel;
							},
							[&command](BAXL& c) {
								c.pattern = command.newLabel;
							},
							[&command](BPXL& c) {
								c.pattern = command.newLabel;
							},
							[](auto&&) {
								//Throw error
								throw std::exception{"[CHP] Change Pattern can only be performed on [BXL,BAXL,BPXL]"};
							}
							}, commands[cmdIndex].cmd);



					},
					[this](XLI& command) {
						auto labeledLine = namedMap.find(command.label);
						int cmdIndex = -1;
						if (labeledLine != namedMap.end()) {
							cmdIndex = labeledLine->second;
						}

						//Throw on wrong command
						switch (command.location)
						{
						case CHLoc::NUMS:
							transformNUMS(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						case CHLoc::DIRS:
							transformDIRS(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						case CHLoc::CHST:
							transformCHST(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						case CHLoc::XLIT:
							transformXLIT(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						case CHLoc::WBTS:
							transformWBTS(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						case CHLoc::TPLS:
							transformTPLS(commands[cmdIndex].cmd, command.prob, command.transform);
							break;
						default:
							break;
						}
					}
					}, cmd);

				
				
					auto labeledLine = namedMap.find(next.data());

					if (labeledLine != namedMap.end()) {
						nextCounter = labeledLine->second;
					}
				
			
			}
			
			++executeCounter[pc];


			if (nextCounter != -1) {
				pc = nextCounter;
			}
			else {
				++pc;
			}
			
		}
	};
};