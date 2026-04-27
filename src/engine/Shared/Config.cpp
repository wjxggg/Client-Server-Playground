#include "Config.h"

#include <filesystem>
#include <fstream>

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include "Utils/StringUtils.h"

BE_USING_NAMESPACE
using namespace rapidjson;

//-----------------------------------------------------------------------------
// Interface
//-----------------------------------------------------------------------------

void Config::ReadOrInit()
{
	if (!ReadConfigFile())
	{
		CreateDefaultConfigFile();
	}
}

bool Config::Save() const
{
	std::filesystem::create_directories(BE_MODULE_CONFIG_DIR);
	if (!std::filesystem::is_directory(BE_MODULE_CONFIG_DIR)) return false;

	std::ofstream ofs{m_configPath, std::ios::out | std::ios::trunc};
	if (!ofs.is_open()) return false;

	OStreamWrapper osw{ofs};
	PrettyWriter writer{osw};

	if (!m_doc.Accept(writer))
	{
		ofs.close();
		return false;
	}

	ofs.close();
	return true;
}

std::vector<Config::ValueInfo> &Config::ValueInfoList()
{
	return m_valueInfoList;
}

bool Config::GetValueInfo(const std::string &name, ValueInfo *valueInfo) const
{
	for (const auto &v : m_valueInfoList)
	{
		if (v.GetName() == name)
		{
			*valueInfo = v;
			return true;
		}
	}

	return false;
}

bool Config::HasValue(const std::string &name) const
{
	return m_doc.HasMember(name);
}

template<Utils::StringConvertible T>
bool Config::SetValue(const std::string &name, const T &value)
{
	if (!m_doc.HasMember(name)) return false;

	Value *val;
	if (m_doc[name].Is<T>()) val = &m_doc[name];
	else if (m_doc[name].HasMember("Value") && m_doc[name]["Value"].Is<T>()) val = &m_doc[name]["Value"];
	else return false;

	val->Set(value, m_doc.GetAllocator());

	if (m_valueChangedCallback) m_valueChangedCallback(this);

	return true;
}

template<Utils::StringConvertible T>
bool Config::SetValue(const std::string &name, const std::vector<T> &value)
{
	if (!m_doc.HasMember(name)) return false;

	Value *val;
	if (m_doc[name].IsArray()) val = &m_doc[name];
	else if (m_doc[name].HasMember("Value") && m_doc[name]["Value"].IsArray()) val = &m_doc[name]["Value"];
	else return false;
	if (!val->Empty() && !val[0].Is<T>()) return false;

	for (T item : value)
	{
		Value v;
		v.Set(std::move(item), m_doc.GetAllocator());
		val->PushBack(std::move(v), m_doc.GetAllocator());
	}

	if (m_valueChangedCallback) m_valueChangedCallback(this);

	return true;
}

template<Utils::StringConvertible T>
bool Config::GetValue(const std::string &name, T *value) const
{
	if (!value) return false;
	if (!m_doc.HasMember(name)) return false;

	const Value *val;
	if (m_doc[name].Is<T>()) val = &m_doc[name];
	else if (m_doc[name].HasMember("Value") && m_doc[name]["Value"].Is<T>()) val = &m_doc[name]["Value"];
	else return false;

	*value = val->Get<T>();

	return true;
}

template<Utils::StringConvertible T>
bool Config::GetValue(const std::string &name, std::vector<T> *value) const
{
	if (!value) return false;
	if (!m_doc.HasMember(name)) return false;

	const Value *val;
	if (m_doc[name].IsArray()) val = &m_doc[name];
	else if (m_doc[name].HasMember("Value") && m_doc[name]["Value"].IsArray()) val = &m_doc[name]["Value"];
	else return false;
	if (!val->Empty() && !val[0].Is<T>()) return false;

	for (const auto &item : val->GetArray())
	{
		value->emplace_back(item.Get<T>());
	}

	return true;
}

template bool Config::SetValue(const std::string &name, const bool &value);
template bool Config::SetValue(const std::string &name, const int32_t &value);
template bool Config::SetValue(const std::string &name, const uint32_t &value);
template bool Config::SetValue(const std::string &name, const int64_t &value);
template bool Config::SetValue(const std::string &name, const uint64_t &value);
template bool Config::SetValue(const std::string &name, const float &value);
template bool Config::SetValue(const std::string &name, const double &value);
template bool Config::SetValue(const std::string &name, const std::string &value);

