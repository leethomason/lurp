#include "consoleutil.h"
#include "util.h"
#include "test.h"

#include <string>
#include <vector>

std::string ReadString()
{
	std::string r;

	while (true) {
		char c = (char)getchar();
		if (c == '\n') break;
		r += c;
	}
	return r;
}

Value Value::ParseValue(const std::string& full)
{
	Value v;
	v.rawStr = full;
	if (full.empty()) return v;

	std::vector<std::string> s = lurp::split(full, ' ');
	assert(!s.empty());
	std::string str = s[0];
	if (s.size() > 1) {
		v.option = std::stoi(s[1]);
	}

	if (str.size() == 1) {
		if (std::isdigit(str[0])) {
			v.type = Value::Type::kInt;
			v.intVal = std::stoi(str);
		}
		else if (std::isalpha(str[0])) {
			v.type = Value::Type::kChar;
			v.cVal = str[0];
		}
		return v;
	}
	if (str.size() > 1) {
		if (std::isalpha(str[0]) && std::isdigit(str[1])) {
			v.type = Value::Type::kCharInt;
			v.cVal = str[0];
			v.intVal = std::stoi(str.c_str() + 1);
			return v;
		}
	}
	if (std::isdigit(str[0])) {
		v.type = Value::Type::kInt;
		v.intVal = std::stoi(str);
		return v;
	}
	v.type = Value::Type::kString;
	v.strVal = str;
	return v;
}

void Value::Test()
{
	Value v;
	v = Value::ParseValue("d");
	TEST(v.type == Value::Type::kChar);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 0);
	TEST(v.hasOption() == false);

	v = Value::ParseValue("d2");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 2);
	TEST(v.hasOption() == false);

	v = Value::ParseValue("d32");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 32);

	v = Value::ParseValue("d32 2");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 32);
	TEST(v.hasOption() == true);
	TEST(v.option == 2);

	v = Value::ParseValue("1");
	TEST(v.type == Value::Type::kInt);
	TEST(v.intVal == 1);

	v = Value::ParseValue("Sam");
	TEST(v.type == Value::Type::kString);
	TEST(v.strVal == "Sam");

	v = Value::ParseValue("Sam 2");
	TEST(v.type == Value::Type::kString);
	TEST(v.strVal == "Sam");
	TEST(v.hasOption() == true);
	TEST(v.option == 2);
}

