#include "Object.h"

Object::Object()
	: id(IDManager::GenerateGUID())
{
}

void Object::OverrideID(const UniqueID &inID)
{
	id = inID;
}
