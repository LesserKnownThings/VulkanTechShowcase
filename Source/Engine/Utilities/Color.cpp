#include "Color.h"

const Color Color::red = Color(1.f, 0.f, 0.f);
const Color Color::blue = Color(0.0f, 0.0f, 1.0f);
const Color Color::green = Color(0.0f, 1.0f, 0.0f);
const Color Color::yellow = Color(1.0f, 1.0f, 0.0f);
const Color Color::white = Color(1.0f, 1.0f, 1.0f);
const Color Color::black = Color(0.0f, 0.0f, 0.0f);
const Color Color::gray = Color(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f);
const Color Color::darkGray = Color(169.0f / 255.0f, 169.0f / 255.0f, 169.0f / 255.0f);

glm::vec3 Color::GetVector() const
{
	return glm::vec3(r, g, b);
}

glm::vec4 Color::GetVectorAlpha() const
{
	return glm::vec4(r, g, b, a);
}

uint32_t Color::GetPackedColor() const
{
	return (static_cast<uint32_t>(roundf(a * 255.0f)) << 24) |
		(static_cast<uint32_t>(roundf(b * 255.0f)) << 16) |
		(static_cast<uint32_t>(roundf(g * 255.0f)) << 8) |
		static_cast<uint32_t>(roundf(r * 255.0f));
}
