#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct Color
{
public:
	Color() {}
	Color(float inR, float inG, float inB)
		: r(inR), g(inG), b(inB), a(1.f) {}
	Color(float inR, float inG, float inB, float inA)
		: r(inR), g(inG), b(inB), a(inA) {}

	Color& operator=(float color[])
	{
		r = color[0];
		g = color[1];
		b = color[2];
		a = color[3];
		return *this;
	}

	bool operator==(const Color& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const Color& other) const
	{
		return !(*this == other);
	}

	static Color Lerp(const Color& a, const Color& b, float t)
	{
		return {
			a.r + (b.r - a.r) * t,
			a.g + (b.g - a.g) * t,
			a.b + (b.b - a.b) * t,
			a.a + (b.a - a.a) * t };
	}

	// Needs a valid array of 4 elements for this
	void GetArray(float outColor[]) const
	{
		outColor[0] = r;
		outColor[1] = g;
		outColor[2] = b;
		outColor[3] = a;
	}

	glm::vec3 GetVector() const;
	glm::vec4 GetVectorAlpha() const;
	uint32_t GetPackedColor() const;

	static const Color red;
	static const Color blue;
	static const Color green;
	static const Color yellow;
	static const Color white;
	static const Color black;
	static const Color gray;
	static const Color darkGray;

	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;
};
