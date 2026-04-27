#pragma once

#include <cassert>
#include <charconv>
#include <string>
#include <vector>

#include "EngineConfig.h"

BE_BEGIN_NAMESPACE

namespace Utils
{

template <typename T>
concept StringConvertible = (
	std::is_same_v<T, bool> 		||
	std::is_same_v<T, int32_t> 		||
	std::is_same_v<T, uint32_t> 	||
	std::is_same_v<T, int64_t> 		||
	std::is_same_v<T, uint64_t> 	||
	std::is_same_v<T, float> 		||
	std::is_same_v<T, double> 		||
	std::is_same_v<T, std::string>
);

template<Utils::StringConvertible T>
inline bool FromString(const std::string &str, T *value)
{
	if (!value) return false;

	return std::from_chars(str.data(), str.data() + str.size(), *value).ec == std::errc{};
}

inline bool FromString(const std::string &str, bool *value)
{
	if (!value) return false;

	if (str == "true")
	{
		*value = true;
		return true;
	}
	else if (str == "false")
	{
		*value = false;
		return true;
	}

	return false;
}

inline bool FromString(const std::string &str, std::string *value)
{
	if (!value) return false;

	*value = str;
	return true;
}

template<Utils::StringConvertible T>
inline std::string ToString(const T &value)
{
	return std::to_string(value);
}

inline std::string ToString(const bool &value)
{
	return value ? "true" : "false";
}

inline std::string ToString(const std::string &value)
{
	return value;
}

inline bool SplitString(const std::string &str, std::vector<std::string> *strs, char delimiter = ' ')
{
	assert(delimiter != '\"' && "Why the fuck?");

	if (!strs) return false;

	if (str.size() == 0)
	{
		strs->emplace_back("");
		return true;
	}

	struct Segment
	{
		size_t pos{0};
		size_t count{0};
	};

	std::vector<Segment> segments;

	size_t len = str.size();

	size_t l = 0;
	size_t r = 0;

	if (delimiter != ' ' && str[0] == delimiter)
	{
		segments.emplace_back(Segment{});
	}

	while (l < len)
	{
		if (str[l] == delimiter) ++l;

		// Remove leading space
		while (l < len && std::isspace(str[l])) ++l;

		// Empty str
		if (l == len)
		{
			segments.emplace_back(Segment{});
		}
		// Use ""
		else if (str[l] == '\"')
		{
			++l;
			r = l;

			// Find next '\"'
			while (r < len && str[r] != '\"') ++r;

			if (r == len) return false;

			segments.emplace_back(Segment{l, r - l});

			// Find next delimiter
			l = r + 1;
			while (l < len && str[l] != delimiter)
			{
				// Ensure there are only space between the string and the next delimiter
				if (!std::isspace(str[l])) return false;
				++l;
			}
		}
		else
		{
			r = l;

			// Find next delimiter or space
			while (r < len && str[r] != delimiter && !std::isspace(str[r]))
			{
				// Ensure there are no '\"' in the string
				if (str[r] == '\"') return false;
				++r;
			}

			segments.emplace_back(Segment{l, r - l});

			// Find next delimiter
			l = r;
			while (l < len && str[l] != delimiter)
			{
				// Ensure there are only space between the string and the next delimiter
				if (!std::isspace(str[l])) return false;
				++l;
			}
		}
	}

	for (auto segment : segments)
	{
		strs->emplace_back(str.substr(segment.pos, segment.count));
	}

	return true;
}

}

BE_END_NAMESPACE