template bool Config::SetValue(const std::string &name, const std::vector<bool> &value);
template bool Config::SetValue(const std::string &name, const std::vector<int32_t> &value);
template bool Config::SetValue(const std::string &name, const std::vector<uint32_t> &value);
template bool Config::SetValue(const std::string &name, const std::vector<int64_t> &value);
template bool Config::SetValue(const std::string &name, const std::vector<uint64_t> &value);
template bool Config::SetValue(const std::string &name, const std::vector<float> &value);
template bool Config::SetValue(const std::string &name, const std::vector<double> &value);
template bool Config::SetValue(const std::string &name, const std::vector<std::string> &value);

template bool Config::GetValue(const std::string &name, bool *value) const;
template bool Config::GetValue(const std::string &name, int32_t *value) const;
template bool Config::GetValue(const std::string &name, uint32_t *value) const;
template bool Config::GetValue(const std::string &name, int64_t *value) const;
template bool Config::GetValue(const std::string &name, uint64_t *value) const;
template bool Config::GetValue(const std::string &name, float *value) const;
template bool Config::GetValue(const std::string &name, double *value) const;
template bool Config::GetValue(const std::string &name, std::string *value) const;

template bool Config::GetValue(const std::string &name, std::vector<bool> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<int32_t> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<uint32_t> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<int64_t> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<uint64_t> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<float> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<double> *value) const;
template bool Config::GetValue(const std::string &name, std::vector<std::string> *value) const;

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

bool Config::ReadConfigFile()
{
	m_doc.RemoveAllMembers();

	std::ifstream ifs{m_configPath, std::ios::in};
	if (!ifs.is_open()) return false;

	Document doc{};
	IStreamWrapper isw{ifs};

	doc.ParseStream(isw);
	ifs.close();

	if (doc.HasParseError()) return false;

	m_doc.RemoveAllMembers();

	for (const auto &valueInfo : m_valueInfoList)
	{
		if (!doc.HasMember(valueInfo.GetName()))
		{
			AddMemberFromValueInfo(valueInfo);
		}
		else
		{
			m_doc.AddMember(Value{valueInfo.GetName(), m_doc.GetAllocator()}, std::move(doc[valueInfo.GetName()]), m_doc.GetAllocator());
		}
	}

	Save();

	return true;
}

void Config::CreateDefaultConfigFile()
{
	m_doc.RemoveAllMembers();

	for (const auto &valueInfo : m_valueInfoList)
	{
		AddMemberFromValueInfo(valueInfo);
	}

	Save();
}

void Config::AddMemberFromValueInfo(const Config::ValueInfo &valueInfo)
{
	Value value{};

	if (valueInfo.OnlyValue())
	{
		SetValueFromValueInfo(valueInfo, &value);
	}
	else
	{
		value.SetObject();
		value.AddMember("Description", Value{valueInfo.GetDesc(), m_doc.GetAllocator()}, m_doc.GetAllocator());

		Value v{};
		SetValueFromValueInfo(valueInfo, &v);

		value.AddMember("Value", v, m_doc.GetAllocator());
	}

	m_doc.AddMember(Value{valueInfo.GetName(), m_doc.GetAllocator()}, value, m_doc.GetAllocator());
}

void Config::SetValueFromValueInfo(const Config::ValueInfo &valueInfo, Value *value)
{
	const auto &defaultValue = valueInfo.GetDefaultValue();

	switch (valueInfo.GetValueType())
	{
		case Config::ValueInfo::BOOL:
		{
			bool v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::INT32:
		{
			int32_t v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::UINT32:
		{
			uint32_t v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::INT64:
		{
			int64_t v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::UINT64:
		{
			uint64_t v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::FLOAT:
		{
			float v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::DOUBLE:
		{
			double v;
			Utils::FromString(defaultValue[0], &v);
			value->Set(v);
			return;
		}
		case Config::ValueInfo::STRING:
		{
			value->Set(defaultValue[0], m_doc.GetAllocator());
			return;
		}
		case Config::ValueInfo::BOOL_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				bool v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::INT32_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				int32_t v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::UINT32_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				uint32_t v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::INT64_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				int64_t v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::UINT64_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				uint64_t v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::FLOAT_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				float v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::DOUBLE_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				double v;
				Utils::FromString(valueStr, &v);
				value->PushBack(v, m_doc.GetAllocator());
			}

			return;
		}
		case Config::ValueInfo::STRING_VECTOR:
		{
			value->SetArray();
			for (const auto &valueStr : defaultValue)
			{
				value->PushBack(Value{valueStr, m_doc.GetAllocator()}, m_doc.GetAllocator());
			}

			return;
		}
	}

	std::unreachable();
}
