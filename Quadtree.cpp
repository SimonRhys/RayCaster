#include "Quadtree.h"



Quadtree::Quadtree(glm::vec2 c, glm::vec2 s)
{
	centre = c;
	size = s;
	leaf = true;
}

bool Quadtree::insert(glm::vec2 p)
{
	if (!leaf)
	{
		if (northWest->inBoundry(p))
		{
			return northWest->insert(p);
		}
		else if (northEast->inBoundry(p))
		{
			return northEast->insert(p);
		}
		else if (southWest->inBoundry(p))
		{
			return southWest->insert(p);
		}
		else if (southEast->inBoundry(p))
		{
			return southEast->insert(p);
		}

		return false;
	}

	if (points.size() < 4 && inBoundry(p))
	{
		points.push_back(p);
		return true;
	}

	subdivide();

	return insert(p);
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
	leaf = false;
	
	glm::vec2 newCentre = glm::vec2(centre.x - 1 / 4 * size.x, centre.y + 1 / 4 * size.y);
	glm::vec2 newSize = glm::vec2(size.x/2, size.y/2);
	northWest = new Quadtree(newCentre, newSize);

	glm::vec2 newCentre = glm::vec2(centre.x + 1 / 4 * size.x, centre.y + 1 / 4 * size.y);
	glm::vec2 newSize = glm::vec2(size.x / 2, size.y / 2);
	northEast = new Quadtree(newCentre, newSize);

	glm::vec2 newCentre = glm::vec2(centre.x - 1 / 4 * size.x, centre.y - 1 / 4 * size.y);
	glm::vec2 newSize = glm::vec2(size.x / 2, size.y / 2);
	southWest = new Quadtree(newCentre, newSize);

	glm::vec2 newCentre = glm::vec2(centre.x + 1 / 4 * size.x, centre.y - 1 / 4 * size.y);
	glm::vec2 newSize = glm::vec2(size.x / 2, size.y / 2);
	southEast = new Quadtree(newCentre, newSize);

	for (int i = 0; i < points.size(); i++)
	{
		insert(points[i]);
	}

	points.clear();
}


Quadtree::~Quadtree()
{
}
