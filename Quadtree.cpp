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
	return inBoundry(p, centre, size);
}

bool Quadtree::inBoundry(glm::vec2 p, glm::vec2 c, glm::vec2 s)
{
	float startLength = c.x - s.x / 2;
	float endLength = c.x + s.x / 2;

	float startHeight = c.y - s.y / 2;
	float endHeight = c.y + s.y / 2;

	if (p.x <= endLength && p.x >= startLength &&
		p.y <= endHeight && p.y >= startHeight)
	{
		return true;
	}

	return false;
}

bool Quadtree::boundryIntersect(glm::vec2 c1, glm::vec2 s1, glm::vec2 c2, glm::vec2 s2)
{
	glm::vec2 topLeftCorner = glm::vec2(c1.x - 0.5*s1.x, c1.y + 0.5*s1.y);
	glm::vec2 topRightCorner = glm::vec2(c1.x + 0.5*s1.x, c1.y + 0.5*s1.y);
	glm::vec2 botLeftCorner = glm::vec2(c1.x - 0.5*s1.x, c1.y - 0.5*s1.y);
	glm::vec2 botRightCorner = glm::vec2(c1.x + 0.5*s1.x, c1.y - 0.5*s1.y);

	if (inBoundry(topLeftCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(topRightCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(botLeftCorner, c2, s2))
	{
		return true;
	}
	else if (inBoundry(botRightCorner, c2, s2))
	{
		return true;
	}

	return false;
}

bool Quadtree::search(glm::vec2 p)
{
	if (leaf)
	{
		for (int i = 0; i < points.size(); i++)
		{
			if (p == points[i])
			{
				return true;
			}
		}
		
		return false;
	}

	if (northWest->inBoundry(p))
	{
		return northWest->search(p);
	}
	else if (northEast->inBoundry(p))
	{
		return northEast->search(p);
	}
	else if (southWest->inBoundry(p))
	{
		return southWest->search(p);
	}
	else if (southEast->inBoundry(p))
	{
		return southEast->search(p);
	}

	return false;
}

std::vector<glm::vec2> Quadtree::search(glm::vec2 c, glm::vec2 s)
{
	std::vector<glm::vec2> results;
	if (leaf)
	{
		for (int i = 0; i < points.size(); i++)
		{
			if (inBoundry(points[i], c, s))
			{
				results.push_back(points[i]);
			}
		}	
	}
	else
	{
		std::vector<glm::vec2> childResult;

		if (boundryIntersect(northWest->getCentre(), northWest->getSize(), c, s))
		{
			childResult = northWest->search(c, s);
			for (int i = 0; i < childResult.size(); i++)
			{
				results.push_back(childResult[i]);
			}
		}

		
		childResult = northEast->search(c, s);
		for (int i = 0; i < childResult.size(); i++)
		{
			results.push_back(childResult[i]);
		}

		childResult = southWest->search(c, s);
		for (int i = 0; i < childResult.size(); i++)
		{
			results.push_back(childResult[i]);
		}

		childResult = southEast->search(c, s);
		for (int i = 0; i < childResult.size(); i++)
		{
			results.push_back(childResult[i]);
		}
	}

	return results;
}

glm::vec2 Quadtree::getCentre()
{
	return centre;
}

glm::vec2 Quadtree::getSize()
{
	return size;
}

void Quadtree::subdivide()
{
	leaf = false;
	
	glm::vec2 newCentre = glm::vec2(centre.x - 0.25 * size.x, centre.y + 0.25 * size.y);
	glm::vec2 newSize = glm::vec2(size.x/2, size.y/2);
	northWest = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x + 0.25 * size.x, centre.y + 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
	northEast = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x - 0.25 * size.x, centre.y - 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
	southWest = new Quadtree(newCentre, newSize);

	newCentre = glm::vec2(centre.x + 0.25 * size.x, centre.y - 0.25 * size.y);
	newSize = glm::vec2(size.x / 2, size.y / 2);
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
