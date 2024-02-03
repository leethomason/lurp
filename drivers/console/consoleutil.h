#pragma once

#include <limits.h>
#include <string>

struct Value
{
	// d		kChar	
	// d2		kCharInt
	// d32		kCharInt
	// 1		kInt
	// Sam		kString
	// all of the above
	// d32 2    option=2

	static Value ParseValue(const std::string& full);
	static void Test();

	enum class Type {
		kNone,
		kChar,
		kCharInt,
		kInt,
		kString,
	};

	char cVal = 0;
	int intVal = 0;
	std::string strVal;
	std::string rawStr;
	int option = INT_MIN;

	Type type = Type::kNone;

	bool intInRange(int end) const {
		return type == Type::kInt && intVal >= 0 && intVal < end;
	}
	bool charIntInRange(char c, int end) const {
		return type == Type::kCharInt && c == cVal && intVal >= 0 && intVal < end;
	}
	bool hasOption() const { return option != INT_MIN; }
};


struct BattleSpec {
	int level = 5;
	int fighters = 1;
	int shooters = 0;
	int arcanes = 0;

	static BattleSpec Parse(const std::string& str);
};

inline int ctoi(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	return 0;
}