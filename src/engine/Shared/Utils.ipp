constexpr uint32_t Util_ToFlagBit(uint32_t i)
{
	return 1u << i;
}

template<typename T>
inline T Util_RangeOverflow(T value, T min, T max)
{
	T size = max - min;
	value -= min;
	value = glm::mod(value, size);
	if (value < 0)
	{
		value += size;
	}
	value += min;
	return value;
}

template<typename T>
inline T Util_NextIntegerInRange(T value, T min, T max)
{
	return value < max ? value + 1 : min;
}

template<typename T>
inline T Util_ClampVelocity(T velocity, float maxLen)
{
	float len = glm::length(velocity);
	if (len > maxLen)
	{
		velocity *= maxLen / len;
	}

	return velocity;
}

inline double Util_GetSecondsSinceEpoch()
{
	return std::chrono::system_clock::now().time_since_epoch().count() / 10000000.0;
}
