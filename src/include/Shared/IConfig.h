#pragma once

#include <string>
#include <vector>

#include <rapidjson/document.h>

#include "EngineConfig.h"
#include "Utils/StringUtils.h"

BE_BEGIN_NAMESPACE

class BE_API Config
{
public:
	class ValueInfo
	{
	public:
		enum ValueType
		{
			BOOL,
			INT32,
			UINT32,
			INT64,
			UINT64,
			FLOAT,
			DOUBLE,
			STRING,

			BOOL_VECTOR,
			INT32_VECTOR,
			UINT32_VECTOR,
			INT64_VECTOR,
			UINT64_VECTOR,
			FLOAT_VECTOR,
			DOUBLE_VECTOR,
			STRING_VECTOR
		};

		ValueInfo() = delete;
		ValueInfo(const Config &rhs) = delete;
		ValueInfo& operator=(const Config &rhs) = delete;
		~ValueInfo() = default;

		template<Utils::StringConvertible T>
		constexpr ValueInfo(const std::string &_name, const T &_defaultValue, const std::string &_desc = "") :
			name(_name),
			desc(_desc)
		{
			if constexpr(std::is_same_v<T, bool>) valueType = BOOL;
			else if constexpr(std::is_same_v<T, int32_t>) valueType = INT32;
			else if constexpr(std::is_same_v<T, uint32_t>) valueType = UINT32;
			else if constexpr(std::is_same_v<T, int64_t>) valueType = INT64;
			else if constexpr(std::is_same_v<T, uint64_t>) valueType = UINT64;
			else if constexpr(std::is_same_v<T, float>) valueType = FLOAT;
			else if constexpr(std::is_same_v<T, double>) valueType = DOUBLE;
			else if constexpr(std::is_same_v<T, std::string>) valueType = STRING;

			defaultValue.emplace_back(Utils::ToString(_defaultValue));
		}

		template<Utils::StringConvertible T>
		constexpr ValueInfo(const std::string &_name, const std::vector<T> &_defaultValue, const std::string &_desc = "") :
			name(_name),
			desc(_desc)
		{
			if constexpr(std::is_same_v<T, bool>) valueType = BOOL_VECTOR;
			else if constexpr(std::is_same_v<T, int32_t>) valueType = INT32_VECTOR;
			else if constexpr(std::is_same_v<T, uint32_t>) valueType = UINT32_VECTOR;
			else if constexpr(std::is_same_v<T, int64_t>) valueType = INT64_VECTOR;
			else if constexpr(std::is_same_v<T, uint64_t>) valueType = UINT64_VECTOR;
			else if constexpr(std::is_same_v<T, float>) valueType = FLOAT_VECTOR;
			else if constexpr(std::is_same_v<T, double>) valueType = DOUBLE_VECTOR;
			else if constexpr(std::is_same_v<T, std::string>) valueType = STRING_VECTOR;

			for (const auto &value : _defaultValue)
			{
				defaultValue.emplace_back(Utils::ToString(value));
			}
		}

		bool OnlyValue() const { return desc.empty(); }
		const std::string &GetName() const { return name; }
		const std::string &GetDesc() const { return desc; }
		ValueType GetValueType() const { return valueType; }
		const std::vector<std::string> &GetDefaultValue() const { return defaultValue; }

	private:
		std::string name;
		std::string desc;
		ValueType valueType;
		std::vector<std::string> defaultValue;

	};

	Config() = delete;
	Config(const Config &rhs) = delete;
    Config& operator=(const Config &rhs) = delete;
	~Config() = default;

	constexpr Config(const std::string &configName,std::vector<ValueInfo> &&valueInfoList, void (*valueChangedCallback)(const Config *) = nullptr) :
		m_configPath(BE_MODULE_CONFIG_DIR "/" + configName + ".json"),
		m_valueInfoList(std::move(valueInfoList)),
		m_valueChangedCallback(valueChangedCallback)
	{

	}

	void ReadOrInit();
	bool Save() const;

	// ValueInfoList must be confirmed before ReadOrInit(),
	// otherwise the value will not be saved
	std::vector<ValueInfo> &ValueInfoList();

	bool GetValueInfo(const std::string &name, ValueInfo *valueInfo) const;

	bool HasValue(const std::string &name) const;

	template<Utils::StringConvertible T>
	bool SetValue(const std::string &name, const T &value);

	template<Utils::StringConvertible T>
	bool SetValue(const std::string &name, const std::vector<T> &value);

	template<Utils::StringConvertible T>
	bool GetValue(const std::string &name, T *value) const;

	template<Utils::StringConvertible T>
	bool GetValue(const std::string &name, std::vector<T> *value) const;

private:
	std::string m_configPath;
	std::vector<ValueInfo> m_valueInfoList;
	rapidjson::Document m_doc{rapidjson::kObjectType};
	void (*m_valueChangedCallback)(const Config *);

	bool ReadConfigFile();
	void CreateDefaultConfigFile();
	void AddMemberFromValueInfo(const Config::ValueInfo &valueInfo);
	void SetValueFromValueInfo(const Config::ValueInfo &valueInfo, rapidjson::Value *value);

};

BE_END_NAMESPACE
