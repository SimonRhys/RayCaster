#include "Quadtree.h"



Quadtree::Quadtree(glm::vec2 c, glm::vec2 s)
{
	centre = c;
	size = s;
	empty = true;
	children = false;
}

bool Quadtree::insert(glm::vec2 p)
{
	if (points.size() < 5 && inBoundry(p))
	{
		points.push_back(p);
		return true;
	}

	subdivide();

	if (northEast->inBoundry(p))
	{
		northWest->insert(p);
	}
}

bool Quadtree::inBoundry(glm::vec2 p)
{
	float startLength = centre.x -= size.x;
	float endLength = centre.x += size.x;

	float startHeight = centre.y -= size.y;
	float endHeight = centre.y += size.y;

	if (p.x < endLength && p.x > startLength &&
		p.y < endHeight && p.y > startHeight)
	{
		return true;
	}

	return false;
}

void Quadtree::subdivide()
{

}


Quadtree::~Quadtree()
{
}
